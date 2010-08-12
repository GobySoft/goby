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
#include "goby/acomms/modem_message.h"
#include <iostream>

bool request_next_dest(goby::acomms::ModemMessage&);
void init_transmission(const goby::acomms::ModemMessage&);

int main(int argc, char* argv[])
{

    //
    // 1. Create a MACManager and feed it a std::ostream to log to
    //
    goby::acomms::MACManager mac(&std::cout);

    //
    // 2. Configure it for TDMA with basic peer discovery, rate 0, 10 second slots, and expire vehicles after 2 cycles of no communications. also, we are modem id 1.
    //
    mac.set_type(goby::acomms::mac_auto_decentralized);
    mac.set_rate(0);
    mac.set_slot_time(10);
    mac.set_expire_cycles(2);
    mac.set_modem_id(1);
    
    //
    // 3. Set up the callbacks
    //

    // give callback for the next destination. this is called before a cycle is initiated, and would be bound to queue::QueueManager::request_next_destination if using libqueue.
    mac.set_callback_dest_request(&request_next_dest);
    // give a callback to use for actually initiating the transmission. this would be bound to goby::acomms::ModemDriverBase::initiate_transmission if using libmodemdriver.
    mac.set_callback_initiate_transmission(&init_transmission);

    //
    // 4. Let it run for a bit alone in the world
    //
    mac.startup();
    for(unsigned i = 1; i < 60; ++i)
    {
        mac.do_work();
        sleep(1);
    }
    
    //
    // 5. Discover some friends (modem ids 2 & 3)
    //
    
    mac.handle_modem_in_parsed(goby::acomms::ModemMessage("src=2"));
    mac.handle_modem_in_parsed(goby::acomms::ModemMessage("src=3"));

    //
    // 6. Run it, hearing consistently from #3, but #2 has gone silent and will be expired after 2 cycles
    //
    
    for(;;)
    {
        mac.do_work();
        sleep(1);
        mac.handle_modem_in_parsed(goby::acomms::ModemMessage("src=3"));
    }
    
    return 0;
}

bool request_next_dest(goby::acomms::ModemMessage& msg)
{
    // let's pretend we always have a message to send to vehicle 10
    msg.set_dest(10);
    return true;
}

void init_transmission(const goby::acomms::ModemMessage& init_message)
{
    std::cout << "starting transmission with these values: " << init_message << std::endl;
}
