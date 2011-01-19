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


// queues a single message from the DCCL library

#include "goby/acomms/queue.h"
#include <iostream>


using goby::acomms::operator<<;

void received_data(goby::acomms::QueueKey, const goby::acomms::protobuf::ModemDataTransmission& msg);

int main()
{
    //
    //  1. Initialize the QueueManager     
    //
    
    // create a QueueManager for all our queues
    // and at the same time add our message as a DCCL queue
    goby::acomms::QueueManager q_manager(QUEUE_EXAMPLES_DIR "/queue_simple/simple.xml",
                                         "../../../libdccl/message_schema.xsd", &std::clog);

    // our modem id (arbitrary, but must be unique in the network)
    int our_id = 1;
    q_manager.set_modem_id(our_id);

    // set up the callback to handle received DCCL messages
    q_manager.set_callback_receive(&received_data);
    
    // see what our QueueManager contains
    std::cout << "1. " << q_manager << std::endl;

    // 
    //  2. Push a message to a queue 
    //
    
    // let's make a message to store in the queue
    goby::acomms::protobuf::ModemDataTransmission data_msg;
    
    unsigned dest = 0;
    data_msg.mutable_base()->set_dest(dest);
    // typically grab these data from DCCLCodec::encode, but here we'll just enter an example
    // hexadecimal string
    data_msg.set_data(goby::acomms::hex_decode("2000802500006162636431323334"));

    // push to queue 1 (which is the Simple message <id/>)
    q_manager.push_message(1, data_msg);
    
    std::cout << "2. pushing message to queue 1: " << data_msg << std::endl;
    std::cout << "\tdata as hex: " << goby::acomms::hex_encode(data_msg.data()) << std::endl;
    
    //
    //  3. Create a loopback to simulate the Link Layer (libmodemdriver & modem firmware) 
    //

    std::cout << "3. executing loopback (simulating sending a message to ourselves over the modem link)" << std::endl;
    
    // pretend the modem is requesting data of up to 32 bytes
    
    goby::acomms::protobuf::ModemDataRequest request_msg;
    request_msg.set_max_bytes(32);
    
    data_msg.Clear();
    q_manager.handle_modem_data_request(request_msg, &data_msg);
    
    std::cout << "4. requesting data, got: " << data_msg << std::endl;
    std::cout << "\tdata as hex: " << goby::acomms::hex_encode(data_msg.data()) << std::endl;
    
    // 
    //  4. Pass the received message to the QueueManager (same as outgoing message) 
    //
    q_manager.handle_modem_receive(data_msg);
    
    return 0;
}

//
//  5. Do something with the received message  
//
void received_data(goby::acomms::QueueKey key, const goby::acomms::protobuf::ModemDataTransmission& msg)
{
    std::cout << "5. received message (key is " << key << "): " << msg << std::endl;
    std::cout << "\tdata as hex: " << goby::acomms::hex_encode(msg.data()) << std::endl;
}

