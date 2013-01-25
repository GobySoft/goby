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


// tests functionality of the UDPDriver

#include "goby/acomms/modemdriver/udp_driver.h"
#include "goby/common/logger.h"
#include "goby/util/binary.h"
#include "goby/acomms/connect.h"
#include "goby/acomms/acomms_helpers.h"

using namespace goby::common::logger;
using namespace goby::acomms;
using goby::util::as;
using goby::common::goby_time;
using namespace boost::posix_time;

boost::asio::io_service io1, io2;
boost::shared_ptr<goby::acomms::UDPDriver> driver1, driver2;

static int check_count = 0;

// terminate with -1
static int tests_to_run [] = { 4,5,-1 };
static int tests_to_run_index = 0;
static int test_number = tests_to_run[tests_to_run_index];

void handle_data_request1(protobuf::ModemTransmission* msg);
void handle_modify_transmission1(protobuf::ModemTransmission* msg);
void handle_transmit_result1(const protobuf::ModemTransmission& msg);
void handle_data_receive1(const protobuf::ModemTransmission& msg);

void handle_data_request2(protobuf::ModemTransmission* msg);
void handle_modify_transmission2(protobuf::ModemTransmission* msg);
void handle_transmit_result2(const protobuf::ModemTransmission& msg);
void handle_data_receive2(const protobuf::ModemTransmission& msg);

// void test0();
// void test1();
// void test2();
// void test3();
void test4();
void test5();

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

    goby::glog.add_group("test", goby::common::Colors::green);
    goby::glog.add_group("driver1", goby::common::Colors::green);
    goby::glog.add_group("driver2", goby::common::Colors::yellow);

    driver1.reset(new goby::acomms::UDPDriver(&io1));
    driver2.reset(new goby::acomms::UDPDriver(&io2));
    
    try
    {    
        goby::acomms::protobuf::DriverConfig cfg1, cfg2;
        
        cfg1.set_modem_id(1);


        //gumstix
        UDPDriverConfig::EndPoint* local_endpoint1 =
            cfg1.MutableExtension(UDPDriverConfig::local);
        local_endpoint1->set_port(19300);
        
        cfg2.set_modem_id(2);

        // shore
        UDPDriverConfig::EndPoint* local_endpoint2 =
            cfg2.MutableExtension(UDPDriverConfig::local);        
        local_endpoint2->set_port(19301);

        UDPDriverConfig::EndPoint* remote_endpoint1 =
            cfg1.MutableExtension(UDPDriverConfig::remote);

        remote_endpoint1->set_ip("localhost");
        remote_endpoint1->set_port(19301);
        
        UDPDriverConfig::EndPoint* remote_endpoint2 =
            cfg2.MutableExtension(UDPDriverConfig::remote);

        remote_endpoint2->set_ip("127.0.0.1");
        remote_endpoint2->set_port(19300);
        
        
        goby::acomms::connect(&driver1->signal_receive, &handle_data_receive1);
        goby::acomms::connect(&driver1->signal_transmit_result, &handle_transmit_result1);
        goby::acomms::connect(&driver1->signal_modify_transmission, &handle_modify_transmission1);
        goby::acomms::connect(&driver1->signal_data_request, &handle_data_request1);
        goby::acomms::connect(&driver2->signal_receive, &handle_data_receive2);
        goby::acomms::connect(&driver2->signal_transmit_result, &handle_transmit_result2);
        goby::acomms::connect(&driver2->signal_modify_transmission, &handle_modify_transmission2);
        goby::acomms::connect(&driver2->signal_data_request, &handle_data_request2);

        
        goby::glog << cfg1.DebugString() << std::endl;

        driver1->startup(cfg1);
        driver2->startup(cfg2);

        int i =0;
        
        while(((i / 10) < 1))
        {
            driver1->do_work();
            driver2->do_work();
        
            usleep(100000);
            ++i;
        }
        
    
        for(;;)
        {
            switch(test_number)
            {
                // case 0: test0(); break; 
                // case 1: test1(); break; 
                // case 2: test2(); break; 
                // case 3: test3(); break; 
                case 4: test4(); break; 
                case 5: test5(); break; 
                case -1:
                    goby::glog << group("test") << "all tests passed" << std::endl;
                    driver1->shutdown();
                    driver2->shutdown();
                    return 0;
            }    

            goby::glog << "Test " << group("test") << test_number << " passed." << std::endl;
            ++tests_to_run_index;
            test_number = tests_to_run[tests_to_run_index];
            check_count = 0;
            sleep(1);
            
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
            msg->add_frame(std::string(32, '5'));
            ++check_count;
        }
        break;
            
        case 5:
        {   
            msg->add_frame(std::string(64, '2'));
            ++check_count;
        }
        break;
            
    }

    goby::glog << group("driver1") << "Post data request: " << *msg << std::endl;
}

void handle_modify_transmission1(protobuf::ModemTransmission* msg)
{
    goby::glog << group("driver1") << "Can modify: " << *msg << std::endl;
}

void handle_transmit_result1(const protobuf::ModemTransmission& msg)
{
    goby::glog << group("driver1") << "Completed transmit: " << msg << std::endl;
}


void handle_data_receive1(const protobuf::ModemTransmission& msg)
{
    goby::glog << group("driver1") << "Received: " << msg << std::endl;

    switch(test_number)
    {

        case 4:
        {
            assert(msg.type() == protobuf::ModemTransmission::ACK);
            assert(msg.src() == 2);
            assert(msg.dest() == 1);
            assert(msg.acked_frame_size() == 1 && msg.acked_frame(0) == 0);
            ++check_count;
        }
        break;

        case 5:
        {
            assert(msg.type() == protobuf::ModemTransmission::ACK);
            assert(msg.src() == 2);
            assert(msg.dest() == 1);
            assert(msg.acked_frame_size() == 2 && msg.acked_frame(0) == 0 && msg.acked_frame(1) == 1);
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
            msg->add_frame(goby::util::hex_decode("0123"));
            ++check_count;
            break;
            
    }
    
    goby::glog << group("driver2") << "Post data request: " << *msg << std::endl;
}

void handle_modify_transmission2(protobuf::ModemTransmission* msg)
{
    goby::glog << group("driver2") << "Can modify: " << *msg << std::endl;
}

void handle_transmit_result2(const protobuf::ModemTransmission& msg)
{
    goby::glog << group("driver2") << "Completed transmit: " << msg << std::endl;
}

void handle_data_receive2(const protobuf::ModemTransmission& msg)
{
    goby::glog << group("driver2") << "Received: " << msg << std::endl;

    switch(test_number)
    {
        default:
            assert(false);
            break;


        case 4:
            if(msg.type() == protobuf::ModemTransmission::DATA)
            {
                assert(msg.src() == 1);
                assert(msg.dest() == 2);
                assert(msg.frame_size() == 1);
                assert(msg.frame(0).data() == std::string(32, '5'));
                ++check_count;
            }
            break;
            
        case 5:
            if(msg.type() == protobuf::ModemTransmission::DATA)
            {
                assert(msg.src() == 1);
                assert(msg.dest() == 2);
                assert(msg.frame_size() == 2);
                assert(msg.frame(0).data() == std::string(64, '1'));
                assert(msg.frame(1).data() == std::string(64, '2'));
                ++check_count;
            }
            break;

            
    }


    
}


// void test0()
// {
//     // ping test
//     goby::glog << group("test") << "Ping test" << std::endl;

//     protobuf::ModemTransmission transmit;
    
//     transmit.set_type(protobuf::ModemTransmission::MICROMODEM_TWO_WAY_PING);
//     transmit.set_src(1);
//     transmit.set_dest(2);

//     driver1->handle_initiate_transmission(transmit);

//     int i = 0;
//     while(((i / 10) < 10) && check_count < 2)
//     {
//         driver1->do_work();
// //        driver2->do_work();
        
//         usleep(100000);
//         ++i;
//     }
//     assert(check_count == 2);
// }


// void test1()
// {
//     goby::glog << group("test") << "Remus LBL test" << std::endl;

//     protobuf::ModemTransmission transmit;
    
//     transmit.set_type(protobuf::ModemTransmission::MICROMODEM_REMUS_LBL_RANGING);
//     transmit.set_src(1);
//     transmit.MutableExtension(micromodem::protobuf::remus_lbl)->set_lbl_max_range(1000);
    
//     driver1->handle_initiate_transmission(transmit);

//     int i = 0;
//     while(((i / 10) < 10) && check_count < 1)
//     {
//         driver1->do_work();
// //        driver2->do_work();
        
//         usleep(100000);
//         ++i;
//     }
//     assert(check_count == 1);
// }

// void test2()
// {
//     goby::glog << group("test") << "Narrowband LBL test" << std::endl;

//     protobuf::ModemTransmission transmit;
    
//     transmit.set_type(protobuf::ModemTransmission::MICROMODEM_NARROWBAND_LBL_RANGING);
//     transmit.set_src(1);

//     micromodem::protobuf::NarrowBandLBLParams* params = transmit.MutableExtension(micromodem::protobuf::narrowband_lbl);
//     params->set_lbl_max_range(1000);
//     params->set_turnaround_ms(20);
//     params->set_transmit_freq(26000);
//     params->set_transmit_ping_ms(5);
//     params->set_receive_ping_ms(5);
//     params->add_receive_freq(25000);
//     params->set_transmit_flag(true);
    
//     driver1->handle_initiate_transmission(transmit);

//     int i = 0;
//     while(((i / 10) < 10) && check_count < 1)
//     {
//         driver1->do_work();
// //        driver2->do_work();
        
//         usleep(100000);
//         ++i;
//     }
//     assert(check_count == 1);
// }

// void test3()
// {
//     goby::glog << group("test") << "Mini data test" << std::endl;

//     protobuf::ModemTransmission transmit;
    
//     transmit.set_type(protobuf::ModemTransmission::MICROMODEM_MINI_DATA);
//     transmit.set_src(2);
//     transmit.set_dest(1);
    
// //    driver2->handle_initiate_transmission(transmit);

//     int i = 0;
//     while(((i / 10) < 10) && check_count < 2)
//     {
//         driver1->do_work();
// //        driver2->do_work();
        
//         usleep(100000);
//         ++i;
//     }
//     assert(check_count == 2);
// }


void test4()
{
    goby::glog << group("test") << "Rate 0 test" << std::endl;
    
    protobuf::ModemTransmission transmit;
    
    transmit.set_type(protobuf::ModemTransmission::DATA);
    transmit.set_src(1);
    transmit.set_dest(2);
    transmit.set_rate(0);
    transmit.set_ack_requested(true);
    driver1->handle_initiate_transmission(transmit);

    int i = 0;
    while(((i / 10) < 100) && check_count < 3)
    {
        driver1->do_work();
        driver2->do_work();
        
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
    transmit.set_src(1);
    transmit.set_dest(2);
    transmit.set_rate(2);
    transmit.add_frame(std::string(64,'1'));
    transmit.set_ack_requested(true);
    
    driver1->handle_initiate_transmission(transmit);

    int i = 0;
    while(((i / 10) < 10) && check_count < 3)
    {
        driver1->do_work();
        driver2->do_work();
        
        usleep(100000);
        ++i;
    }
    assert(check_count == 3);
}
