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

#ifndef Modem20091211H
#define Modem20091211H

#include <ctime>
#include <fstream>

#include <boost/date_time/posix_time/posix_time.hpp>

#include "driver_base.h"

namespace modem { class Message; }

namespace micromodem
{    
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

    const std::string SERIAL_DELIMITER = "\r\n";
    
    class MMDriver : public modem::DriverBase
    {
    
      public:
        MMDriver(std::ostream* out = 0);
        ~MMDriver();
    
        void startup();
        void do_work();        
        // CCCYC
        void initiate_transmission(const modem::Message& m);

      private:
        void validate_and_write(NMEA& nmea);
        
        // startup
        void initialize_talkers();
        void set_clock();
        void write_cfg();
        void check_cfg();

        // output
        void handle_modem_out();
        void pop_out();
    
        // input
        void handle_modem_in(NMEA& nmea);
        void ack(NMEA& nmea, modem::Message& m);
        void drq(NMEA& nmea, modem::Message& m);
        void rxd(NMEA& nmea, modem::Message& m);
        void mpa(NMEA& nmea, modem::Message& m);
        void mpr(NMEA& nmea, modem::Message& m);
        void rev(NMEA& nmea, modem::Message& m);
        void err(NMEA& nmea, modem::Message& m);
        void cfg(NMEA& nmea, modem::Message& m);
        void clk(NMEA& nmea, modem::Message& m);
        void cyc(NMEA& nmea, modem::Message& m);
    
        // utility
        std::string microsec_simple_time_of_day()
        { return boost::posix_time::to_simple_string(boost::posix_time::microsec_clock::universal_time().time_of_day()); }

        boost::posix_time::ptime now()
        { return boost::posix_time::ptime(boost::posix_time::second_clock::universal_time()); }
    
        boost::posix_time::ptime modem_time2posix_time(const std::string& mt);
        double modem_time2unix_time(const std::string& mt);
    
    
      private:
        std::deque<NMEA> out_;

        std::ostream* os_;
    
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

    
        std::map<std::string, TalkerBacks> talker_backs_map_;
        std::map<std::string, TalkerFronts> talker_fronts_map_;
    };


}

#endif
