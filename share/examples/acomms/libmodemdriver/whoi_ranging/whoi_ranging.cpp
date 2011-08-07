// copyright 2011 t. schneider tes@mit.edu
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
// > whoi_ranging /dev/tty_of_modem

#include "goby/acomms/modem_driver.h"
#include "goby/acomms/connect.h"

#include <iostream>

using goby::acomms::operator<<;

int usage()
{
    std::cout << "usage: whoi_ranging /dev/tty_of_modem narrowband|remus max_range_meters" << std::endl;
    return 1;
}


void handle_ranging_reply(const goby::acomms::protobuf::ModemRangingReply& reply_msg);

int main(int argc, char* argv[])
{
    if(argc < 4)
        return usage();

    std::string type = argv[2];
    if(type != "narrowband" && type != "remus")
        return usage();
    
    //
    // 1. Create and initialize the Micro-Modem driver
    //
    goby::acomms::MMDriver driver(&std::clog);
    goby::acomms::protobuf::DriverConfig cfg;

    // set the serial port given on the command line
    cfg.set_serial_port(argv[1]);
    using google::protobuf::uint32;
    // set the source id of this modem
    uint32 our_id = 1;
    cfg.set_modem_id(our_id);

    if(type == "remus")
    {
        cfg.SetExtension(MicroModemConfig::remus_turnaround_ms, 50);
        // enable all four beacons
        cfg.SetExtension(MicroModemConfig::remus_enable_beacons, 0x0F);
    }
    else if(type == "narrowband")
    {
        // Benthos UAT-376 example from Micro-Modem Software Interface Guide
        cfg.SetExtension(MicroModemConfig::narrowband_turnaround_ms, 20);
        cfg.SetExtension(MicroModemConfig::narrowband_transmit_freq, 26000);
        cfg.SetExtension(MicroModemConfig::narrowband_transmit_ping_ms, 5);
        cfg.SetExtension(MicroModemConfig::narrowband_receive_ping_ms, 5);
        cfg.AddExtension(MicroModemConfig::narrowband_receive_freq, 25000);
        cfg.SetExtension(MicroModemConfig::narrowband_transmit_flag, true);
    }
    
    

    goby::acomms::connect(&driver.signal_range_reply, &handle_ranging_reply);
    
    //
    // 2. Startup the driver
    //    
    driver.startup(cfg);

    //
    // 3. Initiate an LBL ping
    //
    
    goby::acomms::protobuf::ModemRangingRequest request_msg;
    request_msg.mutable_base()->set_src(our_id);
    request_msg.set_type((type == "remus") ?
                         goby::acomms::protobuf::REMUS_LBL_RANGING :
                         goby::acomms::protobuf::NARROWBAND_LBL_RANGING);
    request_msg.set_lbl_max_range(goby::util::as<unsigned>(argv[3]));
    
    std::cout << "Sending " << type << " LBL ranging request:\n" << request_msg << std::endl;
    
    driver.handle_initiate_ranging(&request_msg);

    //
    // 4. Run the driver
    //

    // 10 hz is good
    int i = 0;
    while(1)
    {
        ++i;
        // this is when any signals will be emitted and the modem serial comms are performed
        driver.do_work();

        // send another transmission every 60 seconds
        if(!(i % 600))
        {
            driver.handle_initiate_ranging(&request_msg);
            std::cout << "Sending " << type << " LBL ranging request:\n" << request_msg << std::endl;
        }

        // 0.1 sec
        usleep(100000);
    }    
    return 0;
}

void handle_ranging_reply(const goby::acomms::protobuf::ModemRangingReply& reply_msg)
{
    std::cout << "Received LBL ranging reply:\n" << reply_msg << std::endl;
}
