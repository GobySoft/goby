// copyright 2009 t. schneider tes@mit.edu
// 
// this file is part of the goby-acomms WHOI Micro-Modem driver.
// goby-acomms is a collection of libraries 
// for acoustic underwater networking
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

#include <boost/foreach.hpp>
#include <boost/assign.hpp>

#include "acomms/modem_message.h"
#include "util/streamlogger.h"

using namespace serial; // for NMEASentence

micromodem::MMDriver::MMDriver(std::ostream* os /*= 0*/)
    : DriverBase(os, SERIAL_DELIMITER),
      os_(os),
      last_write_time_(time(NULL)),
      waiting_for_modem_(false),
      startup_done_(false),
      global_fail_count_(0),
      present_fail_count_(0),
      clock_set_(false)
{
    initialize_talkers();
    set_baud(DEFAULT_BAUD);
}

micromodem::MMDriver::~MMDriver()
{ }


void micromodem::MMDriver::startup()
{
    serial_start();
    
    write_cfg();
    check_cfg();
    startup_done_ = true;
}

void micromodem::MMDriver::do_work()
{    
    if(!clock_set_ && out_.empty())
        set_clock();
    
    // keep trying to send stuff to the modem
    handle_modem_out();

    // read any incoming messages from the modem
    std::string in;
    while(serial_read(in))
    {
        boost::trim(in);
        if(os_) *os_ << group("mm_in") << "|" << microsec_simple_time_of_day() << "| " << in << std::endl;
        
        NMEASentence nmea(in, NMEASentence::STRICT);
        handle_modem_in(nmea);
    }
}


void micromodem::MMDriver::initiate_transmission(const modem::Message& m)
{   
    //$CCCYC,CMD,ADR1,ADR2,Packet Type,ACK,Npkt*CS
    NMEASentence nmea("$CCCYC", NMEASentence::NOT_STRICT);
    nmea.push_back(0); // CMD: deprecated field
    nmea.push_back(m.src()); // ADR1
    nmea.push_back(m.dest()); // ADR2
    nmea.push_back(m.rate()); // Packet Type (transmission rate)
    nmea.push_back(m.ack()); // ACK: deprecated field, this bit may be used for something that's not related to the ack
    nmea.push_back(acomms_util::PACKET_FRAME_COUNT[m.rate()]); // number of frames we want
    validate_and_write(nmea);
}


void micromodem::MMDriver::handle_modem_out()
{
    if(out_.empty())
        return;
    
    NMEASentence& nmea = out_.front();
    
    bool resend = waiting_for_modem_ && (last_write_time_ <= (time(NULL) - MODEM_WAIT));
    if(!waiting_for_modem_ || resend)
    {
        if(resend)
        {
            if(os_) *os_ << group("mm_out") << warn << "resending " << nmea.talker() <<  " because we had no modem response for " << (time(NULL) - last_write_time_) << " second(s). " << std::endl;
            ++global_fail_count_;
            ++present_fail_count_;
            if(global_fail_count_ == MAX_FAILS_BEFORE_DEAD)
            {
                throw(std::runtime_error(std::string("modem appears to not be responding. going down")));
            }
            
            if(present_fail_count_ == RETRIES)
            {
                if(os_) *os_  << group("mm_out") << warn << "modem did not respond to our command even after " << RETRIES << " retries. continuing onwards anyway..." << std::endl;
                out_.clear();
                return;
            }
        }

        if(os_) *os_ << group("mm_out") << "|" << microsec_simple_time_of_day() << "| " << nmea.message() << std::endl;
        
        serial_write(nmea.message_cr_nl());

        waiting_for_modem_ = true;
        last_write_time_ = time(NULL);
    }
}

void micromodem::MMDriver::pop_out()
{
    waiting_for_modem_ = false;
    out_.pop_front();
    present_fail_count_ = 0;
}


void micromodem::MMDriver::validate_and_write(NMEASentence& nmea)
{
    try { nmea.validate(); }
    catch(std::exception& e)
    {
        if(os_) *os_ << group("mm_out") << warn << e.what() << std::endl;
        return;
    }
    
    out_.push_back(nmea);

    handle_modem_out(); // try to push it now without waiting for the next call to do_work();
}


void micromodem::MMDriver::set_clock()
{
    NMEASentence nmea("$CCCLK", NMEASentence::NOT_STRICT);
    boost::posix_time::ptime p = boost::posix_time::second_clock::universal_time();
    
    nmea.push_back(static_cast<int>(p.date().year()));
    nmea.push_back(static_cast<int>(p.date().month()));
    nmea.push_back(static_cast<int>(p.date().day()));
    nmea.push_back(static_cast<int>(p.time_of_day().hours()));
    nmea.push_back(static_cast<int>(p.time_of_day().minutes()));
    nmea.push_back(static_cast<int>(p.time_of_day().seconds()));
    
    validate_and_write(nmea);
}

void micromodem::MMDriver::write_cfg()
{
    BOOST_FOREACH(const std::string& s, cfg_)
    {
        NMEASentence nmea("$CCCFG", NMEASentence::NOT_STRICT);        
        nmea.push_back(boost::to_upper_copy(s));
        validate_and_write(nmea);
    }    
}

void micromodem::MMDriver::check_cfg()
{
    NMEASentence nmea("$CCCFQ,ALL", NMEASentence::NOT_STRICT);
    validate_and_write(nmea);
}


void micromodem::MMDriver::handle_modem_in(NMEASentence& nmea)
{
    try { nmea.validate(); }
    catch(std::exception& e)
    {
        *os_ << group("mm_out") << warn << e.what() << std::endl;
        return;
    }
    
    // look at the talker front (talker id)
    switch(talker_fronts_map_[nmea.talker_front()])
    {
        // only process messages from CA (modem)
        case CA: global_fail_count_ = 0; break; // reset fail count - modem is alive!
        // ignore the rest
        case CC: case SN: case GP: default: return;
    }

    modem::Message m_in;
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

void micromodem::MMDriver::rxd(NMEASentence& nmea, modem::Message& m)
{
    m.set_src(nmea[1]);
    m.set_dest(nmea[2]);
    m.set_ack(nmea[3]);
    m.set_frame(nmea[4]);
    m.set_data(nmea[5]);

    callback_receive(m);
}
void micromodem::MMDriver::ack(NMEASentence& nmea, modem::Message& m)
{
    m.set_src(nmea[1]);
    m.set_dest(nmea[2]);
    m.set_frame(nmea[3]);
    m.set_ack(nmea[4]);

    callback_ack(m);
}
void micromodem::MMDriver::drq(NMEASentence& nmea_in, modem::Message& m_in)
{
    // read the drq
    m_in.set_t(modem_time2unix_time(nmea_in[1]));
    m_in.set_src(nmea_in[2]);
    m_in.set_dest(nmea_in[3]);
//  m_in.set_ack(nmea_in[4]);
    m_in.set_size(nmea_in[5]);
    m_in.set_frame(nmea_in[6]);

    modem::Message m_out;
    // fetch the data
    callback_datarequest(m_in, m_out);

    // write the txd
    NMEASentence nmea_out("$CCTXD", NMEASentence::NOT_STRICT);
    nmea_out.push_back(m_out.src());
    nmea_out.push_back(m_out.dest());
    nmea_out.push_back(m_out.ack());
    nmea_out.push_back(m_out.data());

    validate_and_write(nmea_out);   
}

void micromodem::MMDriver::cfg(NMEASentence& nmea, modem::Message& m)
{
    if(out_.front().talker_back() != "CFG" && out_.front().talker_back() != "CFQ")
        return;

    pop_out();
}

void micromodem::MMDriver::clk(NMEASentence& nmea, modem::Message& m)
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

void micromodem::MMDriver::mpa(NMEASentence& nmea, modem::Message& m){}
void micromodem::MMDriver::mpr(NMEASentence& nmea, modem::Message& m){}

void micromodem::MMDriver::rev(NMEASentence& nmea, modem::Message& m)
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

void micromodem::MMDriver::err(NMEASentence& nmea, modem::Message& m)
{
    *os_ << group("mm_out") << warn << "modem reports error: " << nmea.message() << std::endl;
}

void micromodem::MMDriver::cyc(NMEASentence& nmea, modem::Message& m)
{
    // somewhat "loose" interpretation of some of the fields
    m.set_src(nmea[2]); // ADR1
    m.set_dest(nmea[3]); // ADR2
    m.set_ack(nmea[5]); // "ACK", actually deprecated free bit
    m.set_frame(nmea[6]); // Npkts, number of packets
}


boost::posix_time::ptime micromodem::MMDriver::modem_time2posix_time(const std::string& mt)
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

double micromodem::MMDriver::modem_time2unix_time(const std::string& mt)
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

void micromodem::MMDriver::initialize_talkers()
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
 
}
