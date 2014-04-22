// Copyright 2009-2014 Toby Schneider (https://launchpad.net/~tes)
//                     GobySoft, LLC (2013-)
//                     Massachusetts Institute of Technology (2007-2014)
//                     Goby Developers Team (https://launchpad.net/~goby-dev)
// 
//
// This file is part of the Goby Underwater Autonomy Project Libraries
// ("The Goby Libraries").
//
// The Goby Libraries are free software: you can redistribute them and/or modify
// them under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 2.1 of the License, or
// (at your option) any later version.
//
// The Goby Libraries are distributed in the hope that they will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with Goby.  If not, see <http://www.gnu.org/licenses/>.



#ifndef IridiumModemDriver20130823H
#define IridiumModemDriver20130823H

#include "goby/common/time.h"

#include "goby/util/linebasedcomms/tcp_client.h"

#include "goby/acomms/modemdriver/driver_base.h"
#include "goby/pb/protobuf/iridium_driver.pb.h"
#include "goby/pb/protobuf/rudics_shore.pb.h"

#include "iridium_driver_fsm.h"

#include "goby/pb/protobuf_node.h"

namespace goby
{
    namespace acomms
    {
        class IridiumDriver : public ModemDriverBase,
            public goby::pb::StaticProtobufNode
        {
          public:
            IridiumDriver(goby::common::ZeroMQService* zeromq_service);
            ~IridiumDriver();
            void startup(const protobuf::DriverConfig& cfg);

            void modem_init();
            
            void shutdown();            
            void do_work();
            void handle_initiate_transmission(const protobuf::ModemTransmission& m);
            void process_transmission(protobuf::ModemTransmission msg, bool dial);
            void handle_mt_response(const acomms::protobuf::MTDataResponse& response);

          private:
            void receive(const protobuf::ModemTransmission& msg);
            void send(const protobuf::ModemTransmission& msg);

            void try_serial_tx();
            void display_state_cfg(std::ostream* os);

            void hangup();
            void set_dtr(bool state);
            bool query_dtr();
            
          private:
            fsm::IridiumDriverFSM fsm_;
            protobuf::DriverConfig driver_cfg_;

            boost::shared_ptr<goby::util::TCPClient> debug_client_;
            
            double last_triple_plus_time_;
            enum { TRIPLE_PLUS_WAIT = 2 };

            // ZMQ stuff
            bool using_zmq_;
            goby::common::ZeroMQService* zeromq_service_;
	    common::protobuf::ZeroMQServiceConfig service_cfg_;
            acomms::protobuf::MTDataRequest request_;
            int request_socket_id_;
            double last_zmq_request_time_;
            double query_interval_seconds_;
            bool waiting_for_reply_;
            
            protobuf::ModemTransmission rudics_mac_msg_;
            double last_send_time_;


            int serial_fd_;

            enum { RATE_RUDICS = 1, RATE_SBD = 0 };    
        };
    }
}
#endif
