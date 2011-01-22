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
            ///
            /// \param m ModemMessage containing the details of the transmission to be started. This does *not* contain data, which must be requested in a call to the datarequest callback (set by DriverBase::set_data_request_cb)
            void handle_initiate_transmission(protobuf::ModemMsgBase* m);

            /// \brief Initiate ranging ("ping") to the modem. 
            ///
            /// \param m ModemMessage containing the details of the ranging request to be started. (source and destination)
            void handle_initiate_ranging(protobuf::ModemRangingRequest* m);

            // set an additional prefix to support the hydroid gateway
            void set_hydroid_gateway_prefix(int id);

            void measure_noise(unsigned milliseconds_to_average);

            // will set the description 
            void append_to_write_queue(const util::NMEASentence& nmea, protobuf::ModemMsgBase* base_msg);
            
          private:
        
            // startup
            void initialize_talkers();
            void set_clock();
            void write_cfg();
            void query_all_cfg();
            
            // output
            void try_send();
            void pop_out();
            void cache_outgoing_data(const protobuf::ModemDataInit& init_msg);
            void mm_write(const protobuf::ModemMsgBase& base_msg);
            
            // input
            void process_receive(const util::NMEASentence& nmea);
            
            // data cycle
            void cyc(const util::NMEASentence& nmea, protobuf::ModemDataInit* init_msg);
            void rxd(const util::NMEASentence& nmea, protobuf::ModemDataTransmission* data_msg);
            void ack(const util::NMEASentence& nmea, protobuf::ModemDataAck* ack_msg);

            // ranging (pings)
            void mpr(const util::NMEASentence& nmea, protobuf::ModemRangingReply* ranging_msg);
            void tta(const util::NMEASentence& nmea, protobuf::ModemRangingReply* ranging_msg);

            // local modem
            void rev(const util::NMEASentence& nmea);
            void err(const util::NMEASentence& nmea);
            void cfg(const util::NMEASentence& nmea, protobuf::ModemMsgBase* base_msg);
            void clk(const util::NMEASentence& nmea, protobuf::ModemMsgBase* base_msg);
            void drq(const util::NMEASentence& nmea);
            
            // utility    
            static boost::posix_time::ptime nmea_time2ptime(const std::string& mt);

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
            enum { ROUGH_SPEED_OF_SOUND = 1500 }; // m/s
            enum { REMUS_LBL_TURN_AROUND_MS = 50 }; // milliseconds
                
            
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
            std::deque< std::pair<util::NMEASentence, protobuf::ModemMsgBase> > out_;

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
            std::map<std::string, std::string> description_map_;
            std::map<std::string, std::string> cfg_map_;
            
            // length of #G1 or #M1
            enum { HYDROID_GATEWAY_PREFIX_LENGTH = 3 };

            // time between requests to the hydroid gateway buoy gps
            static boost::posix_time::time_duration HYDROID_GATEWAY_GPS_REQUEST_INTERVAL;
            boost::posix_time::ptime last_hydroid_gateway_gps_request_;
            bool is_hydroid_gateway_;
            std::string hydroid_gateway_modem_prefix_;
            std::string hydroid_gateway_gps_request_;
            
            
            std::map<std::string, int> nvram_cfg_;

            // cache the appropriate amount of data upon CCCYC request (initiate_transmission)
            // for immediate use upon the DRQ message
            std::deque<protobuf::ModemDataTransmission> cached_data_msgs_;

            // did we initiate the last cycle (and thereby cache data for it)?
            bool local_cccyc_;
            
        };
    }
}
#endif
