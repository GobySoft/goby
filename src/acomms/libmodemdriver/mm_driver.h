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

#include "goby/util/time.h"

#include "driver_base.h"

namespace goby
{
    namespace acomms
    {
        class ModemMessage;


        /// provides an API to the WHOI Micro-Modem driver
        class MMDriver : public ModemDriverBase
        {
          public:
            /// \brief Default constructor.
            ///
            /// \param os std::ostream object or FlexOstream to capture all humanly readable runtime and debug information (optional).
            MMDriver(std::ostream* out = 0);
            /// Destructor.
            ~MMDriver();

            /// \brief Starts the driver.
            void startup();

            /// \brief Must be called regularly for the driver to perform its work.
            void do_work();

            /// \brief Initiate a transmission to the modem. 
            ///
            /// \param m ModemMessage containing the details of the transmission to be started. This does *not* contain data, which must be requested in a call to the datarequest callback (set by DriverBase::set_data_request_cb)
            void handle_mac_initiate_transmission(const ModemMessage& m);

            /// \brief Initiate ranging ("ping") to the modem. 
            ///
            /// \param m ModemMessage containing the details of the ranging request to be started. (source and destination)
            void handle_mac_initiate_ranging(const ModemMessage& m);

            /// \brief Retrieve the desired destination of the next message
            ///
            /// \param rate next rate to be sent
            /// \return successfully stored destination
            bool handle_mac_dest_request(ModemMessage& msg)
            {
                msg.set_max_size(PACKET_SIZE[msg.rate()]);
                // fill in the required destination
                return callback_dest_request(msg);
            }

            // set an additional prefix to support the hydroid gateway
            void set_gateway_prefix(bool is_gateway, int id);

            void write(util::NMEASentence& nmea);
            void measure_noise(unsigned milliseconds_to_average);
            
          private:
        
            // startup
            void initialize_talkers();
            void set_clock();
            void write_cfg();
            void query_all_cfg();

            // output
            void handle_modem_out();
            void pop_out();
            
            // input
            void handle_modem_in(util::NMEASentence& nmea);
            void ack(util::NMEASentence& nmea, ModemMessage& m);
            void drq(util::NMEASentence& nmea, ModemMessage& m);
            void rxd(util::NMEASentence& nmea, ModemMessage& m);
            void mpa(util::NMEASentence& nmea, ModemMessage& m);
            void mpr(util::NMEASentence& nmea, ModemMessage& m);
            void rev(util::NMEASentence& nmea, ModemMessage& m);
            void err(util::NMEASentence& nmea, ModemMessage& m);
            void cfg(util::NMEASentence& nmea, ModemMessage& m);
            void clk(util::NMEASentence& nmea, ModemMessage& m);
            void cyc(util::NMEASentence& nmea, ModemMessage& m);
    
            // utility    
            static boost::posix_time::ptime modem_time2ptime(const std::string& mt);

            // doxygen
            /// \example libmodemdriver/examples/driver_simple/driver_simple.cpp
            /// driver_simple.cpp
        
            /// \example acomms/examples/chat/chat.cpp
        
        
          private:
            // for the serial connection ($CCCFG,BR1,3)
            enum { DEFAULT_BAUD = 19200 };
            // failures before quitting
            enum { MAX_FAILS_BEFORE_DEAD = 5 };
            // how many retries on a given message
            enum { RETRIES = 3 };
            
            // seconds to wait for %modem to respond
            static boost::posix_time::time_duration MODEM_WAIT; 
            // seconds to wait after %modem reboot
            static boost::posix_time::time_duration WAIT_AFTER_REBOOT;
            // allowed time skew between our clock and the %modem clock
            static boost::posix_time::time_duration ALLOWED_SKEW;
            static std::string SERIAL_DELIMITER;
            // number of frames for a given packet type
            static unsigned PACKET_FRAME_COUNT [];
            // size of packet (in bytes) for a given modem rate
            static unsigned PACKET_SIZE [];

            // deque for outgoing messages to the modem, we queue them up and send
            // as the modem acknowledges them
            std::deque<util::NMEASentence> out_;

            std::ostream* log_;
    
            // time of the last message written. we timeout and resend after MODEM_WAIT seconds
            boost::posix_time::ptime last_write_time_;
        
            // are we waiting for a command ack (CA) from the %modem or can we send another output?
            bool waiting_for_modem_;

            // set after the startup routines finish once. we can't startup on instantiation because
            // the base class sets some of our references (from the MOOS file)
            bool startup_done_;
        
            // keeps track of number of failures and exits after reaching MAX_FAILS, assuming %modem dead
            unsigned global_fail_count_;

            // keeps track of number of failures on the present talker and moves on to the next talker
            // if exceeded
            unsigned present_fail_count_;

            bool clock_set_;

            enum TalkerIDs { front_not_defined,CA,CC,SN,GP};

            enum SentenceIDs  { back_not_defined,
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
            
    
            std::map<std::string, TalkerIDs> talker_id_map_;
            std::map<std::string, SentenceIDs> sentence_id_map_;

            std::string gateway_prefix_in_;
            std::string gateway_prefix_out_;

            std::map<std::string, int> nvram_cfg_;

        };
    }
}
#endif
