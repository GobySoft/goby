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


#ifndef IridiumModemDriver20130823H
#define IridiumModemDriver20130823H

#include "goby/common/time.h"

#include "goby/util/linebasedcomms/tcp_client.h"

#include "goby/acomms/modemdriver/driver_base.h"
#include "goby/acomms/protobuf/iridium_driver.pb.h"

#include "iridium_driver_fsm.h"


namespace goby
{
    namespace acomms
    {
        class IridiumDriver : public ModemDriverBase
        {
          public:
            IridiumDriver();
            ~IridiumDriver();
            void startup(const protobuf::DriverConfig& cfg);

            void modem_init(const protobuf::DriverConfig& cfg);
            
            void shutdown();            
            void do_work();
            void handle_initiate_transmission(const protobuf::ModemTransmission& m);

          private:
            void receive(const protobuf::ModemTransmission& msg);
            void send(const protobuf::ModemTransmission& msg);

            void try_serial_tx();
            void DisplayStateConfiguration();
            
          private:
            fsm::IridiumDriverFSM fsm_;
            protobuf::DriverConfig driver_cfg_;

            boost::shared_ptr<goby::util::TCPClient> debug_client_;
            
            double last_triple_plus_time_;
            enum { TRIPLE_PLUS_WAIT = 2 };
                
            
        };
    }
}
#endif
