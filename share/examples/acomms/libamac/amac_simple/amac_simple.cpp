// copyright 2009 t. schneider tes@mit.edu
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

#include "goby/acomms/amac.h"
#include "goby/acomms/connect.h"
#include <iostream>

using goby::acomms::operator<<;

void init_transmission(goby::acomms::protobuf::ModemDataInit* base_msg);

int main(int argc, char* argv[])
{

    //
    // 1. Create a MACManager and feed it a std::ostream to log to
    //
    goby::acomms::MACManager mac(&std::cout);

    //
    // 2. Configure it for TDMA with basic peer discovery, rate 0, 10 second slots, and expire vehicles after 2 cycles of no communications. also, we are modem id 1.
    //
    goby::acomms::protobuf::MACConfig cfg;    
    cfg.set_type(goby::acomms::protobuf::MAC_AUTO_DECENTRALIZED);
    cfg.set_rate(0);
    cfg.set_slot_seconds(10);
    cfg.set_expire_cycles(2);
    cfg.set_modem_id(1);
    
    
    //
    // 3. Set up the callback
    //

    // give a callback to use for actually initiating the transmission. this would be bound to goby::acomms::ModemDriverBase::initiate_transmission if using libmodemdriver.
    goby::acomms::connect(&mac.signal_initiate_transmission, &init_transmission);

    //
    // 4. Let it run for a bit alone in the world
    //
    mac.startup(cfg);
    for(unsigned i = 1; i < 60; ++i)
    {
        mac.do_work();
        sleep(1);
    }
    
    //
    // 5. Discover some friends (modem ids 2 & 3)
    //

    goby::acomms::protobuf::ModemMsgBase msg_from_2, msg_from_3;
    msg_from_2.set_src(2);
    msg_from_3.set_src(3);
    
    mac.handle_modem_all_incoming(msg_from_2);
    mac.handle_modem_all_incoming(msg_from_3);

    //
    // 6. Run it, hearing consistently from #3, but #2 has gone silent and will be expired after 2 cycles
    //
    
    for(;;)
    {
        mac.do_work();
        sleep(1);
        mac.handle_modem_all_incoming(msg_from_2);
    }
    
    return 0;
}

void init_transmission(goby::acomms::protobuf::ModemDataInit* init_message)
{
    std::cout << "starting transmission with these values: " << *init_message << std::endl;
}
