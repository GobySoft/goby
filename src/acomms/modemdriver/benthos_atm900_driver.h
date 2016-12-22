// Copyright 2009-2016 Toby Schneider (http://gobysoft.org/index.wt/people/toby)
//                     GobySoft, LLC (2013-)
//                     Massachusetts Institute of Technology (2007-2014)
//                     Community contributors (see AUTHORS file)
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

#ifndef BenthosATM900Driver20161221H
#define BenthosATM900Driver20161221H

#include "goby/common/time.h"

#include "driver_base.h"
#include "goby/acomms/protobuf/benthos_atm900.pb.h"
#include "goby/acomms/acomms_helpers.h"
#include "benthos_atm900_driver_fsm.h"

namespace goby
{
    namespace acomms
    {
        class BenthosATM900Driver : public ModemDriverBase
        {
          public:
            BenthosATM900Driver();
            void startup(const protobuf::DriverConfig& cfg);
            void shutdown();            
            void do_work();
            void handle_initiate_transmission(const protobuf::ModemTransmission& m);

        private:
            void receive(const protobuf::ModemTransmission& msg);
            void send(const protobuf::ModemTransmission& msg);
            void try_serial_tx();
            

        private:
            enum { DEFAULT_BAUD = 9600 };
            static const std::string SERIAL_DELIMITER;

            benthos_fsm::BenthosATM900FSM fsm_;
            protobuf::DriverConfig driver_cfg_; // configuration given to you at launch
            goby::uint32 next_frame_;
        };
    }
}
#endif
