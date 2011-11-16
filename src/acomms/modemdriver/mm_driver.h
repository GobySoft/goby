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
#include "goby/acomms/protobuf/mm_driver.pb.h"
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
            MMDriver();
            /// Destructor.
            ~MMDriver();

            /// \brief Starts the driver.
            ///
            /// \param cfg Configuration for the Micro-Modem driver. DriverConfig is defined in acomms_driver_base.proto, and various extensions specific to the WHOI Micro-Modem are defined in acomms_mm_driver.proto.
            void startup(const protobuf::DriverConfig& cfg);

            /// \brief Stops the driver.
            void shutdown();
            
            /// \brief See ModemDriverBase::do_work()
            void do_work();
            
            /// \brief See ModemDriverBase::handle_initiate_transmission()
            void handle_initiate_transmission(const protobuf::ModemTransmission& m);

            /// \brief Current clock mode of the modem, necessary for synchronous navigation. 
            int clk_mode() { return clk_mode_; }
            
          private:
        
            // startup
            void initialize_talkers(); // insert strings into sentence_id_map_, etc for later use
            void set_clock(); // set the modem clock from the system (goby) clock
            void write_cfg(); // write the NVRAM configuration values to the modem
            void write_single_cfg(const std::string &s); // write a single NVRAM value
            void query_all_cfg(); // query the current NVRAM configuration of the modem
            void set_hydroid_gateway_prefix(int id); // if using the hydroid gateway, set its id number
            
            // output

            void cccyc(protobuf::ModemTransmission* msg);
            void ccmuc(protobuf::ModemTransmission* msg);
            void ccmpc(const protobuf::ModemTransmission& msg);
            void ccpdt(const protobuf::ModemTransmission& msg);
            void ccpnt(const protobuf::ModemTransmission& msg);
             
            void try_send(); // try to send another NMEA message to the modem
            void pop_out(); // pop the NMEA send deque upon successful NMEA acknowledgment
            void cache_outgoing_data(protobuf::ModemTransmission* msg); // cache data upon a CCCYC
            void append_to_write_queue(const util::NMEASentence& nmea); // add a message
            void mm_write(const util::NMEASentence& nmea); // actually write a message (appends hydroid prefix if needed)
            void increment_present_fail();
            void present_fail_exceeds_retries();

            
            // input
            void process_receive(const util::NMEASentence& nmea); // parse a receive message and call proper method
            
            // data cycle
            void cacyc(const util::NMEASentence& nmea, protobuf::ModemTransmission* msg); // $CACYC
            void carxd(const util::NMEASentence& nmea, protobuf::ModemTransmission* msg); // $CARXD
            void camsg(const util::NMEASentence& nmea, protobuf::ModemTransmission* m);

            void caack(const util::NMEASentence& nmea, protobuf::ModemTransmission* msg); // $CAACK
        
            // mini packet
            void camua(const util::NMEASentence& nmea, protobuf::ModemTransmission* msg); // $CAMUA

            // ranging (pings)
            void campr(const util::NMEASentence& nmea, protobuf::ModemTransmission* msg); // $CAMPR
            void campa(const util::NMEASentence& nmea, protobuf::ModemTransmission* msg); // $CAMPA
            void sntta(const util::NMEASentence& nmea, protobuf::ModemTransmission* msg); // $SNTTA

            
            // local modem
            void caxst(const util::NMEASentence& nmea, protobuf::ModemTransmission* msg); // $CAXST
            void cacst(const util::NMEASentence& nmea, protobuf::ModemTransmission* msg); // $CACST
            void carev(const util::NMEASentence& nmea); // $CAREV
            void caerr(const util::NMEASentence& nmea); // $CAERR
            void cacfg(const util::NMEASentence& nmea);
            void caclk(const util::NMEASentence& nmea); // $CACLK
            void cadrq(const util::NMEASentence& nmea, const protobuf::ModemTransmission& m); // $CADRQ

            void validate_transmission_start(const protobuf::ModemTransmission& message);

            void signal_receive_and_clear(protobuf::ModemTransmission* message);
            
            // utility    
            static boost::posix_time::ptime nmea_time2ptime(const std::string& mt);

            // doxygen
            
            /// \example acomms/modemdriver/driver_simple/driver_simple.cpp

            /// \example acomms/chat/chat.cpp            
        
          private:
            // for the serial connection ($CCCFG,BR1,3)
            enum { DEFAULT_BAUD = 19200 };
            // failures before closing port and throwing exception 
            enum { MAX_FAILS_BEFORE_DEAD = 5 };
            // how many retries on a given message
            enum { RETRIES = 3 };
            enum { ROUGH_SPEED_OF_SOUND = 1500 }; // m/s
                
            
            // seconds to wait for modem to respond
            static boost::posix_time::time_duration MODEM_WAIT; 
            // seconds to wait after modem reboot
            static boost::posix_time::time_duration WAIT_AFTER_REBOOT;
            // allowed time diff in millisecs between our clock and the modem clock
            static int ALLOWED_MS_DIFF;
            
            static std::string SERIAL_DELIMITER;
            // number of frames for a given packet type
            static unsigned PACKET_FRAME_COUNT [];
            // size of packet (in bytes) for a given modem rate
            static unsigned PACKET_SIZE [];


            // all startup configuration (DriverConfig defined in acomms_driver_base.proto and extended in acomms_mm_driver.proto)
            protobuf::DriverConfig driver_cfg_;

            // deque for outgoing messages to the modem, we queue them up and send
            // as the modem acknowledges them
            std::deque<util::NMEASentence> out_;
    
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

            // keeps track of clock mode, necessary for synchronous navigation
            micromodem::protobuf::ClockMode clk_mode_;

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
                                TOA,XST};
            
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

            protobuf::ModemTransmission transmit_msg_;
            unsigned expected_remaining_caxst_; // used to determine how many CAXST to aggregate (so that bost rate 0 transmissions [CYC and TXD] are provided as a single logical unit)
            
            protobuf::ModemTransmission receive_msg_;
            unsigned expected_remaining_cacst_; // used to determine how many CACST to aggregate (so that rate 0 transmissions [CYC and RXD] are provided as a single logical unit)
            
            // keep track of which frames we've sent and are awaiting acks for. This
            // way we have a chance of intercepting unexpected behavior of the modem
            // relating to ACKs
            std::set<unsigned> frames_waiting_for_ack_;

            std::set<unsigned> frames_waiting_to_receive_;

            // true if we initiated the last cycle ($CCCYC) (and thereby cache data for it)?
            // false if a third party initiated the last cycle
            bool local_cccyc_;

            protobuf::ModemTransmission::TransmissionType last_transmission_type_;
            
        };
    }
}
#endif
