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


// tests functionality of the MMDriver WHOI Micro-Modem driver

#include "goby/acomms/modemdriver/mm_driver.h"
#include "goby/util/logger.h"
#include "goby/util/binary.h"
#include "goby/acomms/connect.h"

using namespace goby::acomms;


boost::shared_ptr<goby::acomms::MMDriver> driver1, driver2;

static int test_number = 0;
static int check_count = 0;

void handle_data_request1(protobuf::ModemTransmission* msg);
void handle_modify_transmission1(protobuf::ModemTransmission* msg);
void handle_data_receive1(const protobuf::ModemTransmission& msg);

void handle_data_request2(protobuf::ModemTransmission* msg);
void handle_modify_transmission2(protobuf::ModemTransmission* msg);
void handle_data_receive2(const protobuf::ModemTransmission& msg);

void test0();


int main(int argc, char* argv[])
{
    if(argc < 3)
    {
        std::cout << "usage: test_mmdriver1 /dev/ttyS0 /dev/ttyS1" << std::endl;
        exit(1);
    }
    
    goby::glog.add_stream(goby::util::Logger::GUI, &std::cout);
    goby::glog.set_name(argv[0]);    

    goby::glog.add_group("test", goby::util::Colors::green);
    goby::glog.add_group("driver1", goby::util::Colors::green);
    goby::glog.add_group("driver2", goby::util::Colors::yellow);

    driver1.reset(new goby::acomms::MMDriver(&goby::glog));
    driver2.reset(new goby::acomms::MMDriver(&goby::glog));
    
    try
    {
    
        goby::acomms::protobuf::DriverConfig cfg1, cfg2;
        
        cfg1.set_serial_port(argv[1]);
        cfg1.set_modem_id(1);
        
        cfg2.set_serial_port(argv[2]);
        cfg2.set_modem_id(2);

        goby::acomms::connect(&driver1->signal_receive, &handle_data_receive1);
        goby::acomms::connect(&driver1->signal_modify_transmission, &handle_modify_transmission1);
        goby::acomms::connect(&driver1->signal_data_request, &handle_data_request1);
        goby::acomms::connect(&driver2->signal_receive, &handle_data_receive2);
        goby::acomms::connect(&driver2->signal_modify_transmission, &handle_modify_transmission2);
        goby::acomms::connect(&driver2->signal_data_request, &handle_data_request2);


        goby::glog << cfg1 << std::endl;
        
        driver1->startup(cfg1);

        
        driver2->startup(cfg2);

    
        for(;;)
        {
            switch(test_number)
            {
                case 0: test0(); break;                
                default:
                    goby::glog << group("test") << "all tests passed" << std::endl;
                    return 0;
            }    

            goby::glog << "Test " << group("test") << test_number << " passed." << std::endl;
            ++test_number;
            check_count = 0;
            
        }
    }
    catch(std::exception& e)
    {
        goby::glog <<  warn << "Exception: " << e.what() << std::endl;
        sleep(5);
        exit(2);
    }
    
}


void handle_data_request1(protobuf::ModemTransmission* msg)
{
    goby::glog << group("driver1") << "Data request: " << *msg << std::endl;
}

void handle_modify_transmission1(protobuf::ModemTransmission* msg)
{
    goby::glog << group("driver1") << "Can modify: " << *msg << std::endl;
}

void handle_data_receive1(const protobuf::ModemTransmission& msg)
{
    goby::glog << group("driver1") << "Received: " << msg << std::endl;

    switch(test_number)
    {
        case 0:
            assert(msg.type() == protobuf::ModemTransmission::MICROMODEM_TWO_WAY_PING);
            ++check_count;
            break;

    }
}


void handle_data_request2(protobuf::ModemTransmission* msg)
{
    goby::glog << group("driver2") << "Data request: " << *msg << std::endl;
}

void handle_modify_transmission2(protobuf::ModemTransmission* msg)
{
    goby::glog << group("driver2") << "Can modify: " << *msg << std::endl;
}

void handle_data_receive2(const protobuf::ModemTransmission& msg)
{
    goby::glog << group("driver2") << "Received: " << msg << std::endl;

    switch(test_number)
    {
        case 0:
            assert(msg.type() == protobuf::ModemTransmission::MICROMODEM_TWO_WAY_PING);
            ++check_count;
            break;

    }

}


void test0()
{
    // ping test
    goby::glog << group("test") << "Ping test" << std::endl;

    protobuf::ModemTransmission transmit;
    
    transmit.set_type(protobuf::ModemTransmission::MICROMODEM_TWO_WAY_PING);
    transmit.mutable_base()->set_src(1);
    transmit.mutable_base()->set_dest(2);

    driver1->handle_initiate_transmission(&transmit);

    int i = 0;
    while((i / 10) < 5)
    {
        driver1->poll(0);
        driver2->poll(0);
        
        usleep(100000);
        ++i;
    }
    assert(check_count == 2);
}
