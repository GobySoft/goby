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
using goby::util::as;
using goby::util::goby_time;
using namespace boost::posix_time;



boost::shared_ptr<goby::acomms::MMDriver> driver1, driver2;

static int test_number = 5;
static int check_count = 0;

void handle_data_request1(protobuf::ModemTransmission* msg);
void handle_modify_transmission1(protobuf::ModemTransmission* msg);
void handle_data_receive1(const protobuf::ModemTransmission& msg);

void handle_data_request2(protobuf::ModemTransmission* msg);
void handle_modify_transmission2(protobuf::ModemTransmission* msg);
void handle_data_receive2(const protobuf::ModemTransmission& msg);

void test0();
void test1();
void test2();
void test3();
void test4();
void test5();


int main(int argc, char* argv[])
{
    if(argc < 3)
    {
        std::cout << "usage: test_mmdriver1 /dev/ttyS0 /dev/ttyS1" << std::endl;
        exit(1);
    }
    
    goby::glog.add_stream(goby::util::Logger::DEBUG3, &std::clog);
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
        // 0111
        cfg1.MutableExtension(micromodem::protobuf::Config::remus_lbl)->set_enable_beacons(7);


        cfg1.AddExtension(micromodem::protobuf::Config::nvram_cfg, "AGC,0");
        cfg2.AddExtension(micromodem::protobuf::Config::nvram_cfg, "AGC,0");

        cfg1.AddExtension(micromodem::protobuf::Config::nvram_cfg, "AGN,0");
        cfg2.AddExtension(micromodem::protobuf::Config::nvram_cfg, "AGN,0");

        
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

        int i =0;
        
        while(((i / 10) < 3))
        {
            driver1->poll(0);
            driver2->poll(0);
        
            usleep(100000);
            ++i;
        }
        
    
        for(;;)
        {
            switch(test_number)
            {
                case 0: test0(); break; 
                case 1: test1(); break; 
                case 2: test2(); break; 
                case 3: test3(); break; 
                case 4: test4(); break; 
                case 5: test5(); break; 
                default:
                    goby::glog << group("test") << "all tests passed" << std::endl;
                    return 0;
            }    

            goby::glog << "Test " << group("test") << test_number << " passed." << std::endl;
            --test_number;
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

    switch(test_number)
    {
        case 4:
        {
            
            protobuf::ModemDataTransmission* frame = msg->add_frame();
            frame->set_data(std::string(32, '5'));
            frame->mutable_base()->set_dest(2);
            frame->mutable_base()->set_src(1);
            frame->set_ack_requested(true);
            ++check_count;
        }
        break;
            
        case 5:
        {   
            protobuf::ModemDataTransmission* frame = msg->add_frame();
            frame->set_data(std::string(64, '2'));
            frame->mutable_base()->set_dest(2);
            frame->mutable_base()->set_src(1);
            frame->set_ack_requested(false);
            ++check_count;
        }
        break;
            
    }
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

        case 1:
        {
            assert(msg.type() == protobuf::ModemTransmission::MICROMODEM_REMUS_LBL_RANGING);
            assert(msg.base().src() == 1);
            assert(!msg.base().has_dest());

            ptime now = goby_time();
            ptime reported = as<ptime>(msg.base().time());                
            assert(reported < now && reported > now - seconds(2));
            ++check_count;
        }
        break;

        case 2:
        {
            assert(msg.type() == protobuf::ModemTransmission::MICROMODEM_NARROWBAND_LBL_RANGING);
            assert(msg.base().src() == 1);
            assert(!msg.base().has_dest());

            ptime now = goby_time();
            ptime reported = as<ptime>(msg.base().time());                
            assert(reported < now && reported > now - seconds(2));
            ++check_count;
        }
        break;

        case 3:
        {
            assert(msg.type() == protobuf::ModemTransmission::MICROMODEM_MINI_DATA);
            assert(msg.base().src() == 2);
            assert(msg.base().dest() == 1);
            assert(msg.frame_size() == 1);
            assert(msg.frame(0).data() == goby::util::hex_decode("0123"));
            ++check_count;
        }
        break;
            
        case 4:
        {
            assert(msg.type() == protobuf::ModemTransmission::ACK);
            assert(msg.base().src() == 2);
            assert(msg.base().dest() == 1);
            assert(msg.frame_size() == 1);
            assert(msg.frame(0).has_frame_number() && msg.frame(0).frame_number() == 0);
            ++check_count;
        }
        break;

        default:
            assert(false);
            break;
    }
}


void handle_data_request2(protobuf::ModemTransmission* msg)
{
    goby::glog << group("driver2") << "Data request: " << *msg << std::endl;

    switch(test_number)
    {
        default:
            assert(false);
            break;


        case 3:
            msg->add_frame()->set_data(goby::util::hex_decode("0123"));
            ++check_count;
            break;
            
    }
    
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
        default:
            assert(false);
            break;

        case 0:
            assert(msg.type() == protobuf::ModemTransmission::MICROMODEM_TWO_WAY_PING);
            ++check_count;
            break;

        case 4:
            if(msg.type() == protobuf::ModemTransmission::DATA)
            {
                assert(msg.base().src() == 1);
                assert(msg.base().dest() == 2);
                assert(msg.frame_size() == 1);
                assert(msg.frame(0).data() == std::string(32, '5'));
                ++check_count;
            }
            break;
            
        case 5:
            if(msg.type() == protobuf::ModemTransmission::DATA)
            {
                assert(msg.base().src() == 1);
                assert(msg.base().dest() == 2);
                assert(msg.frame_size() == 2);
                assert(msg.frame(0).data() == std::string(64, '1'));
                assert(msg.frame(1).data() == std::string(64, '2'));
                ++check_count;
            }
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

    driver1->handle_initiate_transmission(transmit);

    int i = 0;
    while(((i / 10) < 10) && check_count < 2)
    {
        driver1->poll(0);
        driver2->poll(0);
        
        usleep(100000);
        ++i;
    }
    assert(check_count == 2);
}


void test1()
{
    goby::glog << group("test") << "Remus LBL test" << std::endl;

    protobuf::ModemTransmission transmit;
    
    transmit.set_type(protobuf::ModemTransmission::MICROMODEM_REMUS_LBL_RANGING);
    transmit.mutable_base()->set_src(1);
    transmit.MutableExtension(micromodem::protobuf::remus_lbl)->set_lbl_max_range(1000);
    
    driver1->handle_initiate_transmission(transmit);

    int i = 0;
    while(((i / 10) < 10) && check_count < 1)
    {
        driver1->poll(0);
        driver2->poll(0);
        
        usleep(100000);
        ++i;
    }
    assert(check_count == 1);
}

void test2()
{
    goby::glog << group("test") << "Narrowband LBL test" << std::endl;

    protobuf::ModemTransmission transmit;
    
    transmit.set_type(protobuf::ModemTransmission::MICROMODEM_NARROWBAND_LBL_RANGING);
    transmit.mutable_base()->set_src(1);

    micromodem::protobuf::NarrowBandLBLParams* params = transmit.MutableExtension(micromodem::protobuf::narrowband_lbl);
    params->set_lbl_max_range(1000);
    params->set_turnaround_ms(20);
    params->set_transmit_freq(26000);
    params->set_transmit_ping_ms(5);
    params->set_receive_ping_ms(5);
    params->add_receive_freq(25000);
    params->set_transmit_flag(true);
    
    driver1->handle_initiate_transmission(transmit);

    int i = 0;
    while(((i / 10) < 10) && check_count < 1)
    {
        driver1->poll(0);
        driver2->poll(0);
        
        usleep(100000);
        ++i;
    }
    assert(check_count == 1);
}

void test3()
{
    goby::glog << group("test") << "Mini data test" << std::endl;

    protobuf::ModemTransmission transmit;
    
    transmit.set_type(protobuf::ModemTransmission::MICROMODEM_MINI_DATA);
    transmit.mutable_base()->set_src(2);
    transmit.mutable_base()->set_dest(1);
    
    driver2->handle_initiate_transmission(transmit);

    int i = 0;
    while(((i / 10) < 10) && check_count < 2)
    {
        driver1->poll(0);
        driver2->poll(0);
        
        usleep(100000);
        ++i;
    }
    assert(check_count == 2);
}


void test4()
{
    goby::glog << group("test") << "Rate 0 test" << std::endl;
    
    protobuf::ModemTransmission transmit;
    
    transmit.set_type(protobuf::ModemTransmission::DATA);
    transmit.mutable_base()->set_src(1);
    transmit.mutable_base()->set_dest(2);
    transmit.mutable_base()->set_rate(0);
    
    driver1->handle_initiate_transmission(transmit);

    int i = 0;
    while(((i / 10) < 10) && check_count < 3)
    {
        driver1->poll(0);
        driver2->poll(0);
        
        usleep(100000);
        ++i;
    }
    assert(check_count == 3);
}

void test5()
{
    goby::glog << group("test") << "Rate 2 test" << std::endl;
    
    protobuf::ModemTransmission transmit;
    
    transmit.set_type(protobuf::ModemTransmission::DATA);
    transmit.mutable_base()->set_src(1);
    transmit.mutable_base()->set_dest(2);
    transmit.mutable_base()->set_rate(2);
    protobuf::ModemDataTransmission* frame = transmit.add_frame();
    frame->set_data(std::string(64,'1'));
    frame->set_ack_requested(false);
    
    driver1->handle_initiate_transmission(transmit);

    int i = 0;
    while(((i / 10) < 10) && check_count < 2)
    {
        driver1->poll(0);
        driver2->poll(0);
        
        usleep(100000);
        ++i;
    }
    assert(check_count == 2);
}
