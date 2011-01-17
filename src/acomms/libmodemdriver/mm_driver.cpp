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

#include <sstream>

#include <boost/foreach.hpp>
#include <boost/assign.hpp>

#include "goby/acomms/modem_message.h"
#include "goby/util/logger.h"

#include "mm_driver.h"
#include "driver_exception.h"

using namespace goby::util; // for NMEASentence & goby_time()

boost::posix_time::time_duration goby::acomms::MMDriver::MODEM_WAIT = boost::posix_time::seconds(3);
boost::posix_time::time_duration goby::acomms::MMDriver::WAIT_AFTER_REBOOT = boost::posix_time::seconds(2);
boost::posix_time::time_duration goby::acomms::MMDriver::ALLOWED_SKEW = boost::posix_time::seconds(2);

boost::posix_time::time_duration goby::acomms::MMDriver::HYDROID_GATEWAY_GPS_REQUEST_INTERVAL = boost::posix_time::seconds(30);



std::string goby::acomms::MMDriver::SERIAL_DELIMITER = "\r";
unsigned goby::acomms::MMDriver::PACKET_FRAME_COUNT [] = { 1, 3, 3, 2, 2, 8 };
unsigned goby::acomms::MMDriver::PACKET_SIZE [] = { 32, 32, 64, 256, 256, 256 };


goby::acomms::MMDriver::MMDriver(std::ostream* log /*= 0*/)
    : ModemDriverBase(log, SERIAL_DELIMITER),
      log_(log),
      last_write_time_(goby_time()),
      waiting_for_modem_(false),
      startup_done_(false),
      global_fail_count_(0),
      present_fail_count_(0),
      clock_set_(false),
      last_hydroid_gateway_gps_request_(goby_time()),
      is_hydroid_gateway_(false)
{
    initialize_talkers();
    set_serial_baud(DEFAULT_BAUD);
}

goby::acomms::MMDriver::~MMDriver()
{ }


void goby::acomms::MMDriver::startup()
{
    modem_start();
    
    set_clock();
    
    write_cfg();
    // useful to have all the CFG values in the log file for later analysis
    query_all_cfg();
    startup_done_ = true;
}

void goby::acomms::MMDriver::do_work()
{    
    // don't try to set the clock if we already have outgoing
    // messages queued since the time will be wrong by the time
    // we can send
    if(!clock_set_ && out_.empty())
        set_clock();
    
    // keep trying to send stuff to the modem
    handle_modem_out();

    // read any incoming messages from the modem
    std::string in;
    while(modem_read(in))
    {
        boost::trim(in);
        if(log_) *log_ << group("mm_in") << in << std::endl;

	// Check for whether the hydroid_gateway buoy is being used and if so, remove the prefix
	if (is_hydroid_gateway_) in.erase(0, HYDROID_GATEWAY_PREFIX_LENGTH);
	
	if(callback_in_raw) callback_in_raw(in); 
  
        try
        {
            NMEASentence nmea(in, NMEASentence::VALIDATE);
            handle_modem_in(nmea);
        }
        catch(std::exception& e)
        {
            if(log_) *log_ << group("mm_in") << warn << e.what() << std::endl;
        }
    }

    // if we're using a hydroid buoy query it for its GPS position
    if(is_hydroid_gateway_ &&
       last_hydroid_gateway_gps_request_ + HYDROID_GATEWAY_GPS_REQUEST_INTERVAL < goby_time())
    {
        modem_write(hydroid_gateway_gps_request_);
        last_hydroid_gateway_gps_request_ = goby_time();
    }
    
}


void goby::acomms::MMDriver::handle_mac_initiate_transmission(const acomms::ModemMessage& m)
{   
    //$CCCYC,CMD,ADR1,ADR2,Packet Type,ACK,Npkt*CS
    NMEASentence nmea("$CCCYC", NMEASentence::IGNORE);
    nmea.push_back(0); // CMD: deprecated field
    nmea.push_back(m.src()); // ADR1
    nmea.push_back(m.dest()); // ADR2
    nmea.push_back(m.rate()); // Packet Type (transmission rate)
    nmea.push_back(int(m.ack())); // ACK: deprecated field, this bit may be used for something that's not related to the ack
    nmea.push_back(PACKET_FRAME_COUNT[m.rate()]); // number of frames we want
    write(nmea);        
}

void goby::acomms::MMDriver::handle_mac_initiate_ranging(const acomms::ModemMessage& m, RangingType type)
{
    switch(type)
    {
        case MODEM:
        {
            //$CCMPC,SRC,DEST*CS
            NMEASentence nmea("$CCMPC", NMEASentence::IGNORE);
            nmea.push_back(m.src()); // ADR1
            nmea.push_back(m.dest()); // ADR2
            write(nmea);
            break;
        }
        

        case REMUS_LBL:
        {
            // $CCPDT,GRP,CHANNEL,SF,STO,Timeout,AF,BF,CF,DF*CS
            NMEASentence nmea("$CCPDT", NMEASentence::IGNORE);
            nmea.push_back(1); // 
            nmea.push_back(m.src() % 4); // can only use 1-4
            nmea.push_back(0); // synchronize may not work?
            nmea.push_back(0); // synchronize may not work?
            nmea.push_back(2500);
            nmea.push_back(1);
            nmea.push_back(1);
            nmea.push_back(1);
            nmea.push_back(1);
            write(nmea);
            break;
        }
        
    }
}



void goby::acomms::MMDriver::handle_modem_out()
{
    if(out_.empty()) return;
    
    NMEASentence& nmea = out_.front();
    
    bool resend = waiting_for_modem_ && (last_write_time_ <= (goby_time() - MODEM_WAIT));
    if(!waiting_for_modem_ || resend)
    {
        if(resend)
        {
            if(log_) *log_ << group("mm_out") << warn << "resending " << nmea.front() <<  " because we had no modem response for " << (goby_time() - last_write_time_).total_seconds() << " second(s). " << std::endl;
            ++global_fail_count_;
            ++present_fail_count_;
            if(global_fail_count_ == MAX_FAILS_BEFORE_DEAD)
                throw(driver_exception(std::string("modem appears to not be responding. going down")));
            
            if(present_fail_count_ == RETRIES)
            {
                if(log_) *log_  << group("mm_out") << warn << "modem did not respond to our command even after " << RETRIES << " retries. continuing onwards anyway..." << std::endl;
                out_.clear();
                return;
            }
        }
        
        mm_write(nmea);
        
        waiting_for_modem_ = true;
        last_write_time_ = goby_time();
    }
}

void goby::acomms::MMDriver::mm_write(const NMEASentence& nmea_out)
{
    if(log_) *log_ << group("mm_out") << hydroid_gateway_modem_prefix_ << nmea_out.message() << std::endl;
 
    if(callback_out_raw) callback_out_raw(nmea_out.message());
    modem_write(hydroid_gateway_modem_prefix_ + nmea_out.message_cr_nl());
}


void goby::acomms::MMDriver::pop_out()
{
    waiting_for_modem_ = false;
    out_.pop_front();
    present_fail_count_ = 0;
}


void goby::acomms::MMDriver::measure_noise(unsigned milliseconds_to_average)
{
    NMEASentence nmea_out("$CCCFR", NMEASentence::IGNORE);
    nmea_out.push_back(milliseconds_to_average);
    mm_write(nmea_out);
}

void goby::acomms::MMDriver::write(NMEASentence& nmea)
{    
    out_.push_back(nmea);
    handle_modem_out(); // try to push it now without waiting for the next call to do_work();
}


void goby::acomms::MMDriver::set_clock()
{
    NMEASentence nmea("$CCCLK", NMEASentence::IGNORE);
    boost::posix_time::ptime p = goby_time();
    
    nmea.push_back(int(p.date().year()));
    nmea.push_back(int(p.date().month()));
    nmea.push_back(int(p.date().day()));
    nmea.push_back(int(p.time_of_day().hours()));
    nmea.push_back(int(p.time_of_day().minutes()));
    nmea.push_back(int(p.time_of_day().seconds()));
    
    write(nmea);

    // take a breath to let the clock be set 
    sleep(1);
}

void goby::acomms::MMDriver::write_cfg()
{
    BOOST_FOREACH(const std::string& s, cfg_)
    {
        NMEASentence nmea("$CCCFG", NMEASentence::IGNORE);        
        nmea.push_back(boost::to_upper_copy(s));
        write(nmea);
    }    
}

void goby::acomms::MMDriver::query_all_cfg()
{
    NMEASentence nmea("$CCCFQ,ALL", NMEASentence::IGNORE);
    write(nmea);
}


void goby::acomms::MMDriver::handle_modem_in(const NMEASentence& nmea)
{    
    // look at the talker front (talker id)
    switch(talker_id_map_[nmea.talker_id()])
    {
      // only process messages from CA (modem)
    case CA: case SN: global_fail_count_ = 0; break; // reset fail count - modem is alive!
            // ignore the rest
    case CC: case GP: default: return;
    }

    acomms::ModemMessage m_in;
    // look at the talker back (message code)
    switch(sentence_id_map_[nmea.sentence_id()])
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
        case TTA: tta(nmea, m_in); break; // cycle init
        default:                   break;
    }

    // clear the last send given modem acknowledgement
    if(!out_.empty() && out_.front().sentence_id() == nmea.sentence_id())
        pop_out();
    
    // for anyone who needs to know that we got a message 
    if(callback_in_parsed) callback_in_parsed(m_in);
}

void goby::acomms::MMDriver::rxd(const NMEASentence& nmea, acomms::ModemMessage& m)
{
    m.set_src(nmea[1]);
    m.set_dest(nmea[2]);
    m.set_ack(nmea[3]);
    m.set_frame(nmea[4]);
    m.set_data(nmea[5]);

    if(callback_receive) callback_receive(m);
}
void goby::acomms::MMDriver::ack(const NMEASentence& nmea, acomms::ModemMessage& m)
{
    m.set_src(nmea[1]);
    m.set_dest(nmea[2]);
    m.set_frame(nmea[3]);
    m.set_ack(nmea[4]);

    if(callback_ack) callback_ack(m);
}
void goby::acomms::MMDriver::drq(const NMEASentence& nmea_in, acomms::ModemMessage& m)
{
    // read the drq
    m.set_time(modem_time2ptime(nmea_in[1]));
    m.set_src(nmea_in[2]);
    m.set_dest(nmea_in[3]);
//  m.set_ack(nmea_in[4]);
    m.set_max_size(nmea_in[5]);
    m.set_frame(nmea_in[6]);

    if(callback_data_request) callback_data_request(m);

    // write the txd
    NMEASentence nmea_out("$CCTXD", NMEASentence::IGNORE);
    nmea_out.push_back(m.src());
    nmea_out.push_back(m.dest());
    nmea_out.push_back(int(m.ack()));
    nmea_out.push_back(m.data());

    write(nmea_out);   
}

void goby::acomms::MMDriver::cfg(const NMEASentence& nmea, acomms::ModemMessage& m)
{
    nvram_cfg_[nmea[1]] = nmea.as<int>(2);
    
    if(out_.empty() || (out_.front().sentence_id() != "CFG" && out_.front().sentence_id() != "CFQ"))
        return;
    
    if(out_.front().sentence_id() == "CFQ") pop_out();
}

void goby::acomms::MMDriver::clk(const NMEASentence& nmea, acomms::ModemMessage& m)
{
    if(out_.empty() || out_.front().sentence_id() != "CLK")
        return;
    
    using namespace boost::posix_time;
    using namespace boost::gregorian;
    // modem responds to the previous second, which is why we subtract one second from the current time
    ptime expected = goby_time();
    ptime reported = ptime(date(nmea.as<int>(1),
                                nmea.as<int>(2),
                                nmea.as<int>(3)),
                           time_duration(nmea.as<int>(4),
                                         nmea.as<int>(5),
                                         nmea.as<int>(6),
                                         0));


    // make sure the modem reports its time as set at the right time
    // we may end up oversetting the clock, but better safe than sorry...
    if(reported >= (expected - ALLOWED_SKEW))
        clock_set_ = true;    
    
}

void goby::acomms::MMDriver::mpa(const NMEASentence& nmea, acomms::ModemMessage& m){}

void goby::acomms::MMDriver::mpr(const NMEASentence& nmea, acomms::ModemMessage& m)
{
    // $CAMPR,SRC,DEST,TRAVELTIME*CS
    m.set_src(nmea[1]);
    m.set_dest(nmea[2]);

    if(nmea.size() > 3)
        m.add_tof(nmea[3]);

    if(callback_range_reply) callback_range_reply(m);
}

void goby::acomms::MMDriver::rev(const NMEASentence& nmea, acomms::ModemMessage& m)
{
    if(nmea[2] == "INIT")
    {
        // reboot
        sleep(WAIT_AFTER_REBOOT.total_seconds());
        clock_set_ = false;
    }
    else if(nmea[2] == "AUV")
    {
        //check the clock
        using boost::posix_time::ptime;
        ptime expected = goby_time();
        ptime reported = modem_time2ptime(nmea[1]);

        if(reported < (expected - ALLOWED_SKEW))
            clock_set_ = false;        
    }
}

void goby::acomms::MMDriver::err(const NMEASentence& nmea, acomms::ModemMessage& m)
{
    *log_ << group("mm_out") << warn << "modem reports error: " << nmea.message() << std::endl;
}

void goby::acomms::MMDriver::cyc(const NMEASentence& nmea, acomms::ModemMessage& m)
{
    // somewhat "loose" interpretation of some of the fields
    m.set_src(nmea[2]); // ADR1
    m.set_dest(nmea[3]); // ADR2
    m.set_ack(nmea[5]); // "ACK", actually deprecated free bit
    m.set_frame(nmea[6]); // Npkts, number of packets
}

void goby::acomms::MMDriver::tta(const NMEASentence& nmea, ModemMessage& m)
{
  m.add_tof(goby::util::as<double>(nmea[1]));
  m.add_tof(goby::util::as<double>(nmea[2]));
  m.add_tof(goby::util::as<double>(nmea[3]));
  m.add_tof(goby::util::as<double>(nmea[4]));
  if(callback_range_reply) callback_range_reply(m);    
}



boost::posix_time::ptime goby::acomms::MMDriver::modem_time2ptime(const std::string& mt)
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


void goby::acomms::MMDriver::initialize_talkers()
{
    boost::assign::insert (sentence_id_map_)
        ("ACK",ACK)("DRQ",DRQ)("RXA",RXA)("RXD",RXD)
        ("RXP",RXP)("TXD",TXD)("TXA",TXA)("TXP",TXP) 
        ("TXF",TXF)("CYC",CYC)("MPC",MPC)("MPA",MPA)
        ("MPR",MPR)("RSP",RSP)("MSC",MSC)("MSA",MSA)
        ("MSR",MSR)("EXL",EXL)("MEC",MEC)("MEA",MEA) 
        ("MER",MER)("MUC",MUC)("MUA",MUA)("MUR",MUR) 
        ("PDT",PDT)("PNT",PNT)("TTA",TTA)("MFD",MFD) 
        ("CLK",CLK)("CFG",CFG)("AGC",AGC)("BBD",BBD) 
        ("CFR",CFR)("CST",CST)("MSG",MSG)("REV",REV) 
        ("DQF",DQF)("SHF",SHF)("MFD",MFD)("SNR",SNR) 
        ("DOP",DOP)("DBG",DBG)("FFL",FFL)("FST",FST) 
        ("ERR",ERR);

    boost::assign::insert (talker_id_map_)
        ("CC",CC)("CA",CA)("SN",SN)("GP",GP); 
}



void goby::acomms::MMDriver::set_hydroid_gateway_prefix(int id)
{
    is_hydroid_gateway_ = true;
    // If the buoy is in use, make the prefix #M<ID>
    hydroid_gateway_gps_request_ = "#G" + as<std::string>(id) + "\r\n";        
    hydroid_gateway_modem_prefix_ = "#M" + as<std::string>(id);
    
    if(log_) *log_ << "Setting the hydroid_gateway buoy prefix: out=" << hydroid_gateway_modem_prefix_ << std::endl;
}

