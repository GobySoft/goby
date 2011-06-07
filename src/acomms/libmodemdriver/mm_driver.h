// copyright 2009-2011 t. schneider tes@mit.edu
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
#include "goby/protobuf/mm_driver.pb.h"
#include "goby/acomms/acomms_helpers.h"

namespace goby
{
    namespace acomms
    {

        /// \class MMDriver mm_driver.h goby/acomms/modem_driver.h
        /// \ingroup acomms_api
        /// \brief provides an API to the WHOI Micro-Modem driver
        class MMDriver : public ModemDriverBase
        {
          public:
            /// \brief Default constructor.
            ///
            /// \param log std::ostream object or FlexOstream to capture all humanly
            /// readable runtime and debug information (optional).
            MMDriver(std::ostream* log = 0);
            /// Destructor.
            ~MMDriver();

            /// \brief Starts the driver.
            ///
            /// \param cfg Configuration for the Micro-Modem driver. DriverConfig is defined in driver_base.proto, and various extensions specific to the WHOI Micro-Modem are defined in mm_driver.proto.
            void startup(const protobuf::DriverConfig& cfg);
            void shutdown();
            
            /// \brief Must be called regularly for the driver to perform its work. 10 Hz is good, but less frequently is fine too. Signals will be emitted only during calls to this method.
            void do_work();
            
            /// \brief Initiate a transmission to the modem.
            void handle_initiate_transmission(protobuf::ModemMsgBase* m);

            /// \brief Initiate ranging ("ping") to the modem. 
            void handle_initiate_ranging(protobuf::ModemRangingRequest* m);

          private:
        
            // startup
            void initialize_talkers(); // insert strings into sentence_id_map_, etc for later use
            void set_clock(); // set the modem clock from the system (goby) clock
            void write_cfg(); // write the NVRAM configuration values to the modem
            void write_single_cfg(const std::string &s); // write a single NVRAM value
            void query_all_cfg(); // query the current NVRAM configuration of the modem
            void set_hydroid_gateway_prefix(int id); // if using the hydroid gateway, set its id number
            
            // output
            void try_send(); // try to send another NMEA message to the modem
            void pop_out(); // pop the NMEA send deque upon successful NMEA acknowledgment
            void cache_outgoing_data(const protobuf::ModemDataInit& init_msg); // cache data upon a CCCYC
            void append_to_write_queue(const util::NMEASentence& nmea, protobuf::ModemMsgBase* base_msg); // add a message
            void mm_write(const protobuf::ModemMsgBase& base_msg); // actually write a message (appends hydroid prefix if needed)
            void increment_present_fail();
            void present_fail_exceeds_retries();
            
            // input
            void process_receive(const util::NMEASentence& nmea); // parse a receive message and call proper method
            
            // data cycle
            void cyc(const util::NMEASentence& nmea, protobuf::ModemDataInit* init_msg); // $CACYC 
            void rxd(const util::NMEASentence& nmea, protobuf::ModemDataTransmission* data_msg); // $CARXD
            void ack(const util::NMEASentence& nmea, protobuf::ModemDataAck* ack_msg); // $CAACK

            // ranging (pings)
            void mpr(const util::NMEASentence& nmea, protobuf::ModemRangingReply* ranging_msg); // $CAMPR
            void tta(const util::NMEASentence& nmea, protobuf::ModemRangingReply* ranging_msg); // $SNTTA, why not $CATTA?
            void toa(const util::NMEASentence& nmea, protobuf::ModemRangingReply* ranging_msg); // $CATOA?
            // send toa once we actually know who the message is from 
            void flush_toa(const protobuf::ModemMsgBase& base_msg, protobuf::ModemRangingReply* ranging_msg);

            
            // local modem
            void rev(const util::NMEASentence& nmea); // $CAREV
            void err(const util::NMEASentence& nmea); // $CAERR
            void cfg(const util::NMEASentence& nmea, protobuf::ModemMsgBase* base_msg); // $CACFG
            void clk(const util::NMEASentence& nmea, protobuf::ModemMsgBase* base_msg); // $CACLK
            void drq(const util::NMEASentence& nmea); // $CADRQ

            bool validate_data(const protobuf::ModemDataRequest& request,
                               protobuf::ModemDataTransmission* data);
            
            bool is_valid_destination(int dest) 
            { return dest >= BROADCAST_ID; }
            
            
            // utility    
            static boost::posix_time::ptime nmea_time2ptime(const std::string& mt);

            // doxygen
            /// \example libmodemdriver/driver_simple/driver_simple.cpp
            
            /// \example acomms/chat/chat.cpp            
        
          private:
            // for the serial connection ($CCCFG,BR1,3)
            enum { DEFAULT_BAUD = 19200 };
            // failures before closing port and throwing exception 
            enum { MAX_FAILS_BEFORE_DEAD = 5 };
            // how many retries on a given message
            enum { RETRIES = 3 };
            enum { ROUGH_SPEED_OF_SOUND = 1500 }; // m/s
            enum { REMUS_LBL_TURN_AROUND_MS = 50 }; // milliseconds
                
            
            // seconds to wait for modem to respond
            static boost::posix_time::time_duration MODEM_WAIT; 
            // seconds to wait after modem reboot
            static boost::posix_time::time_duration WAIT_AFTER_REBOOT;
            // allowed time skew between our clock and the modem clock
            static boost::posix_time::time_duration ALLOWED_SKEW;
            
            static std::string SERIAL_DELIMITER;
            // number of frames for a given packet type
            static unsigned PACKET_FRAME_COUNT [];
            // size of packet (in bytes) for a given modem rate
            static unsigned PACKET_SIZE [];


            // all startup configuration (DriverConfig defined in driver_base.proto and extended in mm_driver.proto)
            protobuf::DriverConfig driver_cfg_;

            // deque for outgoing messages to the modem, we queue them up and send
            // as the modem acknowledges them
            std::deque< std::pair<util::NMEASentence, protobuf::ModemMsgBase> > out_;

            // human readable debug log (e.g. &std::cout)
            std::ostream* log_;
    
            // time of the last message written. we timeout and resend after MODEM_WAIT seconds
            boost::posix_time::ptime last_write_time_;
        
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

            // has the clock been properly set. we must reset the clock after reboot ($CAREV,INIT)
            bool clock_set_;

            enum TalkerIDs { TALKER_NOT_DEFINED = 0,CA,CC,SN,GP};

            enum SentenceIDs  { SENTENCE_NOT_DEFINED = 0,
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
                                DBG,FFL,FST,ERR,
                                TOA};
            
            std::map<std::string, TalkerIDs> talker_id_map_;
            std::map<std::string, SentenceIDs> sentence_id_map_;
            std::map<std::string, std::string> description_map_;
            std::map<std::string, std::string> cfg_map_;

            //
            // stuff to deal with the non-standard Hydroid gateway buoy
            //
            
            // length of #G1 or #M1
            enum { HYDROID_GATEWAY_PREFIX_LENGTH = 3 };
            // time between requests to the hydroid gateway buoy gps
            static boost::posix_time::time_duration HYDROID_GATEWAY_GPS_REQUEST_INTERVAL;
            boost::posix_time::ptime last_hydroid_gateway_gps_request_;
            bool is_hydroid_gateway_;
            std::string hydroid_gateway_modem_prefix_;
            std::string hydroid_gateway_gps_request_;
            

            // NVRAM parameters like SRC, DTO, PTO, etc.
            std::map<std::string, int> nvram_cfg_;

            // cache the appropriate amount of data upon CCCYC request (initiate_transmission)
            // for immediate use upon the DRQ message
            // maps frame number to DataTransmission object
            std::map<unsigned, protobuf::ModemDataTransmission> cached_data_msgs_;

            // keep track of which frames we've sent and are awaiting acks for. This
            // way we have a chance of intercepting unexpected behavior of the modem
            // relating to ACKs
            std::set<unsigned> frames_waiting_for_ack_;
            
            // true if we initiated the last cycle ($CCCYC) (and thereby cache data for it)?
            // false if a third party initiated the last cycle
            bool local_cccyc_;
            
        };
    }
}
#endif
