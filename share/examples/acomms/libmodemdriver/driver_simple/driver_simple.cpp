// copyright 2009-2011 t. schneider tes@mit.edu
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
// Usage (WHOI Micro-Modem): run
// > driver_simple /dev/tty_of_modem_A 1
//
// wait a few seconds
// 
// > driver_simple /dev/tty_of_modem_B 2
//
// be careful of collisions if you start them at the same time (this is why libamac exists!)

// Usage (example ABCModem): run
// > driver_simple /dev/tty_of_modem_A 1 ABCDriver
// > driver_simple /dev/tty_of_modem_B 2 ABCDriver
// Also see abc_modem_simulator.cpp

#include "goby/acomms/modem_driver.h"
#include "goby/util/binary.h"
#include "goby/acomms/connect.h"

#include <iostream>

using goby::acomms::operator<<;


void handle_data_request(const goby::acomms::protobuf::ModemDataRequest& request_msg,
                         goby::acomms::protobuf::ModemDataTransmission* data_msg);
void handle_data_receive(const goby::acomms::protobuf::ModemDataTransmission& data_msg);

int main(int argc, char* argv[])
{
    if(argc < 3)
    {
        std::cout << "usage: driver_simple /dev/tty_of_modem modem_id [type: MMDriver (default)|ABCDriver]" << std::endl;
        return 1;
    }

    //
    // 1. Create and initialize the driver we want 
    //
    goby::acomms::ModemDriverBase* driver = 0;
    goby::acomms::protobuf::DriverConfig cfg;

    // set the serial port given on the command line
    cfg.set_serial_port(argv[1]);
    using google::protobuf::uint32;
    // set the source id of this modem
    uint32 our_id = goby::util::as<uint32>(argv[2]);
    cfg.set_modem_id(our_id);

    if(argc == 4)
    {
        if(boost::iequals(argv[3],"ABCDriver"))
        {
            std::cout << "Starting Example driver ABCDriver" << std::endl;
            driver = new goby::acomms::ABCDriver(&std::clog);
        }
    }

    // default to WHOI MicroModem
    if(!driver)
    {
        std::cout << "Starting WHOI Micro-Modem MMDriver" << std::endl;
        driver = new goby::acomms::MMDriver(&std::clog);
        // turn data quality factor message on
        // (example of setting NVRAM configuration)
        cfg.AddExtension(MicroModemConfig::nvram_cfg, "DQF,1");
    }
    

    // for handling $CADRQ
    goby::acomms::connect(&driver->signal_receive, &handle_data_receive);
    goby::acomms::connect(&driver->signal_data_request, &handle_data_request);
    
    //
    // 2. Startup the driver
    //    
    driver->startup(cfg);

    //
    // 3. Initiate a transmission cycle
    //
    
    goby::acomms::protobuf::ModemDataInit transmit_init_message;
    transmit_init_message.mutable_base()->set_src(goby::util::as<unsigned>(our_id));
    transmit_init_message.mutable_base()->set_dest(goby::acomms::BROADCAST_ID);
    transmit_init_message.mutable_base()->set_rate(0);

    std::cout << transmit_init_message << std::endl;
    
    driver->handle_initiate_transmission(&transmit_init_message);

    //
    // 4. Run the driver
    //

    // 10 hz is good
    int i = 0;
    while(1)
    {
        ++i;
        driver->do_work();

        // send another transmission every 60 seconds
        if(!(i % 600))
            driver->handle_initiate_transmission(&transmit_init_message);
            
        // in here you can initiate more transmissions as you want
        usleep(100000);
    }    

    delete driver;
    return 0;
}

//
// 5. Handle the data request ($CADRQ)
//

void handle_data_request(const goby::acomms::protobuf::ModemDataRequest& request_msg,
                         goby::acomms::protobuf::ModemDataTransmission* data_msg)
{
    data_msg->set_data(goby::util::hex_decode("aa1100bbccddef0987654321"));
    data_msg->set_ack_requested(false);
}

//
// 6. Post the received data 
//

void handle_data_receive(const goby::acomms::protobuf::ModemDataTransmission& data_msg)
{
    std::cout << "got a message: " << data_msg << std::endl;
}
