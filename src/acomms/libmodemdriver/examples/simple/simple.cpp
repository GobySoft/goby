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

//
// Usage: run
// > driver_simple /dev/tty_of_modem_A 1
//
// wait a few seconds
// 
// > driver_simple /dev/tty_of_modem_B 2
//
// be careful of collisions if you start them at the same time


#include "acomms/modem_driver.h"
#include <iostream>

bool data_request(const modem::Message&, modem::Message&);
void data_receive(const modem::Message&);

int main(int argc, char* argv[])
{
    if(argc != 3)
    {
        std::cout << "usage: driver_simple /dev/tty_of_modem modem_id" << std::endl;
        return 1;
    }    

    //
    // 1. Create and initialize the driver we want (currently WHOI Micro-Modem)
    //
    
    micromodem::MMDriver mm_driver(&std::cout);

    // set the serial port given on the command line
    mm_driver.set_serial_port(argv[1]);

    // set the source id of this modem
    std::string our_id = argv[2];
    std::vector<std::string> cfg(1, std::string("SRC," + our_id)); 

    mm_driver.set_cfg(cfg);

    // for handling $CADRQ
    mm_driver.set_datarequest_cb(&data_request);
    mm_driver.set_receive_cb(&data_receive);
    
    //
    // 2. Startup the driver
    //
    
    mm_driver.startup();

    //
    // 3. Initiate a transmission cycle
    //
    
    modem::Message transmit_init_message;
    transmit_init_message.set_src(our_id);
    transmit_init_message.set_dest(acomms_util::BROADCAST_ID);
    // one frame @ 32 bytes
    transmit_init_message.set_rate(0);

    mm_driver.initiate_transmission(transmit_init_message);

    //
    // 4. Run the driver
    //

    // 10 hz is good
    while(1)
    {
        mm_driver.do_work();

        // in here you can initiate more transmissions as you want
        usleep(100);
    }    
    return 0;
}

//
// 5. Handle the data request ($CADRQ)
//

bool data_request(const modem::Message& request_message, modem::Message& message_out)
{
    message_out.set_src(request_message.src());
    message_out.set_dest(request_message.dest());
    message_out.set_data("aa1100bbccddef0987654321");

    // we have data
    return true;
}

//
// 6. Post the received data 
//

void data_receive(const modem::Message& message_in)
{
    std::cout << "got a message: " << message_in << std::endl;
    std::cout << "\t" << "data: " << message_in.data() << std::endl;
}
