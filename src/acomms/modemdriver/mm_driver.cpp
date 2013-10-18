// Copyright 2009-2013 Toby Schneider (https://launchpad.net/~tes)
//                     Massachusetts Institute of Technology (2007-)
//                     Woods Hole Oceanographic Institution (2007-)
//                     Goby Developers Team (https://launchpad.net/~goby-dev)
// 
//
// This file is part of the Goby Underwater Autonomy Project Libraries
// ("The Goby Libraries").
//
// The Goby Libraries are free software: you can redistribute them and/or modify
// them under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// The Goby Libraries are distributed in the hope that they will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with Goby.  If not, see <http://www.gnu.org/licenses/>.


#include <sstream>

#include <boost/foreach.hpp>
#include <boost/assign.hpp>
#include <boost/algorithm/string.hpp>

#include "goby/common/logger.h"
#include "goby/util/binary.h"

#include "mm_driver.h"
#include "driver_exception.h"

using goby::util::NMEASentence;
using goby::common::goby_time;
using goby::util::hex_encode;
using goby::util::hex_decode;
using goby::common::ptime2unix_double;
using goby::util::as;
using goby::glog;
using google::protobuf::uint32;
using namespace goby::common::tcolor;
using namespace goby::common::logger;
using namespace goby::common::logger_lock;
using goby::common::nmea_time2ptime;

const boost::posix_time::time_duration goby::acomms::MMDriver::MODEM_WAIT = boost::posix_time::seconds(5);
const boost::posix_time::time_duration goby::acomms::MMDriver::WAIT_AFTER_REBOOT = boost::posix_time::seconds(2);
const int goby::acomms::MMDriver::ALLOWED_MS_DIFF = 3000;
const boost::posix_time::time_duration goby::acomms::MMDriver::HYDROID_GATEWAY_GPS_REQUEST_INTERVAL = boost::posix_time::seconds(30);
const std::string goby::acomms::MMDriver::SERIAL_DELIMITER = "\r";
const unsigned goby::acomms::MMDriver::PACKET_FRAME_COUNT [] = { 1, 3, 3, 2, 2, 8 };
const unsigned goby::acomms::MMDriver::PACKET_SIZE [] = { 32, 64, 64, 256, 256, 256 };


//
// INITIALIZATION
//

goby::acomms::MMDriver::MMDriver()
    : last_write_time_(goby_time()),
      waiting_for_modem_(false),
      startup_done_(false),
      global_fail_count_(0),
      present_fail_count_(0),
      clock_set_(false),
      last_hydroid_gateway_gps_request_(goby_time()),
      is_hydroid_gateway_(false),
      expected_remaining_caxst_(0),
      expected_remaining_cacst_(0),
      local_cccyc_(false)
{
    initialize_talkers();
}

void goby::acomms::MMDriver::startup(const protobuf::DriverConfig& cfg)
{
    glog.is(DEBUG1, lock) && glog << group(glog_out_group()) << "Goby Micro-Modem driver starting up." << std::endl << unlock;

    if(startup_done_)
    {
        glog.is(DEBUG1, lock) && glog << group(glog_out_group()) << " ... driver is already started, not restarting." << std::endl << unlock;
        return;
    }

    // store a copy for us later
    driver_cfg_ = cfg;
    
    if(!cfg.has_line_delimiter())
        driver_cfg_.set_line_delimiter(SERIAL_DELIMITER);

    if(!cfg.has_serial_baud())
        driver_cfg_.set_serial_baud(DEFAULT_BAUD);

    // support the non-standard Hydroid gateway buoy
    if(driver_cfg_.HasExtension(micromodem::protobuf::Config::hydroid_gateway_id))
        set_hydroid_gateway_prefix(driver_cfg_.GetExtension(micromodem::protobuf::Config::hydroid_gateway_id));

    
    modem_start(driver_cfg_);

    write_cfg();
    
    // so that we know what the Micro-Modem has for all the NVRAM values, not just the ones we set
    query_all_cfg();

    while(!out_.empty())
    {
        do_work();
        usleep(100000); // 10 Hz
    }
    
    // some clock stuff -- set clk_mode_ zeros for starters
    set_clock();
    clk_mode_ = micromodem::protobuf::NO_SYNC_TO_PPS_AND_CCCLK_BAD;
    
    while(!clock_set_)
    {
        do_work();
        usleep(100000); // 10 Hz
    }
    
    startup_done_ = true;
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
        ("ERR",ERR)("TOA",TOA)("XST",XST);

    boost::assign::insert (talker_id_map_)
        ("CC",CC)("CA",CA)("SN",SN)("GP",GP); 

    // from Micro-Modem Software Interface Guide v. 3.04
    boost::assign::insert (description_map_)
        ("$CAACK","Acknowledgment of a transmitted packet")
        ("$CADRQ","Data request message, modem to host")
        ("$CARXA","Received ASCII message, modem to host")
        ("$CARXD","Received binary message, modem to host")
        ("$CARXP","Incoming packet detected, modem to host")
        ("$CCTXD","Transmit binary data message, host to modem")
        ("$CCTXA","Transmit ASCII data message, host to modem")
        ("$CATXD","Echo back of transmit binary data message")
        ("$CATXA","Echo back of transmit ASCII data message")
        ("$CATXP","Start of packet transmission, modem to host")
        ("$CATXF","End of packet transmission, modem to host")
        ("$CCCYC","Network Cycle Initialization Command")
        ("$CACYC","Echo of Network Cycle Initialization command")
        ("$CCMPC","Mini-Packet Ping command, host to modem")
        ("$CAMPC","Echo of Ping command, modem to host")
        ("$CAMPA","A Ping has been received, modem to host")
        ("$CAMPR","Reply to Ping has been received, modem to host")
        ("$CCRSP","Pinging with an FM sweep")
        ("$CARSP","Respose to FM sweep ping command")
        ("$CCMSC","Sleep command, host to modem")
        ("$CAMSC","Echo of Sleep command, modem to host")
        ("$CAMSA","A Sleep was received acoustically, modem to host")
        ("$CAMSR","A Sleep reply was received, modem to host")
        ("$CCEXL","External hardware control command, local modem only")
        ("$CCMEC","External hardware control command, host to modem")
        ("$CAMEC","Echo of hardware control command, modem to host")
        ("$CAMEA","Hardware control command received acoustically")
        ("$CAMER","Hardware control command reply received")
        ("$CCMUC","User Mini-Packet command, host to modem")
        ("$CAMUC","Echo of user Mini-Packet, modem to host")
        ("$CAMUA","Mini-Packet received acoustically, modem to host")
        ("$CAMUR","Reply to Mini-Packet received, modem to host")
        ("$CCPDT","Ping REMUS digital transponder, host to modem")
        ("$CCPNT","Ping narrowband transponder, host to modem")
        ("$SNTTA","Transponder travel times, modem to host")
        ("$SNMFD","Nav matched filter information, modem to host")
        ("$CCCLK","Set clock, host to modem")
        ("$CCCFG","Set NVRAM configuration parameter, host to modem")
        ("$CACFG","Echo of NVRAM configuration parameter, modem to host")
        ("$CCCFQ","Query configuration parameter, host to modem")
        ("$CCAGC","Set automatic gain control")
        ("$CABBD","Dump of baseband data to serial port, modem to host")
        ("$CCCFR","Measure noise level at receiver, host to modem")
        ("$SNCFR","Noise report, modem to host")
        ("$CACST","Communication cycle receive statistics")
        ("$CAXST","Communication cycle transmit statistics")
        ("$CAMSG","Transaction message, modem to host")
        ("$CAREV","Software revision message, modem to host")
        ("$CADQF","Data quality factor information, modem to host")
        ("$CASHF","Shift information, modem to host")
        ("$CAMFD","Comms matched filter information, modem to host")
        ("$CACLK","Time/Date message, modem to host")
        ("$CASNR","SNR statistics on the incoming PSK packet")
        ("$CADOP","Doppler speed message, modem to host")
        ("$CADBG","Low level debug message, modem to host")
        ("$CAERR","Error message, modem to host")
        ("$CATOA","Message from modem to host reporting time of arrival of the previous packet, and the synchronous timing mode used to determine that time.");

    // from Micro-Modem Software Interface Guide v. 3.04
    boost::assign::insert (cfg_map_)
        ("AGC","Turn on automatic gain control")
        ("AGN","Analog Gain (50 is 6 dB, 250 is 30 dB)")
        ("ASD","Always Send Data. Tells the modem to send test data when the user does not provide any.")
        ("BBD","PSK Baseband data dump to serial port")
        ("BND","Frequency Bank (1, 2, 3 for band A, B, or C, 0 for user-defined PSK only band)")
        ("BR1","Baud rate for serial port 1 (3 = 19200)")
        ("BR2","Baud rate for serial port 2 (3 = 19200)")
        ("BRN","Run bootloader at next revert")
        ("BSP","Boot loader serial port")
        ("BW0","Bandwidth for Band 0 PSK CPR 0-1 Coprocessor power toggle switch 1")
        ("CRL","Cycle init reverb lockout (ms) 50")
        ("CST","Cycle statistics message 1")
        ("CTO","Cycle init timeout (sec) 10")
        ("DBG","Enable low-level debug messages 0")
        ("DGM","Diagnostic messaging 0")
        ("DOP","Whether or not to send the $CADOP message")
        ("DQF","Whether or not to send the $CADQF message")
        ("DTH","Matched filter signal threshold, FSK")
        ("DTO","Data request timeout (sec)")
        ("DTP","Matched filter signal threshold, PSK")
        ("ECD","Int Delay at end of cycle (ms)")
        ("EFF","Feedforward taps for the LMS equalizer")
        ("EFB","Feedback taps for the LMS equalizer")
        ("FMD","PSK FM probe direction,0 up, 1 down")
        ("FML","PSK FM probe length, symbols")
        ("FC0","Carrier at Band 0 PSK only")
        ("GPS","GPS parser on aux. serial port")
        ("HFC","Hardware flow control on main serial port")
        ("MCM","Enable current mode hydrophone power supply on Rev. C Multi-Channel Analog Board. Must be set to 1 for Rev. B Multi-Channel Analog Board.")
        ("MFD","Whether or not to send the MFD messages")
        ("IRE","Print impulse response of FM sweep")
        ("MFC","MFD calibration value (samples)")
        ("MFD","Whether or not to send the MFD messages")
        ("MOD","0 sends FSK minipacket, 1 sends PSK minipacket")
        ("MPR","Enable power toggling on Multi-Channel Analog Board")
        ("MSE","Print symbol mean squared error (dB) from the LMS equalizer")
        ("MVM","Enable voltage mode hydrophone power supply on Multi-Channel Analog Board")
        ("NDT","Detect threshold for nav detector") 
        ("NPT","Power threshold for nav detector")
        ("NRL","Navigation reverb lockout (ms)")
        ("NRV","Number of CTOs before hard reboot")
        ("PAD","Power-amp delay (ms)")
        ("PCM","Passband channel mask")
        ("POW","Detection power threshold (dB) PRL Int Packet reverb lockout (ms)")
        ("PTH","Matched filter detector power threshold")
        ("PTO","Packet timeout (sec)")
        ("REV","Whether or not to send the $CAREV message")
        ("SGP","Show GPS messages on main serial port")
        ("RXA","Whether or not to send the $CARXA message")
        ("RXD","Whether or not to send the $CARXD message")
        ("RXP","Whether or not to send the $CARXP message") 
        ("SCG","Set clock from GPS")
        ("SHF","Whether or not to send the $CASHF message")
        ("SNR","Turn on SNR stats for PSK comms")
        ("SNV","Synchronous transmission of packets")
        ("SRC","Default Source Address")
        ("TAT","Navigation turn-around-time (msec)")
        ("TOA","Display time of arrival of a packet (sec)")
        ("TXD","Delay before transmit (ms)")
        ("TXP","Turn on start of transmit message")
        ("TXF","Turn on end of transmit message")
        ("XST","Turn on transmit stats message, CAXST");    
    
    
}

void goby::acomms::MMDriver::set_hydroid_gateway_prefix(int id)
{
    is_hydroid_gateway_ = true;
    // If the buoy is in use, make the prefix #M<ID>
    hydroid_gateway_gps_request_ = "#G" + as<std::string>(id) + "\r\n";        
    hydroid_gateway_modem_prefix_ = "#M" + as<std::string>(id);
    
    glog.is(DEBUG1, lock) && glog << group(glog_out_group()) << "Setting the hydroid_gateway buoy prefix: out=" << hydroid_gateway_modem_prefix_ << std::endl << unlock;
}


void goby::acomms::MMDriver::set_clock()
{
    glog.is(DEBUG1, lock) && glog << group(glog_out_group()) << "Setting the Micro-Modem clock." << std::endl << unlock;

    NMEASentence nmea("$CCCLK", NMEASentence::IGNORE);
    boost::posix_time::ptime p = goby_time();

    // for sync nav, let's make sure to send the ccclk at the beginning of the
    // second: between 1ms-50ms after the top of the second
    // see WHOI sync nav manual
    // http://acomms.whoi.edu/documents/Synchronous%20Navigation%20With%20MicroModem%20RevD.pdf
    double frac_sec = double(p.time_of_day().fractional_seconds())/p.time_of_day().ticks_per_second();
    while(frac_sec > 50e-3 || frac_sec < 1e-3)
    {
        // wait 1 ms
        usleep(1000);
        p = goby_time();
        frac_sec = double(p.time_of_day().fractional_seconds())/p.time_of_day().ticks_per_second();
    }
    
    nmea.push_back(int(p.date().year()));
    nmea.push_back(int(p.date().month()));
    nmea.push_back(int(p.date().day()));
    nmea.push_back(int(p.time_of_day().hours()));
    nmea.push_back(int(p.time_of_day().minutes()));
    nmea.push_back(int(p.time_of_day().seconds()));
    
    append_to_write_queue(nmea);

    // take a breath to let the clock be set 
    sleep(1);
}

void goby::acomms::MMDriver::write_cfg()
{
    // reset nvram if requested and not a Hydroid buoy
    // as this resets the baud to 19200 and the buoy
    // requires 4800
    if(!is_hydroid_gateway_ && driver_cfg_.GetExtension(micromodem::protobuf::Config::reset_nvram))
        write_single_cfg("ALL,0");

    // try to enforce CST to be enabled
    write_single_cfg("CST,1");
    
    
    for(int i = 0, n = driver_cfg_.ExtensionSize(micromodem::protobuf::Config::nvram_cfg); i < n; ++i)
        write_single_cfg(driver_cfg_.GetExtension(micromodem::protobuf::Config::nvram_cfg, i));

    // enforce SRC to be set the same as provided modem id. we need this for sanity...
    write_single_cfg("SRC," + as<std::string>(driver_cfg_.modem_id()));
    
    // enforce REV to be enabled, we use it for current time and detecting modem reboots
    write_single_cfg("REV,1");

    // enforce RXP to be enabled, we use it to detect start of received message
    write_single_cfg("RXP,1");


}

void goby::acomms::MMDriver::write_single_cfg(const std::string &s)
{
    NMEASentence nmea("$CCCFG", NMEASentence::IGNORE);        

    // old three letter cfg (always upper case)
    const std::string::size_type MM1_CFG_LENGTH = 3;
    if(s.find(',') == MM1_CFG_LENGTH)
         nmea.push_back(boost::to_upper_copy(s));
    else // new config 
         nmea.push_back(s);
    
    append_to_write_queue(nmea);
}



void goby::acomms::MMDriver::query_all_cfg()
{
    NMEASentence nmea("$CCCFQ,ALL", NMEASentence::IGNORE);
    append_to_write_queue(nmea);
}


//
// SHUTDOWN
//


void goby::acomms::MMDriver::shutdown()
{
    startup_done_ = false;
    modem_close();
}

goby::acomms::MMDriver::~MMDriver()
{ }


//
// LOOP
//

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
    while(modem_read(&in))
    {
        boost::trim(in);
	// Check for whether the hydroid_gateway buoy is being used and if so, remove the prefix
	if (is_hydroid_gateway_) in.erase(0, HYDROID_GATEWAY_PREFIX_LENGTH);

        // try to handle the received message, posting appropriate signals
        try
        {
            NMEASentence nmea(in, NMEASentence::VALIDATE);
            process_receive(nmea);
        }
        catch(std::exception& e)
        {
            glog.is(DEBUG1, lock) && glog << group(glog_in_group()) << warn << "Failed to handle message: " << e.what() << std::endl << unlock;
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

//
// HANDLE MAC SIGNALS
//

void goby::acomms::MMDriver::handle_initiate_transmission(const protobuf::ModemTransmission& msg)
{
    transmit_msg_.CopyFrom(msg);    
    
    try
    {
        glog.is(DEBUG1, lock) && glog << group(glog_out_group()) << "Beginning to initiate transmission." << std::endl << unlock;

        // allows zero to N third parties modify the transmission before sending.
        signal_modify_transmission(&transmit_msg_);        
        
        switch(transmit_msg_.type())
        {
            case protobuf::ModemTransmission::DATA: cccyc(&transmit_msg_); break;
            case protobuf::ModemTransmission::DRIVER_SPECIFIC:
            {
                switch(transmit_msg_.GetExtension(micromodem::protobuf::type))
                {
                    case micromodem::protobuf::MICROMODEM_MINI_DATA: ccmuc(&transmit_msg_); break;
                    case micromodem::protobuf::MICROMODEM_TWO_WAY_PING: ccmpc(transmit_msg_); break;
                    case micromodem::protobuf::MICROMODEM_REMUS_LBL_RANGING: ccpdt(transmit_msg_); break;
                    case micromodem::protobuf::MICROMODEM_NARROWBAND_LBL_RANGING: ccpnt(transmit_msg_); break;
                    default:
                        glog.is(DEBUG1, lock) && glog << group(glog_out_group()) << warn << "Not initiating transmission because we were given an invalid DRIVER_SPECIFIC transmission type for the Micro-Modem:" << transmit_msg_ << std::endl << unlock;
                        break;
                }
            }
            break;

            default:
                glog.is(DEBUG1, lock) && glog << group(glog_out_group()) << warn << "Not initiating transmission because we were given an invalid transmission type for the base Driver:" << transmit_msg_ << std::endl << unlock;
                break;
            
        }
    }

    catch(ModemDriverException& e)
    {
        glog.is(DEBUG1, lock) && glog << group(glog_out_group()) << warn << "Failed to initiate transmission: " << e.what() << std::endl << unlock;
    }
    

}

void goby::acomms::MMDriver::cccyc(protobuf::ModemTransmission* msg)
{
    glog.is(DEBUG1, lock) && glog << group(glog_out_group()) << "\tthis is a DATA transmission" << std::endl << unlock;

    // we initiated this cycle so don't grab data *again* on the CACYC (in cacyc()) 
    local_cccyc_ = true;
    
    msg->set_max_num_frames(PACKET_FRAME_COUNT[msg->rate()]);
    msg->set_max_frame_bytes(PACKET_SIZE[msg->rate()]);
        
    cache_outgoing_data(msg);
    
    // don't start a local cycle if we have no data
    const bool is_local_cycle = msg->src() == driver_cfg_.modem_id();
    if(!(is_local_cycle && (msg->frame_size() == 0 || msg->frame(0) == "")))
    {
        //$CCCYC,CMD,ADR1,ADR2,Packet Type,ACK,Npkt*CS
        NMEASentence nmea("$CCCYC", NMEASentence::IGNORE);
        nmea.push_back(0); // CMD: deprecated field
        nmea.push_back(msg->src()); // ADR1
        nmea.push_back(msg->dest()); // ADR2
        nmea.push_back(msg->rate()); // Packet Type (transmission rate)
        nmea.push_back(is_local_cycle
                       ? static_cast<int>(msg->ack_requested())
                       : 1); // ACK: deprecated field, but still dictates the value provided by CADRQ
        nmea.push_back(is_local_cycle ? msg->frame_size() : msg->max_num_frames()); // number of frames we want

        append_to_write_queue(nmea);

        // if rate 0 we have an extra transmission for the CCCYC
        expected_remaining_caxst_ = (msg->rate() == 0 && is_local_cycle) ? 1 : 0;
    }
    else
    {
        glog.is(DEBUG1, lock) && glog << group(glog_out_group()) << "Not initiating transmission because we have no data to send" << std::endl << unlock;
    }
}
    
void goby::acomms::MMDriver::ccmuc(protobuf::ModemTransmission* msg)
{
    glog.is(DEBUG1, lock) && glog << group(glog_out_group()) << "\tthis is a MICROMODEM_MINI_DATA transmission" << std::endl << unlock;

    const int MINI_NUM_FRAMES = 1;
    const int MINI_PACKET_SIZE = 2;
    msg->set_max_num_frames(MINI_NUM_FRAMES);
    msg->set_max_frame_bytes(MINI_PACKET_SIZE);

    cache_outgoing_data(msg);
        
    if(msg->frame_size() > 0 && msg->frame(0).size())
    {        
        glog.is(DEBUG1, lock) && glog << "Mini-data message: " << *msg << std::endl << unlock;
        msg->mutable_frame(0)->resize(MINI_PACKET_SIZE);
        glog.is(DEBUG1, lock) && glog << "Mini-data message after resize: " << *msg << std::endl << unlock;

        
        if((msg->frame(0)[0] & 0x1F) != msg->frame(0)[0])
        {
            glog.is(DEBUG1, lock) && glog << group(glog_out_group()) << warn << "MINI transmission can only be 13 bits; top three bits passed were *not* zeros, so discarding. You should AND your two bytes with 0x1FFF to get 13 bits" << std::endl << unlock;
            msg->mutable_frame(0)->at(0) &= 0x1F;
        }
            
        //$CCMUC,SRC,DEST,HHHH*CS
        NMEASentence nmea("$CCMUC", NMEASentence::IGNORE);
        nmea.push_back(msg->src()); // ADR1
        nmea.push_back(msg->dest()); // ADR2
        nmea.push_back(goby::util::hex_encode(msg->frame(0))); //HHHH 
        append_to_write_queue(nmea);
    }
    else
    {
        glog.is(DEBUG1, lock) && glog << group(glog_out_group()) << warn << "MINI transmission failed: no data provided" << std::endl << unlock;
    }
    

}
    
void goby::acomms::MMDriver::ccmpc(const protobuf::ModemTransmission& msg)
{
    glog.is(DEBUG1, lock) && glog << group(glog_out_group()) << "\tthis is a MICROMODEM_TWO_WAY_PING transmission" << std::endl << unlock;

    //$CCMPC,SRC,DEST*CS
    NMEASentence nmea("$CCMPC", NMEASentence::IGNORE);
    nmea.push_back(msg.src()); // ADR1
    nmea.push_back(msg.dest()); // ADR2
    
    append_to_write_queue(nmea);
}
    
void goby::acomms::MMDriver::ccpdt(const protobuf::ModemTransmission& msg)
{
    glog.is(DEBUG1, lock) && glog << group(glog_out_group()) << "\tthis is a MICROMODEM_REMUS_LBL_RANGING transmission" << std::endl << unlock;

    last_lbl_type_ = micromodem::protobuf::MICROMODEM_REMUS_LBL_RANGING;
    
    // start with configuration parameters
    micromodem::protobuf::REMUSLBLParams params =
        driver_cfg_.GetExtension(micromodem::protobuf::Config::remus_lbl);
    // merge (overwriting any duplicates) the parameters given in the request
    params.MergeFrom(msg.GetExtension(micromodem::protobuf::remus_lbl));
    
    uint32 tat = params.turnaround_ms();
    if(static_cast<unsigned>(nvram_cfg_["TAT"]) != tat)
        write_single_cfg("TAT," + as<std::string>(tat));

    // $CCPDT,GRP,CHANNEL,SF,STO,Timeout,AF,BF,CF,DF*CS
    NMEASentence nmea("$CCPDT", NMEASentence::IGNORE);
    nmea.push_back(1); // GRP 1 is the only group right now 
    nmea.push_back(msg.src() % 4 + 1); // can only use 1-4
    nmea.push_back(0); // synchronize may not work?
    nmea.push_back(0); // synchronize may not work?
    // REMUS LBL is 50 ms turn-around time, assume 1500 m/s speed of sound
    nmea.push_back(int((params.lbl_max_range()*2.0 / ROUGH_SPEED_OF_SOUND)*1000 + tat));
    nmea.push_back(params.enable_beacons() >> 0 & 1);
    nmea.push_back(params.enable_beacons() >> 1 & 1);
    nmea.push_back(params.enable_beacons() >> 2 & 1);
    nmea.push_back(params.enable_beacons() >> 3 & 1);            
    append_to_write_queue(nmea);
}
    
void goby::acomms::MMDriver::ccpnt(const protobuf::ModemTransmission& msg)
{
    glog.is(DEBUG1, lock) && glog << group(glog_out_group()) << "\tthis is a MICROMODEM_NARROWBAND_LBL_RANGING transmission" << std::endl << unlock;
    
    last_lbl_type_ = micromodem::protobuf::MICROMODEM_NARROWBAND_LBL_RANGING;

    // start with configuration parameters
    micromodem::protobuf::NarrowBandLBLParams params =
        driver_cfg_.GetExtension(micromodem::protobuf::Config::narrowband_lbl);
    // merge (overwriting any duplicates) the parameters given in the request
    params.MergeFrom(msg.GetExtension(micromodem::protobuf::narrowband_lbl));

    uint32 tat = params.turnaround_ms();
    if(static_cast<unsigned>(nvram_cfg_["TAT"]) != tat)
        write_single_cfg("TAT," + as<std::string>(tat));

    // $CCPNT, Ftx, Ttx, Trx, Timeout, FA, FB, FC, FD,Tflag*CS
    NMEASentence nmea("$CCPNT", NMEASentence::IGNORE);
    nmea.push_back(params.transmit_freq());
    nmea.push_back(params.transmit_ping_ms());
    nmea.push_back(params.receive_ping_ms());
    nmea.push_back(static_cast<int>((params.lbl_max_range()*2.0 / ROUGH_SPEED_OF_SOUND)*1000 + tat));

    // no more than four allowed
    const int MAX_NUMBER_RX_BEACONS = 4;
            
    int number_rx_freq_provided = std::min(MAX_NUMBER_RX_BEACONS, params.receive_freq_size());
            
    for(int i = 0, n = std::max(MAX_NUMBER_RX_BEACONS, number_rx_freq_provided); i < n; ++i)
    {
        if(i < number_rx_freq_provided)
            nmea.push_back(params.receive_freq(i));
        else
            nmea.push_back(0);
    }
            
    nmea.push_back(static_cast<int>(params.transmit_flag()));
    append_to_write_queue(nmea);
}
    

//
// OUTGOING NMEA
//

void goby::acomms::MMDriver::append_to_write_queue(const NMEASentence& nmea)
{
    out_.push_back(nmea);
    try_send(); // try to push it now without waiting for the next call to do_work();
}


void goby::acomms::MMDriver::try_send()
{
    if(out_.empty()) return;

    const util::NMEASentence& nmea = out_.front();
    
    bool resend = waiting_for_modem_ && (last_write_time_ <= (goby_time() - MODEM_WAIT));
    if(!waiting_for_modem_)
    {
        mm_write(nmea);
    }
    else if(resend)
    {
        glog.is(DEBUG1, lock) && glog << group(glog_out_group()) << "resending last command; no serial ack in " << (goby_time() - last_write_time_).total_seconds() << " second(s). " << std::endl << unlock;
        ++global_fail_count_;
        
        if(global_fail_count_ == MAX_FAILS_BEFORE_DEAD)
        {
            modem_close();
            throw(ModemDriverException("Micro-Modem appears to not be responding!"));
        }
        
        try
        {
            // try to increment the present (current NMEA sentence) fail counter
            // will throw if fail counter exceeds RETRIES
            increment_present_fail();
            // assuming we're still ok, write the line again
            mm_write(nmea);
        }
        catch(ModemDriverException& e)
        {
            present_fail_exceeds_retries();
        }
    }
}



void goby::acomms::MMDriver::mm_write(const util::NMEASentence& nmea)
{
    protobuf::ModemRaw raw_msg;
    raw_msg.set_raw(nmea.message());
    raw_msg.set_description(description_map_[nmea.front()]);

    glog.is(DEBUG1, lock) && glog << group(glog_out_group()) << hydroid_gateway_modem_prefix_
                            << raw_msg.raw() << "\n" << "^ "
                            << magenta << raw_msg.description() << nocolor << std::endl << unlock;
    
    signal_raw_outgoing(raw_msg);    
 
    modem_write(hydroid_gateway_modem_prefix_ + raw_msg.raw() + "\r\n");

    waiting_for_modem_ = true;
    last_write_time_ = goby_time();
}

void goby::acomms::MMDriver::increment_present_fail()
{
    ++present_fail_count_;
    if(present_fail_count_ >= RETRIES)
        throw(ModemDriverException("Fail count exceeds RETRIES"));
}

void goby::acomms::MMDriver::present_fail_exceeds_retries()
{
    
    glog.is(DEBUG1, lock) && glog << group(glog_out_group()) << warn << "Micro-Modem did not respond to our command even after " << RETRIES << " retries. continuing onwards anyway..." << std::endl << unlock;
    pop_out();    
}

void goby::acomms::MMDriver::pop_out()
{
    waiting_for_modem_ = false;

    if(!out_.empty())
        out_.pop_front();
    else
    {
        glog.is(DEBUG1, lock) && glog << group(glog_out_group()) << warn
                                << "Expected to pop outgoing NMEA message but out_ deque is empty" << std::endl << unlock;
    }
    
    present_fail_count_ = 0;
}


//
// INCOMING NMEA
//


void goby::acomms::MMDriver::process_receive(const NMEASentence& nmea)
{
    // need to print this first so raw log messages appear causal (as they are)
    protobuf::ModemRaw raw_msg;
    raw_msg.set_raw(nmea.message());
    raw_msg.set_description(description_map_[nmea.front()]);

    if(sentence_id_map_[nmea.sentence_id()] == CFG)
        *raw_msg.mutable_description() += ":  " + cfg_map_[nmea.at(1)];

    
    glog.is(DEBUG1, lock) && glog << group(glog_in_group()) << hydroid_gateway_modem_prefix_
                   << raw_msg.raw() << "\n" << "^ "
                   << magenta << raw_msg.description() << nocolor << std::endl << unlock;
    
    signal_raw_incoming(raw_msg);
    
    global_fail_count_ = 0;
    

    
    // look at the sentence id (last three characters of the NMEA 0183 talker)
    switch(sentence_id_map_[nmea.sentence_id()])
    {
        //
        // local modem
        //
        case REV: carev(nmea); break; // software revision
        case ERR: caerr(nmea); break; // error message
        case DRQ: cadrq(nmea, transmit_msg_); break; // data request
        case CFG: cacfg(nmea); break; // configuration
        case CLK: caclk(nmea); break; // clock

            
            //
            // data cycle
            //
        case CYC: cacyc(nmea, &transmit_msg_); break; // cycle init
        case XST: caxst(nmea, &transmit_msg_); break; // transmit stats for clock mode
        case RXD: carxd(nmea, &receive_msg_); break; // data receive
        case MSG: camsg(nmea, &receive_msg_); break; // for picking up BAD_CRC
        case CST: cacst(nmea, &receive_msg_); break; // transmit stats for clock mode
        case MUA: camua(nmea, &receive_msg_); break; // mini-packet receive
        case ACK: caack(nmea, &receive_msg_); break; // acknowledge

            //
            // ranging
            //
        case MPR: campr(nmea, &receive_msg_); break; // two way ping
        case MPA: campa(nmea, &receive_msg_); break; // hear request for two way ping 
        case TTA: sntta(nmea, &receive_msg_); break; // remus / narrowband lbl times 

        default: break;
    }

    
    // clear the last send given modem acknowledgement
    if(!out_.empty() && out_.front().sentence_id() == nmea.sentence_id())
        pop_out();    
}

void goby::acomms::MMDriver::caack(const NMEASentence& nmea, protobuf::ModemTransmission* m)
{
    // ACK has nothing to do with us!
    if(as<int32>(nmea[2]) != driver_cfg_.modem_id())
        return;
    
    // WHOI counts starting at 1, Goby counts starting at 0
    uint32 frame = as<uint32>(nmea[3])-1;
    
    if(frames_waiting_for_ack_.count(frame))
    {
        m->set_time(goby_time<uint64>());
        m->set_src(as<uint32>(nmea[1]));
        m->set_dest(as<uint32>(nmea[2]));
        m->set_type(protobuf::ModemTransmission::ACK);
        m->add_acked_frame(frame);

        frames_waiting_for_ack_.erase(frame);

        glog.is(DEBUG1, lock) && glog << group(glog_in_group()) << "Received ACK from " << m->src() << " for frame " << frame << std::endl << unlock;
        
        // if enabled cacst will signal_receive
        if(!nvram_cfg_["CST"])
            signal_receive_and_clear(m);
    }
    else
    {
        glog.is(DEBUG1, lock) && glog << group(glog_in_group()) << "Received ACK for Micro-Modem frame " << frame + 1 << " (Goby frame " << frame << ") that we were not expecting." << std::endl << unlock;
    }
}

void goby::acomms::MMDriver::cadrq(const NMEASentence& nmea_in, const protobuf::ModemTransmission& m)
{

    //$CADRQ,HHMMSS,SRC,DEST,ACK,N,F#*CS

    NMEASentence nmea_out("$CCTXD", NMEASentence::IGNORE);        

    // WHOI counts frames from 1, we count from 0
    // TEMPORARY MM2 BUG WORKAROUND (DRQ frame is 0, not 1)
    int frame = (driver_cfg_.GetExtension(micromodem::protobuf::Config::mm_version) == 2) ? as<int>(nmea_in[6]) : as<int>(nmea_in[6])-1;
    
    if(frame < m.frame_size() && !m.frame(frame).empty())
    {
        // use the cached data
        nmea_out.push_back(m.src());
        nmea_out.push_back(m.dest());
        nmea_out.push_back(int(m.ack_requested()));
        
//        int max_bytes = nmea_in.as<int>(5);
        
        nmea_out.push_back(hex_encode(m.frame(frame)));
        
        if(m.ack_requested())
            frames_waiting_for_ack_.insert(frame);
    }
    else
    {
        // send a blank message to supress further DRQ
        nmea_out.push_back(nmea_in[2]); // SRC
        nmea_out.push_back(nmea_in[3]); // DEST
        nmea_out.push_back(nmea_in[4]); // ACK
        nmea_out.push_back(""); // no data
    }
    append_to_write_queue(nmea_out);
}

void goby::acomms::MMDriver::camsg(const NMEASentence& nmea, protobuf::ModemTransmission* m)
{
    // CAMSG,BAD_CRC,4
    if(nmea.as<std::string>(1) == "BAD_CRC" &&
       !frames_waiting_to_receive_.empty()) // it's not a bad CRC on the CCCYC
    {
        m->AddExtension(micromodem::protobuf::frame_with_bad_crc, m->frame_size());
        // add a blank (placeholder) frame
        m->add_frame();
        // assume it's for the next frame
        frames_waiting_to_receive_.erase(frames_waiting_to_receive_.begin());

        // if enabled cacst will signal_receive, otherwise signal if this is the last frame
        if(frames_waiting_to_receive_.empty() && !nvram_cfg_["CST"])
            signal_receive_and_clear(m);

        glog.is(DEBUG1, lock) && glog << group(glog_in_group()) << warn << "Received message with bad CRC" << std::endl << unlock;
    }    
}

void goby::acomms::MMDriver::carxd(const NMEASentence& nmea, protobuf::ModemTransmission* m)
{
    // WHOI counts from 1, we count from 0
    unsigned frame = as<uint32>(nmea[4])-1;
    if(frame == 0)
    {
        m->set_time(goby_time<uint64>());
        m->set_src(as<uint32>(nmea[1]));
        m->set_dest(as<uint32>(nmea[2]));
        m->set_type(protobuf::ModemTransmission::DATA);
        m->set_ack_requested(as<bool>(nmea[3]));
    }

    if(!nmea[5].empty()) // don't add blank messages
    {
        if(static_cast<unsigned>(m->frame_size()) != frame)
        {
            glog.is(DEBUG1, lock) && glog << group(glog_out_group()) << warn << "frame count mismatch: (Micro-Modem reports): " << frame << ", (goby expects): " << m->frame_size() << std::endl << unlock;
        }

        m->add_frame(hex_decode(nmea[5]));
        glog.is(DEBUG1, lock) && glog << group(glog_in_group()) << "Received " << m->frame(m->frame_size()-1).size() << " byte DATA frame " << frame << " from " << m->src() << std::endl << unlock;

    }

    
    frames_waiting_to_receive_.erase(frame);
    
    // if enabled cacst will signal_receive, otherwise signal if this is the last frame
    if(frames_waiting_to_receive_.empty() && !nvram_cfg_["CST"])
        signal_receive_and_clear(m);
}


void goby::acomms::MMDriver::camua(const NMEASentence& nmea, protobuf::ModemTransmission* m)
{
//    m->Clear();

    m->set_time(goby_time<uint64>());
    m->set_src(as<uint32>(nmea[1]));
    m->set_dest(as<uint32>(nmea[2]));
    m->set_type(protobuf::ModemTransmission::DRIVER_SPECIFIC);
    m->SetExtension(micromodem::protobuf::type, micromodem::protobuf::MICROMODEM_MINI_DATA);
    
    m->add_frame(goby::util::hex_decode(nmea[3]));

    glog.is(DEBUG1, lock) && glog << group(glog_in_group()) << "Received MICROMODEM_MINI_DATA packet from " << m->src() << std::endl << unlock;

    // if enabled cacst will signal_receive
    if(!nvram_cfg_["CST"])
        signal_receive_and_clear(m);
}

void goby::acomms::MMDriver::cacfg(const NMEASentence& nmea)
{
    nvram_cfg_[nmea[1]] = nmea.as<int>(2);

    
    if(out_.empty() ||
       (out_.front().sentence_id() != "CFG" && out_.front().sentence_id() != "CFQ"))
        return;
    
    if(out_.front().sentence_id() == "CFQ") pop_out();
}

void goby::acomms::MMDriver::caclk(const NMEASentence& nmea)
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
                                         nmea.as<int>(6)+1,
                                         0));
    glog.is(DEBUG1, lock) && glog << group(glog_in_group()) << "Micro-Modem reported time: " << reported << std::endl << unlock;
    

    
    // make sure the modem reports its time as set at the right time
    // we may end up oversetting the clock, but better safe than sorry...
    boost::posix_time::time_duration t_diff = (reported - expected);
    
    if( abs( int( t_diff.total_milliseconds())) < ALLOWED_MS_DIFF )
    {
        glog.is(DEBUG1, lock) && glog << group(glog_out_group()) << "Micro-Modem clock acceptably set." << std::endl << unlock;        
        clock_set_ = true;
    }
    else
    {
        glog.is(DEBUG1, lock) && glog << group(glog_out_group()) << "Time is not within allowed skew, setting Micro-Modem clock again." << std::endl << unlock;
    }
}

void goby::acomms::MMDriver::caxst(const NMEASentence& nmea, protobuf::ModemTransmission* m)
{
    micromodem::protobuf::TransmitStatistics* xst = m->AddExtension(micromodem::protobuf::transmit_stat);

    // old XST has date as first field, and we'll assume all dates are
    // greater than UNIX epoch
    xst->set_version(nmea.as<int>(1) > 19700000 ? 0 : nmea.as<int>(1));

    try
    {
        
        int version_offset = 0; // offset in NMEA field number
        if(xst->version() == 0)
        {
            version_offset = 0;    
        }
        else if(xst->version() == 6)
        {
            version_offset = 1;
        }
    
        xst->set_date(nmea.as<std::string>(1 + version_offset));
        xst->set_time(nmea.as<std::string>(2 + version_offset));

        micromodem::protobuf::ClockMode clock_mode =
            micromodem::protobuf::ClockMode_IsValid(nmea.as<int>(3+version_offset)) ?
            nmea.as<micromodem::protobuf::ClockMode>(3+version_offset) :
            micromodem::protobuf::INVALID_CLOCK_MODE;
        
        xst->set_clock_mode(clock_mode);

        micromodem::protobuf::TransmitMode transmit_mode =
            micromodem::protobuf::TransmitMode_IsValid(nmea.as<int>(4+version_offset)) ?
            nmea.as<micromodem::protobuf::TransmitMode>(4+version_offset) :
            micromodem::protobuf::INVALID_TRANSMIT_MODE;
        
        xst->set_mode(transmit_mode);

        // TEMPORARY MM2 BUG WORKAROUND
        if(driver_cfg_.GetExtension(micromodem::protobuf::Config::mm_version) == 2)
            version_offset = 0;
        
        xst->set_probe_length(nmea.as<int32>(5 + version_offset));
        xst->set_bandwidth(nmea.as<int32>(6 + version_offset));
        xst->set_carrier_freq(nmea.as<int32>(7 + version_offset));
        xst->set_rate(nmea.as<int32>(8 + version_offset));
        xst->set_source(nmea.as<int32>(9 + version_offset));
        xst->set_dest(nmea.as<int32>(10 + version_offset));
        xst->set_ack_requested(nmea.as<bool>(11 + version_offset));
        xst->set_number_frames_expected(nmea.as<int32>(12 + version_offset));
        xst->set_number_frames_sent(nmea.as<int32>(13 + version_offset));

        micromodem::protobuf::PacketType packet_type =
            micromodem::protobuf::PacketType_IsValid(nmea.as<int>(14+version_offset)) ?
            nmea.as<micromodem::protobuf::PacketType>(14+version_offset) :
            micromodem::protobuf::PACKET_TYPE_UNKNOWN;

        xst->set_packet_type(packet_type);
        xst->set_number_bytes(nmea.as<int32>(15 + version_offset));
    
        clk_mode_ = xst->clock_mode();
    }
    catch(std::out_of_range& e) // thrown by std::vector::at() called by NMEASentence::as()
    {
        glog.is(DEBUG1, lock) && glog << group(glog_in_group()) << warn << "$CAXST message shorter than expected" << std::endl << unlock;
    }    
    
    if(expected_remaining_caxst_ == 0)
    {
        signal_transmit_result(*m);
        m->Clear();
    }
    else
    {
        --expected_remaining_caxst_;
    }
}


void goby::acomms::MMDriver::campr(const NMEASentence& nmea, protobuf::ModemTransmission* m)
{
    m->set_time(goby_time<uint64>());
    
    // $CAMPR,SRC,DEST,TRAVELTIME*CS
    // reverse src and dest so they match the original request
    m->set_src(as<uint32>(nmea[1]));
    m->set_dest(as<uint32>(nmea[2]));

    micromodem::protobuf::RangingReply* ranging_reply = m->MutableExtension(micromodem::protobuf::ranging_reply);
    
    if(nmea.size() > 3)
        ranging_reply->add_one_way_travel_time(as<double>(nmea[3]));

    m->set_type(protobuf::ModemTransmission::DRIVER_SPECIFIC);
    m->SetExtension(micromodem::protobuf::type, micromodem::protobuf::MICROMODEM_TWO_WAY_PING);

    glog.is(DEBUG1, lock) && glog << group(glog_in_group()) << "Received MICROMODEM_TWO_WAY_PING response from " << m->src() << ", 1-way travel time: " << ranging_reply->one_way_travel_time(ranging_reply->one_way_travel_time_size()-1) << "s" << std::endl << unlock;
    
    // if enabled cacst will signal_receive
    if(!nvram_cfg_["CST"])
        signal_receive_and_clear(m);
}

void goby::acomms::MMDriver::campa(const NMEASentence& nmea, protobuf::ModemTransmission* m)
{
    m->set_time(goby_time<uint64>());
    
    // $CAMPR,SRC,DEST*CS
    m->set_src(as<uint32>(nmea[1]));
    m->set_dest(as<uint32>(nmea[2]));

    m->set_type(protobuf::ModemTransmission::DRIVER_SPECIFIC);
    m->SetExtension(micromodem::protobuf::type, micromodem::protobuf::MICROMODEM_TWO_WAY_PING);
    
    // if enabled cacst will signal_receive
    if(!nvram_cfg_["CST"])
        signal_receive_and_clear(m);
}


void goby::acomms::MMDriver::sntta(const NMEASentence& nmea, protobuf::ModemTransmission* m)
{
//    m->Clear();    

    micromodem::protobuf::RangingReply* ranging_reply = m->MutableExtension(micromodem::protobuf::ranging_reply);

    ranging_reply->add_one_way_travel_time(as<double>(nmea[1]));
    ranging_reply->add_one_way_travel_time(as<double>(nmea[2]));
    ranging_reply->add_one_way_travel_time(as<double>(nmea[3]));
    ranging_reply->add_one_way_travel_time(as<double>(nmea[4]));
    
    m->set_type(protobuf::ModemTransmission::DRIVER_SPECIFIC);
    m->SetExtension(micromodem::protobuf::type, last_lbl_type_);

    
    m->set_src(driver_cfg_.modem_id());
    m->set_time(as<uint64>(nmea_time2ptime(nmea[5])));
    m->set_time_source(protobuf::ModemTransmission::MODEM_TIME);

    if(last_lbl_type_ == micromodem::protobuf::MICROMODEM_REMUS_LBL_RANGING)
        glog.is(DEBUG1, lock) && glog << group(glog_in_group()) << "Received MICROMODEM_REMUS_LBL_RANGING response " << std::endl << unlock;
    else if(last_lbl_type_ == micromodem::protobuf::MICROMODEM_NARROWBAND_LBL_RANGING)
        glog.is(DEBUG1, lock) && glog << group(glog_in_group()) << "Received MICROMODEM_NARROWBAND_LBL_RANGING response " << std::endl << unlock; 
   
    // no cacst on sntta, so signal receive here
    signal_receive_and_clear(m);
}

void goby::acomms::MMDriver::carev(const NMEASentence& nmea)
{
    if(nmea[2] == "INIT")
    {
        glog.is(DEBUG1, lock) && glog << group(glog_in_group()) << "Micro-Modem rebooted." << std::endl << unlock; 
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

        boost::posix_time::time_duration t_diff = (reported - expected);
        
        if( abs( int( t_diff.total_milliseconds())) > ALLOWED_MS_DIFF )
            clock_set_ = false;
    }
}

void goby::acomms::MMDriver::caerr(const NMEASentence& nmea)
{
    glog.is(DEBUG1, lock) && glog << group(glog_out_group()) << warn << "Micro-Modem reports error: " << nmea.message() << std::endl << unlock;

    
    // recover quicker if old firmware does not understand one of our commands
    if(nmea.at(2) == "NMEA")
    {
        waiting_for_modem_ = false;

        try
        {
            increment_present_fail(); 
        }
        catch(ModemDriverException& e)
        {
            present_fail_exceeds_retries();
        }
    }
}



void goby::acomms::MMDriver::cacyc(const NMEASentence& nmea, protobuf::ModemTransmission* msg)
{
    // we're sending
    if(as<int32>(nmea[2]) == driver_cfg_.modem_id())
    {
        // handle a third-party CYC
        if(!local_cccyc_)
        {
            // we have to clear the message if XST isn't set since
            // otherwise the transmit_msg was never cleared (or copied from initiate_transmission)
            if(!nvram_cfg_["XST"])
                msg->Clear();
            
            msg->set_time(goby_time<uint64>());

            msg->set_src(as<uint32>(nmea[2])); // ADR1
            msg->set_dest(as<uint32>(nmea[3])); // ADR2
            msg->set_rate(as<uint32>(nmea[4])); // Rate
            msg->set_max_num_frames(as<uint32>(nmea[6])); // Npkts, number of packets
            msg->set_max_frame_bytes(PACKET_SIZE[msg->rate()]);

            cache_outgoing_data(msg);
        }
        else  // clear flag for next cycle
        {
            local_cccyc_ = false;
        }
    }
    // we're receiving
    else
    {
        unsigned rate = as<uint32>(nmea[4]);
        if(local_cccyc_ && rate != 0) // clear flag for next cycle
        {            
            // if we poll for rates > 0, we get *two* CACYC - the one from the poll and the one from the message
            // thus, ignore the first of these
            local_cccyc_ = false;
            return;
        }       
        
        unsigned num_frames = as<uint32>(nmea[6]);
        if(!frames_waiting_to_receive_.empty())
        {
            glog.is(DEBUG1, lock) && glog << group(glog_out_group()) << warn << "flushing " << frames_waiting_to_receive_.size() << " expected frames that were never received." << std::endl << unlock;
            frames_waiting_to_receive_.clear();
        }

        for(unsigned i = 0; i < num_frames; ++i)
            frames_waiting_to_receive_.insert(i);
        
        // if rate 0 and we didn't send the CCCYC, we have two cacsts (one for the cacyc and one for the carxd)
        // otherwise, we have one
        expected_remaining_cacst_ = (as<int32>(nmea[4]) == 0 && !local_cccyc_) ? 1 : 0;

        local_cccyc_ = false;
    }
}

void goby::acomms::MMDriver::cache_outgoing_data(protobuf::ModemTransmission* msg)
{
    if(msg->src() == driver_cfg_.modem_id())
    {
        if(!frames_waiting_for_ack_.empty())
        {
            glog.is(DEBUG1, lock) && glog << group(glog_out_group()) << warn << "flushing " << frames_waiting_for_ack_.size() << " expected acknowledgments that were never received." << std::endl << unlock;
            frames_waiting_for_ack_.clear();
        }
        
        signal_data_request(msg);
    }
}

void goby::acomms::MMDriver::validate_transmission_start(const protobuf::ModemTransmission& message)
{
    if(message.src() < 0)
        throw("ModemTransmission::src is invalid: " + as<std::string>(message.src()));
    else if(message.dest() < 0)
        throw("ModemTransmission::dest is invalid: " + as<std::string>(message.dest()));
    else if(message.rate() < 0 || message.rate() > 5)
        throw("ModemTransmission::rate is invalid: " + as<std::string>(message.rate()));
    
}

void goby::acomms::MMDriver::cacst(const NMEASentence& nmea, protobuf::ModemTransmission* m)
{
    micromodem::protobuf::ReceiveStatistics* cst = m->AddExtension(micromodem::protobuf::receive_stat);

    cst->set_version(nmea.as<int>(1) < 6 ? 0 : nmea.as<int>(1));

    try
    {
        
        int version_offset = 0; // offset in NMEA field number
        if(cst->version() == 0)
        {
            version_offset = 0;    
        }
        else if(cst->version() == 6)
        {
            version_offset = 1;
        }

        micromodem::protobuf::ReceiveMode mode =
            micromodem::protobuf::ReceiveMode_IsValid(nmea.as<int>(1+version_offset)) ?
            nmea.as<micromodem::protobuf::ReceiveMode>(1+version_offset) :
            micromodem::protobuf::INVALID_RECEIVE_MODE;
        
        cst->set_mode(mode);
        cst->set_time(as<uint64>(nmea_time2ptime(nmea.as<std::string>(2+version_offset))));

        micromodem::protobuf::ClockMode clock_mode =
            micromodem::protobuf::ClockMode_IsValid(nmea.as<int>(3+version_offset)) ?
            nmea.as<micromodem::protobuf::ClockMode>(3+version_offset) :
            micromodem::protobuf::INVALID_CLOCK_MODE;

        cst->set_clock_mode(clock_mode);
        cst->set_mfd_peak(nmea.as<double>(4+version_offset));
        cst->set_mfd_power(nmea.as<double>(5+version_offset));
        cst->set_mfd_ratio(nmea.as<double>(6+version_offset));
        cst->set_spl(nmea.as<double>(7+version_offset));
        cst->set_shf_agn(nmea.as<double>(8+version_offset));
        cst->set_shf_ainpshift(nmea.as<double>(9+version_offset));
        cst->set_shf_ainshift(nmea.as<double>(10+version_offset)); 
        cst->set_shf_mfdshift(nmea.as<double>(11+version_offset));
        cst->set_shf_p2bshift(nmea.as<double>(12+version_offset));
        cst->set_rate(nmea.as<int32>(13+version_offset));
        cst->set_source(nmea.as<int32>(14+version_offset));
        cst->set_dest(nmea.as<int32>(15+version_offset));

        micromodem::protobuf::PSKErrorCode psk_error_code =
            micromodem::protobuf::PSKErrorCode_IsValid(nmea.as<int>(16+version_offset)) ?
            nmea.as<micromodem::protobuf::PSKErrorCode>(16+version_offset) :
            micromodem::protobuf::INVALID_PSK_ERROR_CODE;

        cst->set_psk_error_code(psk_error_code);

        micromodem::protobuf::PacketType packet_type =
            micromodem::protobuf::PacketType_IsValid(nmea.as<int>(17+version_offset)) ?
            nmea.as<micromodem::protobuf::PacketType>(17+version_offset) :
            micromodem::protobuf::PACKET_TYPE_UNKNOWN;

        
        cst->set_packet_type(packet_type);
        cst->set_number_frames(nmea.as<int32>(18+version_offset));
        cst->set_number_bad_frames(nmea.as<int32>(19+version_offset));
        cst->set_snr_rss(nmea.as<double>(20+version_offset));
        cst->set_snr_in(nmea.as<double>(21+version_offset));
        cst->set_snr_out(nmea.as<double>(22+version_offset));
        cst->set_snr_symbols(nmea.as<double>(23+version_offset));
        cst->set_mse_equalizer(nmea.as<double>(24+version_offset));
        cst->set_data_quality_factor(nmea.as<int32>(25+version_offset));
        cst->set_doppler(nmea.as<double>(26+version_offset));
        cst->set_stddev_noise(nmea.as<double>(27+version_offset));
        cst->set_carrier_freq(nmea.as<double>(28+version_offset));
        cst->set_bandwidth(nmea.as<double>(29+version_offset));
    }
    catch(std::out_of_range& e) // thrown by std::vector::at() called by NMEASentence::as()
    {
        glog.is(DEBUG1, lock) && glog << group(glog_in_group()) << warn << "$CACST message shorter than expected" << std::endl << unlock;
    }    
    
    //
    // set toa parameters
    //
    
    // timing relative to synched pps is good, if ccclk is bad, and range is
    // known to be less than ~1500m, range is still usable
    clk_mode_ = cst->clock_mode();
    if(clk_mode_ == micromodem::protobuf::SYNC_TO_PPS_AND_CCCLK_GOOD ||
       clk_mode_ == micromodem::protobuf::SYNC_TO_PPS_AND_CCCLK_BAD)
    {
        micromodem::protobuf::RangingReply* ranging_reply = m->MutableExtension(micromodem::protobuf::ranging_reply);
        boost::posix_time::ptime toa = as<boost::posix_time::ptime>(cst->time());
        double frac_sec = double(toa.time_of_day().fractional_seconds())/toa.time_of_day().ticks_per_second();

        ranging_reply->add_one_way_travel_time(frac_sec);

        ranging_reply->set_ambiguity(micromodem::protobuf::RangingReply::OWTT_SECOND_AMBIGUOUS);
        
        ranging_reply->set_receiver_clk_mode(clk_mode_);
        ranging_reply->set_is_one_way_synchronous(true);        
    }

    if(cst->has_time())
    {
        m->set_time(cst->time());
        m->set_time_source(protobuf::ModemTransmission::MODEM_TIME);
    }    
    

    // aggregate cacst until they form a coherent "single" transmission
    if(expected_remaining_cacst_ == 0)
        signal_receive_and_clear(m);     // CACST is last, so flush the received message
    else
        --expected_remaining_cacst_;
}

//
// UTILITY
//

void goby::acomms::MMDriver::signal_receive_and_clear(protobuf::ModemTransmission* message)
{
    try
    {
        signal_receive(*message);
        message->Clear();
    }
    catch(std::exception& e)
    {
        message->Clear();
        throw;
    }
}


void goby::acomms::MMDriver::set_silent(bool silent)
{
    if(silent)
        write_single_cfg("SRC,0"); // set to Goby broadcast ID to prevent ACKs
    else
        write_single_cfg("SRC," + as<std::string>(driver_cfg_.modem_id()));
}

