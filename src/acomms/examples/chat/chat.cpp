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

// usage: connect two modems and then run
// > chat /dev/tty_modem_A 1 2 log_file_A
// > chat /dev/tty_modem_A 2 1 log_file_B

// type into a window and hit enter to send a message. messages will be queued
// and sent on a fixed rotating cycle

#include <iostream>

#include "goby/acomms/dccl.h"
#include "goby/acomms/queue.h"
#include "goby/acomms/modem_driver.h"
#include "goby/acomms/amac.h"
#include "goby/acomms/bind.h"
#include "goby/util/string.h"

#include <boost/lexical_cast.hpp>

#include "chat_curses.h"

using namespace goby::acomms;
using namespace goby::util;

int startup_failure();
void received_data(const protobuf::ModemDataTransmission&);
void received_ack(const protobuf::ModemDataAck&);
std::string decode_received(const std::string& data);

std::ofstream fout_;
DCCLCodec dccl_;
QueueManager q_manager_(&fout_);
MMDriver mm_driver_(&fout_);
MACManager mac_(&fout_);
ChatCurses curses_;


int main(int argc, char* argv[])
{
    //
    // 1. Deal with command line parameters
    //
    
    if(argc != 5) return startup_failure();
    
    std::string serial_port = argv[1];
    int my_id;
    int buddy_id;
    try
    {
        my_id = boost::lexical_cast<int>(argv[2]);
        buddy_id = boost::lexical_cast<int>(argv[3]);
    }
    catch(boost::bad_lexical_cast&)
    {
        std::cerr << "bad value for my_id: " << argv[2] << " or buddy_id: " << argv[3] << ". these must be unsigned integers." << std::endl;
        return startup_failure();        
    }

    std::string log_file = argv[4];
    fout_.open(log_file.c_str());
    if(!fout_.is_open())
    {
        std::cerr << "bad value for log_file: " << log_file << std::endl;
        return startup_failure();        
    }    

    // bind the signals of these libraries
    bind(mm_driver_, q_manager_, mac_);
    
    //
    // Initiate DCCL (libdccl)
    //
    goby::acomms::protobuf::DCCLConfig dccl_cfg;
    dccl_cfg.add_message_file()->set_path(ACOMMS_EXAMPLES_DIR "/chat/chat.xml");
    dccl_cfg.set_schema("../../libdccl/message_schema.xsd");
    
    //
    // Initiate queue manager (libqueue)
    //
    goby::acomms::protobuf::QueueManagerConfig q_manager_cfg;
    q_manager_cfg.set_modem_id(my_id);
    goby::acomms::connect(&q_manager_.signal_receive, &received_data);
    goby::acomms::connect(&q_manager_.signal_ack, &received_ack);
    for(int i = 0, n = dccl_cfg.message_file_size(); i < n; ++i)
        q_manager_cfg.add_message_file()->CopyFrom(dccl_cfg.message_file(i));

    //
    // Initiate modem driver (libmodemdriver)
    //
    goby::acomms::protobuf::DriverConfig driver_cfg;
    driver_cfg.set_serial_port(serial_port);
    driver_cfg.AddExtension(MicroModemConfig::nvram_cfg, "SRC," + as<std::string>(my_id));

    //
    // Initiate medium access control (libamac)
    //
    goby::acomms::protobuf::MACConfig mac_cfg;
    mac_cfg.set_type(protobuf::MAC_FIXED_DECENTRALIZED);
    mac_cfg.set_modem_id(my_id);

    goby::acomms::protobuf::Slot my_slot;
    my_slot.set_src(my_id);
    my_slot.set_dest(buddy_id);
    my_slot.set_rate(0);
    my_slot.set_slot_seconds(12);  
    my_slot.set_type(goby::acomms::protobuf::SLOT_DATA);
    
    goby::acomms::protobuf::Slot buddy_slot;
    buddy_slot.set_src(buddy_id);
    buddy_slot.set_dest(my_id);
    buddy_slot.set_rate(0);
    buddy_slot.set_slot_seconds(12);
    buddy_slot.set_type(goby::acomms::protobuf::SLOT_DATA);

    if(my_id < buddy_id)
    {
        mac_cfg.add_slot()->CopyFrom(my_slot);
        mac_cfg.add_slot()->CopyFrom(buddy_slot);
    }
    else
    {
        mac_cfg.add_slot()->CopyFrom(buddy_slot);
        mac_cfg.add_slot()->CopyFrom(my_slot);
    }    
    
    //
    // Start up everything
    //
    try
    {
        dccl_.set_cfg(dccl_cfg);
        q_manager_.set_cfg(q_manager_cfg);
        mac_.startup(mac_cfg);
        mm_driver_.startup(driver_cfg);
    }
    catch(std::runtime_error& e)
    {
        std::cerr << "exception at startup: " << e.what() << std::endl;
        return startup_failure();
    }
    
    curses_.set_modem_id(my_id);    
    curses_.startup();
    
    //
    // Loop until terminated (CTRL-C)
    //
    for(;;)
    {
        std::string line;
        curses_.run_input(line);

        if(!line.empty())
        {
            std::map<std::string, DCCLMessageVal> vals;
            vals["message"] = line;

            std::string bytes_out;

            unsigned message_id = 1;
            dccl_.encode(message_id, bytes_out, vals);
            
            protobuf::ModemDataTransmission message_out;
            message_out.set_data(bytes_out);
            // send this message to my buddy!
            message_out.mutable_base()->set_dest(buddy_id);

            message_out.mutable_queue_key()->set_id(message_id);
            q_manager_.push_message(message_out);
        }
            
        try
        {
            mm_driver_.do_work();
            mac_.do_work();
        }
        catch(std::runtime_error& e)
        {
            curses_.cleanup();
            std::cerr << "exception while running: " << e.what() << std::endl;
            return 1;
        }
    }
    
    return 0;
}

int startup_failure()
{
    std::cerr << "usage: chat /dev/tty_modem my_id buddy_id log_file" << std::endl;
    return 1;
}

void received_data(const protobuf::ModemDataTransmission& message_in)
{    
    curses_.post_message(message_in.base().src(), decode_received(message_in.data()));
}

void received_ack(const protobuf::ModemDataAck& ack_message)
{   
    curses_.post_message
        (ack_message.base().src(),
         std::string("{ acknowledged receiving message starting with: "
                     + decode_received(ack_message.orig_msg().data()).substr(0,5)) + " }");
}

std::string decode_received(const std::string& data)
{
    std::map<std::string, DCCLMessageVal> vals;
    dccl_.decode(data, vals);
    return vals["message"];
}
