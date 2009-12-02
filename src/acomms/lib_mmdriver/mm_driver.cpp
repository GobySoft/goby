// t. schneider tes@mit.edu 06.19.09
// ocean engineering graduate student - mit / whoi joint program
// massachusetts institute of technology (mit)
// laboratory for autonomous marine sensing systems (lamss)
// 
// this is mm_driver.cpp, part of lib_mmdriver, a
// fast WHOI Micro-Modem driver
//
// see the readme file within this directory for information
// pertaining to usage and purpose of this script.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This software is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this software.  If not, see <http://www.gnu.org/licenses/>.

#include "mm_driver.h"

#include <boost/thread/mutex.hpp>
#include <boost/foreach.hpp>
#include <boost/assign.hpp>

#include "serial_client.h"
#include "modem_message.h"
#include "term_color.h"


using namespace termcolor;

MMDriver::MMDriver(FlexCout& tout) : baud_(DEFAULT_BAUD),
                                                 modem_id_(acomms_util::BROADCAST_ID),
                                                 serial_(io_,
                                                         in_,
                                                         in_mutex_),
                                                 tout_(tout),
                                                 last_write_time_(time(NULL)),
                                                 waiting_for_modem_(false),
                                                 startup_done_(false),
                                                 global_fail_count_(0),
                                                 present_fail_count_(0),
                                                 clock_set_(false)
{
    initialize_talkers();
}

void MMDriver::startup()
{
    
    tout_ << "setting SRC to given modem_id" << std::endl;
    cfg_["SRC"] = modem_id_;
    

    tout_ << "opening serial port " << serial_port_
          << " @ " << baud_ << std::endl;
    try { serial_.start(serial_port_, baud_); }
    catch (...)
    {
        tout_ << die << "failed to open serial port: " << serial_port_ << ". make sure this port exists and that you have permission to use it." << std::endl;
    }   

    // start the serial client
    // toss the io service (for the serial client) into its own thread
    boost::thread t(boost::bind(&asio::io_service::run, &io_));


    if(!log_path_.empty())
    {
        using namespace boost::posix_time;
    std::string file_name =
        "acomms_src" + boost::lexical_cast<std::string>(modem_id_)
        + "_" + to_iso_string(second_clock::universal_time()) + ".txt";
    
        tout_ << "logging nmea values to file: " << file_name << std::endl;  
        
        fout_.open(std::string(log_path_ + "/" + file_name).c_str());
        
        // if fails, try logging to this directory
        if(!fout_.is_open())
        {
            fout_.open(std::string("./" + file_name).c_str());
            tout_ << warn << "logging to current directory because given directory is unwritable!" << std::endl;
        }
        // if still no go, quit
        if(!fout_.is_open())
            tout_ << die << "cannot write to current directory, so cannot log." << std::endl;
    }
    
    write_cfg();
    check_cfg();
    startup_done_ = true;
}

void MMDriver::do_work()
{    
    if(!clock_set_ && out_.empty())
        set_clock();
    
    // keep trying to send stuff to the modem
    handle_modem_out();

    // read any incoming messages from the modem
    while(!in_.empty())
    {
        // SerialClient can write to this deque, so lock it
        boost::mutex::scoped_lock lock(in_mutex_);
        
        std::string& in = in_.front();
        boost::trim(in);
        
        tout_ << group("mm_in") << "|" << microsec_simple_time_of_day() << "| " << lt_blue << in << std::endl;
        if(fout_.is_open()) fout_ << "|" << microsec_iso_date_time() << "| " << in << std::endl;
        
        NMEA nmea(in, STRICT);
        handle_modem_in(nmea);
        in_.pop_front();
    }
}

void MMDriver::handle_modem_out()
{
    if(out_.empty())
        return;
    
    NMEA& nmea = out_.front();
    
    bool resend = waiting_for_modem_ && (last_write_time_ <= (time(NULL) - MODEM_WAIT));
    if(!waiting_for_modem_ || resend)
    {
        if(resend)
        {
            tout_ << warn << "resending " << nmea.talker() <<  " because we had no modem response for " << (time(NULL) - last_write_time_) << " second(s). " << std::endl;
            ++global_fail_count_;
            ++present_fail_count_;
            if(global_fail_count_ == MAX_FAILS_BEFORE_DEAD)
                tout_ << die << "modem appears to not be responding. going down" << std::endl;
            if(present_fail_count_ == RETRIES)
            {
                tout_ << warn << "modem did not respond to our command even after " << RETRIES << " retries. continuing onwards anyway..." << std::endl;
                out_.clear();
                return;
            }
        }
        
        callback_out_raw(nmea.message_no_cs());

        tout_ << group("mm_out") << "|" << microsec_simple_time_of_day() << "| " << lt_magenta << nmea.message() << std::endl;
        if(fout_.is_open()) fout_ << "|" + microsec_iso_date_time() + "| " << nmea.message() << std::endl;
        
        serial_.write(nmea.message_cr_nl());

        waiting_for_modem_ = true;
        last_write_time_ = time(NULL);
    }
}

void MMDriver::pop_out()
{
    waiting_for_modem_ = false;
    out_.pop_front();
    present_fail_count_ = 0;
}


void MMDriver::validate_and_write(NMEA& nmea)
{
    try { nmea.validate(); }
    catch(std::exception& e)
    {
        tout_ << warn << e.what() << std::endl;
        return;
    }
    
    out_.push_back(nmea);

    handle_modem_out(); // try to push it now without waiting for the next call to do_work();
}


void MMDriver::set_clock()
{
    NMEA nmea("$CCCLK", NOT_STRICT);
    boost::posix_time::ptime p = boost::posix_time::second_clock::universal_time();
    
    nmea.push_back(static_cast<int>(p.date().year()));
    nmea.push_back(static_cast<int>(p.date().month()));
    nmea.push_back(static_cast<int>(p.date().day()));
    nmea.push_back(static_cast<int>(p.time_of_day().hours()));
    nmea.push_back(static_cast<int>(p.time_of_day().minutes()));
    nmea.push_back(static_cast<int>(p.time_of_day().seconds()));
    
    validate_and_write(nmea);
}

void MMDriver::write_cfg()
{
    typedef std::pair<std::string, unsigned int> P;
    BOOST_FOREACH(const P& p, cfg_)
    {
        NMEA nmea("$CCCFG", NOT_STRICT);        
        nmea.push_back(boost::to_upper_copy(p.first));
        nmea.push_back(p.second);
        validate_and_write(nmea);
    }    
}

void MMDriver::check_cfg()
{
    NMEA nmea("$CCCFQ,ALL", NOT_STRICT);
    validate_and_write(nmea);
}


void MMDriver::handle_modem_in(NMEA& nmea)
{
    try { nmea.validate(); }
    catch(std::exception& e)
    {
        tout_ << warn << e.what() << std::endl;
        return;
    }
    
    callback_in_raw(nmea.message_no_cs());

    // look at the talker front (talker id)
    switch(talker_fronts_map_[nmea.talker_front()])
    {
        // only process messages from CA (modem)
        case CA: global_fail_count_ = 0; break; // reset fail count - modem is alive!
        // ignore the rest
        case CC: case SN: case GP: default: return;
    }

    micromodem::Message m_in;
    // look at the talker back (message code)
    switch(talker_backs_map_[nmea.talker_back()])
    {
        case ACK: ack(nmea, m_in); break; // acknowledge
        case DRQ: drq(nmea, m_in); break; // data request
        case RXD: rxd(nmea, m_in); break; // data receive            
        case MPA: mpa(nmea, m_in); break; // ping acknowledge
        case MPR: mpr(nmea, m_in); break; // ping response
        case REV: rev(nmea, m_in); break; // software revision
        case ERR: err(nmea, m_in); break; // error message
        case CFG: cfg(nmea, m_in); break; // configuration
        case CLK: clk(nmea, m_in); break; // clock
        case CYC: cyc(nmea, m_in); break; // cycle init
        default:                   break;
    }

    // clear the last send given modem acknowledgement
    if(!out_.empty() && out_.front().talker_back() == nmea.talker_back()) pop_out();

    // for anyone who needs to know that we got a message 
    callback_decoded(m_in);
}

void MMDriver::rxd(NMEA& nmea, micromodem::Message& m)
{
    m.set_src(nmea[1]);
    m.set_dest(nmea[2]);
    m.set_ack(nmea[3]);
    m.set_frame(nmea[4]);
    m.set_data(nmea[5]);

    callback_rxd(m);
}
void MMDriver::ack(NMEA& nmea, micromodem::Message& m)
{
    m.set_src(nmea[1]);
    m.set_dest(nmea[2]);
    m.set_frame(nmea[3]);
    m.set_ack(nmea[4]);

    callback_ack(m);
}
void MMDriver::drq(NMEA& nmea_in, micromodem::Message& m_in)
{
    // read the drq
    m_in.set_t(modem_time2unix_time(nmea_in[1]));
    m_in.set_src(nmea_in[2]);
    m_in.set_dest(nmea_in[3]);
//  m_in.set_ack(nmea_in[4]);
    m_in.set_size(nmea_in[5]);
    m_in.set_frame(nmea_in[6]);

    micromodem::Message m_out;
    // fetch the data
    callback_drq(m_in, m_out);

    // write the txd
    NMEA nmea_out("$CCTXD", NOT_STRICT);
    nmea_out.push_back(m_out.src());
    nmea_out.push_back(m_out.dest());
    nmea_out.push_back(m_out.ack());
    nmea_out.push_back(m_out.data());

    validate_and_write(nmea_out);   
}

void MMDriver::cfg(NMEA& nmea, micromodem::Message& m)
{
    if(out_.front().talker_back() != "CFG" && out_.front().talker_back() != "CFQ")
        return;

    pop_out();
}

void MMDriver::clk(NMEA& nmea, micromodem::Message& m)
{
    if(out_.front().talker_back() != "CLK")
        return;

    using namespace boost::posix_time;
    using namespace boost::gregorian;
    // modem responds to the previous second, which is why we subtract one second from the current time
    ptime expected = now();
    ptime reported = ptime(date(nmea.part_as_int(1),
                                nmea.part_as_int(2),
                                nmea.part_as_int(3)),
                           time_duration(nmea.part_as_int(4),
                                         nmea.part_as_int(5),
                                         nmea.part_as_int(6),
                                         0));


    // make sure the modem reports its time as set at the right time
    // we may end up oversetting the clock, but better safe than sorry...
    if((expected == reported) || (expected - seconds(ALLOWED_SKEW) == reported))
        clock_set_ = true;
}

void MMDriver::mpa(NMEA& nmea, micromodem::Message& m){}
void MMDriver::mpr(NMEA& nmea, micromodem::Message& m){}

void MMDriver::rev(NMEA& nmea, micromodem::Message& m)
{
    if(nmea[2] == "INIT")
    {
        // reboot
        sleep(WAIT_AFTER_REBOOT);
        clock_set_ = false;
    }
    else if(nmea[2] == "AUV")
    {
        //check the clock
        using namespace boost::posix_time;
        ptime expected = now();
        ptime reported = modem_time2posix_time(nmea[1]);

        if((expected != reported) && (expected - seconds(ALLOWED_SKEW) != reported))
            clock_set_ = false;
    }
    
}

void MMDriver::err(NMEA& nmea, micromodem::Message& m)
{
    tout_ << warn << "modem reports error: " << nmea.message() << std::endl;
}

void MMDriver::cyc(NMEA& nmea, micromodem::Message& m)
{
    // somewhat "loose" interpretation of some of the fields
    m.set_src(nmea[2]); // ADR1
    m.set_dest(nmea[3]); // ADR2
    m.set_ack(nmea[5]); // "ACK", actually deprecated free bit
    m.set_frame(nmea[6]); // Npkts, number of packets
}


boost::posix_time::ptime MMDriver::modem_time2posix_time(const std::string& mt)
{   
    using namespace boost::posix_time;
    using namespace boost::gregorian;

    if(!(mt.length() == 6 || mt.length() == 11))
        return ptime(not_a_date_time);  
    else
    {
        // HHMMSS or HHMMSS.SSSS 
        std::string s_hour = mt.substr(0,2), s_min = mt.substr(2,2), s_sec = mt.substr(4,2), s_fs = "0";
        if(mt.length() == 11)
            s_fs = mt.substr(7);
        
        try
        {
            int hour = boost::lexical_cast<int>(s_hour);
            int min = boost::lexical_cast<int>(s_min);
            int sec = boost::lexical_cast<int>(s_sec);
            int micro_sec = boost::lexical_cast<int>(s_fs) * 100;
            
            return (ptime(date(day_clock::universal_day()), time_duration(hour, min, sec, 0)) + microseconds(micro_sec));
        }
        catch (boost::bad_lexical_cast&)
        {
            return ptime(not_a_date_time);
        }        
    }
}

double MMDriver::modem_time2unix_time(const std::string& mt)
{
    using namespace boost::posix_time;
    using namespace boost::gregorian;
    
    ptime given_time = modem_time2posix_time(mt);
    if (given_time == not_a_date_time)
        return -1;
    
    ptime time_t_epoch(date(1970,1,1));

    time_duration diff = given_time - time_t_epoch;
    return (diff.total_seconds() + diff.fractional_seconds() / time_duration::ticks_per_second());
}

void MMDriver::initialize_talkers()
{
    boost::assign::insert (talker_backs_map_)
        ("ACK",ACK)
        ("DRQ",DRQ)
        ("RXA",RXA)
        ("RXD",RXD)
        ("RXP",RXP)
        ("TXD",TXD)
        ("TXA",TXA) 
        ("TXP",TXP) 
        ("TXF",TXF)  
        ("CYC",CYC) 
        ("MPC",MPC) 
        ("MPA",MPA) 
        ("MPR",MPR) 
        ("RSP",RSP) 
        ("MSC",MSC) 
        ("MSA",MSA) 
        ("MSR",MSR) 
        ("EXL",EXL) 
        ("MEC",MEC) 
        ("MEA",MEA) 
        ("MER",MER) 
        ("MUC",MUC) 
        ("MUA",MUA) 
        ("MUR",MUR) 
        ("PDT",PDT) 
        ("PNT",PNT) 
        ("TTA",TTA) 
        ("MFD",MFD) 
        ("CLK",CLK) 
        ("CFG",CFG) 
        ("AGC",AGC) 
        ("BBD",BBD) 
        ("CFR",CFR) 
        ("CST",CST) 
        ("MSG",MSG) 
        ("REV",REV) 
        ("DQF",DQF) 
        ("SHF",SHF) 
        ("MFD",MFD) 
        ("CLK",CLK) 
        ("SNR",SNR) 
        ("DOP",DOP) 
        ("DBG",DBG) 
        ("FFL",FFL) 
        ("FST",FST) 
        ("ERR",ERR);

    boost::assign::insert (talker_fronts_map_)
        ("CC",CC)
        ("CA",CA)
        ("SN",SN)
        ("GP",GP);
 
   
    boost::assign::insert (talker_human_map_)
        ("CAACK","Acknowledgment of a transmitted packet")
        ("CADRQ","Data request message, modem to host")
        ("CARXA","Received ASCII message, modem to host")
        ("CARXD","Received binary message, modem to host")
        ("CARXP","Incoming packet detected, modem to host")
        ("CCTXD","Transmit binary data message, host to modem")  
        ("CCTXA","Transmit ASCII data message, host to modem")
        ("CATXD","Echo back of transmit binary data message")  
        ("CATXA","Echo back of transmit ASCII data message")
        ("CCTXP","Start of packet transmission, modem to host")
        ("CCTXF","End of packet transmission, modem to host")
        ("CCCYC","Network Cycle Initialization Command")
        ("CACYC","Echo of Network Cycle Initialization command")
        ("CCMPC","MiniPacket Ping command, host to modem")
        ("CAMPC","Echo of Ping command, modem to host")
        ("CAMPA","A Ping has been received, modem to host")
        ("CAMPR","Reply to Ping has been received, modem to host")
        ("CCRSP","Pinging with an FM sweep")
        ("CARSP","Respose to FM sweep ping command")
        ("CCMSC","Sleep command, host to modem")
        ("CAMSC","Echo of Sleep command, modem to host")
        ("CAMSA","A Sleep was received acoustically, modem to host")
        ("CAMSR","A Sleep reply was received, modem to host")
        ("CCEXL","External hardware control command, local modem only")
        ("CCMEC","External hardware control command, host to modem")
        ("CAMEC","Echo of hardware control command, modem to host")
        ("CAMEA","Hardware control command received acoustically")
        ("CAMER","Hardware control command reply received")
        ("CCMUC","User MiniPacket command, host to modem")
        ("CAMUC","Echo of user MiniPacket, modem to host")
        ("CAMUA","MiniPacket received acoustically, modem to host")
        ("CAMUR","Reply to MiniPacket received, modem to host")
        ("CCPDT","Ping REMUS digital transponder, host to modem")  
        ("CCPNT","Ping narrowband transponder, host to modem")
        ("SNTTA","Transponder travel times, modem to host")
        ("SNMFD","Nav matched filter information, modem to host")  
        ("CCCLK","Set clock, host to modem")
        ("CCCFG","Set NVRAM configuration parameter, host to modem")
        ("CACFG","Report NVRAM configuration parameter, modem to host")
        ("CCCFQ","Query configuration parameter, host to modem")
        ("CCAGC","Set automatic gain control")
        ("CCBBD","Dump baseband data to serial port")  
        ("CCCFR","Measure noise level at receiver, host to modem")
        ("SNCFR","Noise report, modem to host")  
        ("CACST","Communication cycle statistics")  
        ("CAMSG","Transaction message, modem to host") 
        ("CAREV","Software revision message, modem to host")
        ("CADQF","Data quality factor information, modem to host")
        ("CASHF","Shift information, modem to host")
        ("CAMFD","Comms matched filter information, modem to host")
        ("CACLK","Time/Date message, modem to host")
        ("CASNR","SNR statistics on the incoming PSK packet")  
        ("CADOP","Doppler speed message, modem to host")
        ("CADBG","Low level debug message, modem to host")  
        ("CCFFL","FIFO flush, host to modem")
        ("CAFFL","FIFO flush acknowledgement host")  
        ("CCFST","Query FIFO status, host to modem")  
        ("CAFST","FIFO status, modem to host")
        ("CAERR","Error message, modem to host");
}
