// t. schneider tes@mit.edu 06.19.09
// ocean engineering graduate student - mit / whoi joint program
// massachusetts institute of technology (mit)
// laboratory for autonomous marine sensing systems (lamss)
// 
// this is mm_driver.h, part of lib_mmdriver, a
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

#ifndef ModemH
#define ModemH

#include <ctime>
#include <fstream>

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread.hpp>
#include "asio.hpp"

#include "nmea.h"
#include "flex_cout.h"
#include "serial_client.h"

// for the serial connection ($CCCFG,BR1,3)
const unsigned DEFAULT_BAUD = 19200;

// seconds to wait for modem to respond
const time_t MODEM_WAIT = 3;
// failures before quitting
const unsigned MAX_FAILS_BEFORE_DEAD = 5;
// how many retries on a given message
const unsigned RETRIES = 3;
// seconds to wait after modem reboot
const unsigned WAIT_AFTER_REBOOT = 2;

// allowed time skew between our clock and the modem clock
const unsigned ALLOWED_SKEW = 1;

namespace micromodem { class Message; }

class MMDriver
{
    
  public:
    MMDriver(FlexCout & tout);

    void startup();
    void do_work();
    
    void validate_and_write(NMEA & nmea);

    void set_serial_port(const std::string& s) { serial_port_ = s; }
    void set_baud(unsigned u) { baud_ = u; }

    void set_cfg(std::map<std::string, unsigned> cfg) { cfg_ = cfg; }
    void set_reset_cfg() { cfg_["ALL"] = 0; }
    
    void set_log_path(const std::string& s) { log_path_ = s; }

    void set_modem_id(unsigned u) { modem_id_ = u; }
    
    
    typedef boost::function<bool (micromodem::Message & message)> MsgFunc1;
    typedef boost::function<bool (micromodem::Message & message1, micromodem::Message & message2)> MsgFunc2;
    typedef boost::function<void (const std::string& s)> StrFunc1;
    void set_receive_cb(MsgFunc1 func) { callback_rxd = func; }
    void set_ack_cb(MsgFunc1 func) { callback_ack = func; }
    void set_datarequest_cb(MsgFunc2 func) { callback_drq = func; }
    void set_in_raw_cb(StrFunc1 func) { callback_in_raw = func; }
    void set_out_raw_cb(StrFunc1 func) { callback_out_raw = func; }
    void set_in_parsed_cb(MsgFunc1 func) { callback_decoded = func; }
        
    unsigned baud() { return baud_; }
    
  private:
    // callbacks to the parent class
    MsgFunc1 callback_rxd;
    MsgFunc1 callback_ack;
    MsgFunc2 callback_drq;
    MsgFunc1 callback_decoded;
    
    StrFunc1 callback_in_raw;
    StrFunc1 callback_out_raw;    
    
    
    // startup
    void initialize_talkers();
    void set_clock();
    void write_cfg();
    void check_cfg();

    // output
    void handle_modem_out();
    void pop_out();
    
    // input
    void handle_modem_in(NMEA & nmea);    
    void ack(NMEA & nmea, micromodem::Message& m);
    void drq(NMEA & nmea, micromodem::Message& m);
    void rxd(NMEA & nmea, micromodem::Message& m);
    void mpa(NMEA & nmea, micromodem::Message& m);
    void mpr(NMEA & nmea, micromodem::Message& m);
    void rev(NMEA & nmea, micromodem::Message& m);
    void err(NMEA & nmea, micromodem::Message& m);
    void cfg(NMEA & nmea, micromodem::Message& m);
    void clk(NMEA & nmea, micromodem::Message& m);
    void cyc(NMEA & nmea, micromodem::Message& m);
    
    // utility
    std::string microsec_simple_time_of_day()
    { return boost::posix_time::to_simple_string(boost::posix_time::microsec_clock::universal_time().time_of_day()); }

    std::string microsec_iso_date_time()
    { return boost::posix_time::to_iso_string(boost::posix_time::microsec_clock::universal_time()); }

    boost::posix_time::ptime now()
    { return boost::posix_time::ptime(boost::posix_time::second_clock::universal_time()); }
    
    boost::posix_time::ptime modem_time2posix_time(const std::string & mt);
    double modem_time2unix_time(const std::string & mt);
    
    
  private:
    unsigned baud_;
    std::string serial_port_;

    unsigned modem_id_;
    
    
    std::string log_path_;
    
    std::deque<std::string> in_;
    std::deque<NMEA> out_;

    // serial port io service
    asio::io_service io_;

    boost::mutex in_mutex_;

    SerialClient serial_;
    
    FlexCout& tout_;

    // modem startup work
    std::map<std::string, unsigned> cfg_; // what config params do we need to send?

    // time of the last message written. we timeout and resend after MODEM_WAIT seconds
    time_t last_write_time_;

    // are we waiting for a command ack (CA) from the modem or can we send another output?
    bool waiting_for_modem_;

    // set after the startup routines finish once. we can't startup on instantiation because
    // the base class sets some of our references (from the MOOS file)
    bool startup_done_;

    // keeps track of number of failures and exits after reaching MAX_FAILS, assuming modem dead
    unsigned global_fail_count_;

    // keeps track of number of failures on the present talker and moves on to the next talker
    // if exceeded
    unsigned present_fail_count_;

    std::ofstream fout_;

    bool clock_set_;


    enum TalkerFronts { front_not_defined,CA,CC,SN,GP};

    enum TalkerBacks  { back_not_defined,
                        ACK,DRQ,RXA,RXD,
                        RXP,TXD,TXA,TXP,
                        TXF,CYC,MPC,MPA,
                        MPR,RSP,MSC,MSA,
                        MSR,EXL,MEC,MEA,
                        MER,MUC,MUA,MUR,
                        PDT,PNT,TTA,MFD,
                        CLK,CFG,AGC,BBD,
                        CFR,CST,MSG,REV,
                        DQF,SHF,SNR,DOP,
                        DBG,FFL,FST,ERR};

    

    // not supported by all compilers and more importantly, not until boost 1.37
//    boost::unordered_map<std::string, TalkerBacks> talker_backs_map_;
//    boost::unordered_map<std::string, TalkerFronts> talker_fronts_map_;
//    boost::unordered_map<std::string, std::string> talker_human_map_;
    
    std::map<std::string, TalkerBacks> talker_backs_map_;
    std::map<std::string, TalkerFronts> talker_fronts_map_;
    std::map<std::string, std::string> talker_human_map_;    

};

#endif
