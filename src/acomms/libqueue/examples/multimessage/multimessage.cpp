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


// queues two messages from the DCCL library

#include "goby/acomms/queue.h"
#include "goby/acomms/connect.h"
#include <iostream>

using goby::acomms::operator<<;

void received_data(const goby::acomms::protobuf::ModemDataTransmission&);

int main()
{
    //
    //  1. Initialize the QueueManager     
    //
    
    // create a QueueManager for all our queues
    // and at the same time add our message as a DCCL queue
    goby::acomms::QueueManager q_manager(&std::cerr);
    
    unsigned our_id = 1;

    goby::acomms::protobuf::QueueManagerConfig cfg;
    cfg.set_modem_id(our_id);
    cfg.add_message_file()->set_path(QUEUE_EXAMPLES_DIR "/queue_simple/simple.xml");
    q_manager.set_cfg(cfg);
    
    // set up the callback to handle received DCCL messages
    goby::acomms::connect(&q_manager.signal_receive, &received_data);

    // 
    //  2. Push a message to a queue 
    //
    
    // let's make a message to store in the queue
    goby::acomms::protobuf::ModemDataTransmission data_msg;
    unsigned dest = 0;

    data_msg.mutable_base()->set_dest(dest);
    // typically grab these data from DCCLCodec::encode, but here we'll just enter an example
    // hexadecimal string
    data_msg.set_data(goby::acomms::hex_decode("200080250000010203040506070809101112131415161718192021222324252A"));

    // push to queue 1 (which is the Simple message <id/>)
    data_msg.mutable_queue_key()->set_id(1);
    q_manager.push_message(data_msg);

    std::cout << "pushing message to queue 1: " << data_msg << std::endl;
    std::cout << "\t" << "data as hex: " <<  goby::acomms::hex_encode(data_msg.data()) << std::endl;
    
    data_msg.mutable_base()->set_dest(goby::acomms::BROADCAST_ID);
    data_msg.set_data(goby::acomms::hex_decode("200080250000262524232221201918171615141312111009080706050403020B"));
    q_manager.push_message(data_msg);
    
    std::cout << "pushing message to queue 1: " << data_msg << std::endl;
    std::cout << "\t" << "data as hex: " <<  goby::acomms::hex_encode(data_msg.data()) << std::endl;
    
    //
    //  3. Create a loopback to simulate the Link Layer (libmodemdriver & modem firmware) 
    //

    std::cout << "executing loopback (simulating sending a message to ourselves over the modem link)" << std::endl;
    
    // pretend the modem is requesting data of up to 64 bytes
    goby::acomms::protobuf::ModemDataRequest request_msg;
    request_msg.set_max_bytes(64);
    data_msg.Clear();
    
    q_manager.handle_modem_data_request(request_msg, &data_msg);

    // we set the incoming message equal to the outgoing message to create the loopback.

    std::cout << "link_layer: " << data_msg << std::endl;
    std::cout << "link_layer: "
              << goby::acomms::hex_encode(data_msg.data())
              << std::endl; 
    
    // 
    //  4. Pass the received message to the QueueManager 
    //
    
    q_manager.handle_modem_receive(data_msg);
    
    return 0;
}

//
//  5. Do something with the received messages  
//
void received_data(const goby::acomms::protobuf::ModemDataTransmission& app_layer_message_in)
{
    std::cout << "received message: " << app_layer_message_in << std::endl;
    std::cout << "\t" << "data as hex: " <<  goby::acomms::hex_encode(app_layer_message_in.data()) << std::endl;
}

