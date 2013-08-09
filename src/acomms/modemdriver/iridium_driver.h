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


#ifndef IridiumModemDriver20120409H
#define IridiumModemDriver20120409H

#include "goby/common/time.h"

#include "goby/acomms/modemdriver/driver_base.h"
#include "goby/acomms/protobuf/iridium_driver.pb.h"


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
            void shutdown();            
            void do_work();
            void handle_initiate_transmission(const protobuf::ModemTransmission& m);

          private:
            void receive(const protobuf::ModemTransmission& msg);
            void send(const google::protobuf::Message& msg);

          private:            
            protobuf::DriverConfig driver_cfg_;
        };
    }
}
#endif
