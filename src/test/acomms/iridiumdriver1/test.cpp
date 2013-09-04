// Copyright 2009-2013 Toby Schneider (https://launchpad.net/~tes)
//                     Massachusetts Institute of Technology (2007-)
//                     Woods Hole Oceanographic Institution (2007-)
//                     Goby Developers Team (https://launchpad.net/~goby-dev)
// 
//
// This file is part of the Goby Underwater Autonomy Project Binaries
// ("The Goby Binaries").
//
// The Goby Binaries are free software: you can redistribute them and/or modify
// them under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// The Goby Binaries are distributed in the hope that they will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Goby.  If not, see <http://www.gnu.org/licenses/>.


// tests functionality of the Iridium Driver

#include "goby/acomms/modemdriver/iridium_driver.h"
#include "../driver_tester/driver_tester.h"


boost::shared_ptr<goby::acomms::ModemDriverBase> driver1, driver2;

int main(int argc, char* argv[])
{
    goby::glog.add_stream(goby::common::logger::DEBUG3, &std::clog);
    std::ofstream fout;

    if(argc == 2)
    {
        fout.open(argv[1]);
        goby::glog.add_stream(goby::common::logger::DEBUG3, &fout);        
    }
    
    goby::glog.set_name(argv[0]);    
    
    driver1.reset(new goby::acomms::IridiumDriver);
    driver2.reset(new goby::acomms::IridiumDriver);
    
    goby::acomms::protobuf::DriverConfig cfg1, cfg2;
        
    // // at-duck1
    // cfg1.set_modem_id(1);
    // cfg1.set_connection_type(goby::acomms::protobuf::DriverConfig::CONNECTION_TCP_AS_CLIENT);
    // cfg1.set_tcp_server("127.0.0.1");
    // cfg1.set_reconnect_interval(1);
    // cfg1.set_tcp_port(40001);    
    // IridiumDriverConfig::Remote* shore =
    //     cfg1.MutableExtension(IridiumDriverConfig::remote);
    // shore->set_iridium_number("6002");
    // shore->set_modem_id(2);    
    // cfg1.AddExtension(IridiumDriverConfig::config, "+GSN");
    
    // // at-duck2
    // cfg2.set_modem_id(2);
    // cfg2.set_connection_type(goby::acomms::protobuf::DriverConfig::CONNECTION_TCP_AS_CLIENT);
    // cfg2.set_tcp_server("127.0.0.1");
    // cfg2.set_reconnect_interval(1);
    // cfg2.set_tcp_port(40001);    
    // IridiumDriverConfig::Remote* gumstix =
    //     cfg2.MutableExtension(IridiumDriverConfig::remote);
    // gumstix->set_iridium_number("6001");
    // gumstix->set_modem_id(2);
    // cfg2.AddExtension(IridiumDriverConfig::config, "+GSN");

//    rudics
    {
        cfg1.set_modem_id(1);
        cfg1.set_connection_type(goby::acomms::protobuf::DriverConfig::CONNECTION_TCP_AS_CLIENT);
        cfg1.set_tcp_server("rudics.whoi.edu");
        cfg1.set_tcp_port(54321);    
        IridiumDriverConfig::Remote* remote =
            cfg1.MutableExtension(IridiumDriverConfig::remote);
        remote->set_modem_id(2);
        remote->set_iridium_number("i881693783740");
        cfg1.AddExtension(IridiumDriverConfig::config, "s29=8");
        cfg1.AddExtension(IridiumDriverConfig::config, "s57=9600");
    }
    
//    gumstix
    {
        cfg2.set_modem_id(2);
        cfg2.set_connection_type(goby::acomms::protobuf::DriverConfig::CONNECTION_SERIAL);
        cfg2.set_serial_port("/dev/ttyUSB0");
        cfg2.set_serial_baud(9600);
        IridiumDriverConfig::Remote* remote =
            cfg2.MutableExtension(IridiumDriverConfig::remote);
        remote->set_iridium_number("i00881600005141");
        remote->set_modem_id(1);
        cfg2.AddExtension(IridiumDriverConfig::config, "+IPR=5,0");
        cfg2.AddExtension(IridiumDriverConfig::config, "+CGSN");
        cfg2.AddExtension(IridiumDriverConfig::config, "+CGMM");
        cfg2.AddExtension(IridiumDriverConfig::config, "+CGMI");
        cfg2.AddExtension(IridiumDriverConfig::config, "Q0"); // quiet mode off
        cfg2.AddExtension(IridiumDriverConfig::config, "&D2"); // DTR low hangs up
        cfg2.AddExtension(IridiumDriverConfig::config, "&C1"); // DCD indicates state of connection
    }

    
    std::vector<int> tests_to_run;
    tests_to_run.push_back(4);
    tests_to_run.push_back(5);
    
    DriverTester tester(driver1, driver2, cfg1, cfg2, tests_to_run);
    return tester.run();
}
