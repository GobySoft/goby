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
#include <boost/algorithm/string.hpp>

#include "goby/util/logger.h"

#include "mm_driver.h"
#include "driver_exception.h"

using goby::util::NMEASentence;
using goby::util::goby_time;
using goby::util::ptime2unix_double;
using goby::util::as;
using google::protobuf::uint32;

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
      is_hydroid_gateway_(false),
      local_cccyc_(false)
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
    try_send();

    // read any incoming messages from the modem
    std::string in;
    while(modem_read(in))
    {
        boost::trim(in);
        if(log_) *log_ << group("mm_in") << in << std::endl;

	// Check for whether the hydroid_gateway buoy is being used and if so, remove the prefix
	if (is_hydroid_gateway_) in.erase(0, HYDROID_GATEWAY_PREFIX_LENGTH);
  
        try
        {
            NMEASentence nmea(in, NMEASentence::VALIDATE);
            process_receive(nmea);
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


void goby::acomms::MMDriver::handle_initiate_transmission(const protobuf::ModemMsgBase& base_msg)
{
    // we initiated this cycle so don't grab data *again* on the CACYC
    local_cccyc_ = true;
    protobuf::ModemDataInit init_msg;
    init_msg.mutable_base()->CopyFrom(base_msg);
    init_msg.set_num_frames(PACKET_FRAME_COUNT[base_msg.rate()]);
    cache_outgoing_data(init_msg);

    // don't start a cycle if we have no data
    if(!cached_data_msgs_.empty())
    {
        //$CCCYC,CMD,ADR1,ADR2,Packet Type,ACK,Npkt*CS
        NMEASentence nmea("$CCCYC", NMEASentence::IGNORE);
        nmea.push_back(0); // CMD: deprecated field
        nmea.push_back(init_msg.base().src()); // ADR1
        nmea.push_back(cached_data_msgs_.front().base().dest()); // ADR2
        nmea.push_back(init_msg.base().rate()); // Packet Type (transmission rate)
        nmea.push_back(0); // ACK: deprecated field, this bit may be used for something that's not related to the ack
        nmea.push_back(init_msg.num_frames()); // number of frames we want
        
        write(nmea);
    }
}

void goby::acomms::MMDriver::handle_initiate_ranging(const protobuf::ModemRangingRequest& m)
{
    switch(m.type())
    {
        case protobuf::MODEM_RANGING:
        {
            //$CCMPC,SRC,DEST*CS
            NMEASentence nmea("$CCMPC", NMEASentence::IGNORE);
            nmea.push_back(m.base().src()); // ADR1
            nmea.push_back(m.base().dest()); // ADR2
            write(nmea);
            break;
        }
        

        case protobuf::REMUS_LBL_RANGING:
        {
            // $CCPDT,GRP,CHANNEL,SF,STO,Timeout,AF,BF,CF,DF*CS
            NMEASentence nmea("$CCPDT", NMEASentence::IGNORE);
            nmea.push_back(1); // GRP 1 is the only group right now 
            nmea.push_back(m.base().src() % 4 + 1); // can only use 1-4
            nmea.push_back(0); // synchronize may not work?
            nmea.push_back(0); // synchronize may not work?
            // REMUS LBL is 50 ms turn-around time, assume 1500 m/s speed of sound
            nmea.push_back(int((m.max_range()*2.0 / ROUGH_SPEED_OF_SOUND)*1000 + REMUS_LBL_TURN_AROUND_MS));
            nmea.push_back(m.enable_beacons() >> 0 & 1);
            nmea.push_back(m.enable_beacons() >> 1 & 1);
            nmea.push_back(m.enable_beacons() >> 2 & 1);
            nmea.push_back(m.enable_beacons() >> 3 & 1);
            write(nmea);
            break;
        }
        
    }
}



void goby::acomms::MMDriver::try_send()
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

void goby::acomms::MMDriver::write(const NMEASentence& nmea)
{    
    out_.push_back(nmea);
    try_send(); // try to push it now without waiting for the next call to do_work();
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


void goby::acomms::MMDriver::process_receive(const NMEASentence& nmea)
{    
    // look at the talker front (talker id)
    switch(talker_id_map_[nmea.talker_id()])
    {
        // only process messages from CA (modem)
        case CA: case SN: global_fail_count_ = 0; break; // reset fail count - modem is alive!
            // ignore the rest
        case CC: case GP: default: return;
    }

    // look at the talker back (message code)
    switch(sentence_id_map_[nmea.sentence_id()])
    {
        //
        // local modem
        //
        case REV: rev(nmea); break; // software revision
        case ERR: err(nmea); break; // error message
        case CFG: cfg(nmea); break; // configuration
        case CLK: clk(nmea); break; // clock
        case DRQ: drq(nmea); break; // data request
            
        //
        // data cycle
        //
        case CYC:  // cycle init
        {
            protobuf::ModemDataInit m;
            cyc(nmea, &m);
            process_parsed(nmea, m.mutable_base());
            break;
        }
        
        
        case RXD:  // data receive
        {
            protobuf::ModemDataTransmission m;
            rxd(nmea, &m);
            process_parsed(nmea, m.mutable_base());
            break;
        }

        case ACK:  // acknowledge
        {
            protobuf::ModemDataAck m;
            ack(nmea, &m);
            process_parsed(nmea, m.mutable_base());
            break;
        }

        //
        // ranging
        //
        case MPR:  // ping response
        {
            protobuf::ModemRangingReply m;
            mpr(nmea, &m);
            process_parsed(nmea, m.mutable_base());
            break;
        }
        
        case TTA: // remus lbl times
        {
            protobuf::ModemRangingReply m;
            tta(nmea, &m);
            process_parsed(nmea, m.mutable_base());
            break; 
        }
        
        default: break;
    }

    // clear the last send given modem acknowledgement
    if(!out_.empty() && out_.front().sentence_id() == nmea.sentence_id())
        pop_out();    
}

void goby::acomms::MMDriver::ack(const NMEASentence& nmea, protobuf::ModemDataAck* m)
{
    set_time(m->mutable_base());
    
    m->mutable_base()->set_src(as<uint32>(nmea[1]));
    m->mutable_base()->set_dest(as<uint32>(nmea[2]));
    // WHOI counts starting at 1, Goby counts starting at 0
    m->set_frame(as<uint32>(nmea[3])-1);
    
    if(callback_ack) callback_ack(*m);
}
void goby::acomms::MMDriver::drq(const NMEASentence& nmea_in)
{    
    NMEASentence nmea_out("$CCTXD", NMEASentence::IGNORE);

    // use the cached data
    if(!cached_data_msgs_.empty())
    {
        protobuf::ModemDataTransmission& data_msg = cached_data_msgs_.front();    
        nmea_out.push_back(data_msg.base().src());
        nmea_out.push_back(data_msg.base().dest());
        nmea_out.push_back(int(data_msg.ack_requested()));
        nmea_out.push_back(hex_encode(data_msg.data()));
        cached_data_msgs_.pop_front();
    }
    else // send a blank message to stop further DRQs
    {
        // $CCTXD,SRC,DEST,ACK bit, Hex encoded data message
        // $CADRQ,Time,SRC,DEST,ACK Request Bit,Max # Bytes Requested,Frame Counter
        nmea_out.push_back(nmea_in.at(2)); 
        nmea_out.push_back(nmea_in.at(3));
        nmea_out.push_back(nmea_in.at(4));
        nmea_out.push_back("");
    }
    
    write(nmea_out);   
}

void goby::acomms::MMDriver::rxd(const NMEASentence& nmea, protobuf::ModemDataTransmission* m)
{
    set_time(m->mutable_base());
    m->mutable_base()->set_src(as<uint32>(nmea[1]));
    m->mutable_base()->set_dest(as<uint32>(nmea[2]));
    m->set_ack_requested(as<bool>(nmea[3]));
    // WHOI counts from 1, we count from 0
    m->set_frame(as<uint32>(nmea[4])-1);
    m->set_data(hex_decode(nmea[5]));

    if(callback_receive) callback_receive(*m);
}


void goby::acomms::MMDriver::cfg(const NMEASentence& nmea)
{
    nvram_cfg_[nmea[1]] = nmea.as<int>(2);
    
    if(out_.empty() ||
       (out_.front().sentence_id() != "CFG" && out_.front().sentence_id() != "CFQ"))
        return;
    
    if(out_.front().sentence_id() == "CFQ") pop_out();
}

void goby::acomms::MMDriver::clk(const NMEASentence& nmea)
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

void goby::acomms::MMDriver::mpr(const NMEASentence& nmea, protobuf::ModemRangingReply* m)
{
    set_time(m->mutable_base());
    
    // $CAMPR,SRC,DEST,TRAVELTIME*CS
    m->mutable_base()->set_src(as<uint32>(nmea[1]));
    m->mutable_base()->set_dest(as<uint32>(nmea[2]));

    if(nmea.size() > 3)
        m->add_one_way_travel_time(as<double>(nmea[3]));

    m->set_type(protobuf::MODEM_RANGING);
    
    if(callback_range_reply) callback_range_reply(*m);
}

void goby::acomms::MMDriver::rev(const NMEASentence& nmea)
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
        ptime reported = nmea_time2ptime(nmea[1]);

        if(reported < (expected - ALLOWED_SKEW))
            clock_set_ = false;
    }
}

void goby::acomms::MMDriver::err(const NMEASentence& nmea)
{
    *log_ << group("mm_out") << warn << "modem reports error: " << nmea.message() << std::endl;
}

void goby::acomms::MMDriver::cyc(const NMEASentence& nmea, protobuf::ModemDataInit* init_msg)
{
    set_time(init_msg->mutable_base());

    // somewhat "loose" interpretation of some of the fields
    init_msg->mutable_base()->set_src(as<uint32>(nmea[2])); // ADR1
    init_msg->mutable_base()->set_dest(as<uint32>(nmea[3])); // ADR2
    init_msg->set_num_frames(as<uint32>(nmea[6])); // Npkts, number of packets

    if(log_) *log_ << *init_msg << std::endl;
    
    // if we're supposed to send and we didn't initiate the cycle
    if(!local_cccyc_)
        cache_outgoing_data(*init_msg);
    else // clear flag for next cycle
        local_cccyc_ = false;
}

void goby::acomms::MMDriver::cache_outgoing_data(const protobuf::ModemDataInit& init_msg)
{
    if(init_msg.base().src() == nvram_cfg_["SRC"])
    {
        if(!cached_data_msgs_.empty())
        {
            if(log_) *log_ << warn << "flushing " << cached_data_msgs_.size() << " messages that were never sent in response to a $CADRQ." << std::endl;
            cached_data_msgs_.clear();
        }
    
        static protobuf::ModemDataRequest request_msg;
        request_msg.Clear();
        // make a data request in anticipation that we will need to send
        set_time(request_msg.mutable_base());
    
        request_msg.mutable_base()->set_src(as<uint32>(init_msg.base().src()));
        request_msg.mutable_base()->set_dest(as<uint32>(init_msg.base().dest()));
        // nmea_in[4] == ack
        request_msg.set_max_bytes(PACKET_SIZE[init_msg.base().rate()]);
        static protobuf::ModemDataTransmission data_msg;
        for(unsigned i = 0, n = init_msg.num_frames(); i < n; ++i)
        {
            request_msg.set_frame(i);
            data_msg.Clear();
            // copy the request base over to the data base
            data_msg.mutable_base()->CopyFrom(request_msg.base());
            if(callback_data_request) callback_data_request(request_msg, &data_msg);
        
            // no more data to send
            if(data_msg.data().empty()) break;
        
            cached_data_msgs_.push_back(data_msg);
        }
    }
}


void goby::acomms::MMDriver::tta(const NMEASentence& nmea, protobuf::ModemRangingReply* m)
{

    m->add_one_way_travel_time(as<double>(nmea[1]));
    m->add_one_way_travel_time(as<double>(nmea[2]));
    m->add_one_way_travel_time(as<double>(nmea[3]));
    m->add_one_way_travel_time(as<double>(nmea[4]));
    
    m->set_type(protobuf::REMUS_LBL_RANGING);

    set_time(m->mutable_base(), nmea_time2ptime(nmea[5]));    
    m->mutable_base()->set_time_source(protobuf::ModemMsgBase::MODEM_TIME);
    
    if(callback_range_reply) callback_range_reply(*m);    
}

boost::posix_time::ptime goby::acomms::MMDriver::nmea_time2ptime(const std::string& mt)
{   
    using namespace boost::posix_time;
    using namespace boost::gregorian;

    // must be at least HHMMSS
    if(mt.length() < 6)
        return ptime(not_a_date_time);  
    else
    {
        std::string s_hour = mt.substr(0,2), s_min = mt.substr(2,2), s_sec = mt.substr(4,2), s_fs = "0";

        // has some fractional seconds
        if(mt.length() > 7)
            s_fs = mt.substr(7); // everything after the "."
	        
        try
        {
            int hour = boost::lexical_cast<int>(s_hour);
            int min = boost::lexical_cast<int>(s_min);
            int sec = boost::lexical_cast<int>(s_sec);
            int micro_sec = boost::lexical_cast<int>(s_fs)*pow(10, 6-s_fs.size());
            
            return (ptime(date(day_clock::universal_day()), time_duration(hour, min, sec, 0)) + microseconds(micro_sec));
        }
        catch (boost::bad_lexical_cast&)
        {
            return ptime(not_a_date_time);
        }        
    }
}

void goby::acomms::MMDriver::process_parsed(const util::NMEASentence& nmea, protobuf::ModemMsgBase* m)
{
    m->set_raw(nmea.message());
    if(callback_all_incoming) callback_all_incoming(*m);
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

