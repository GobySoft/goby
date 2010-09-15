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
#include <iostream>

using goby::acomms::operator<<;

void received_data(goby::acomms::QueueKey, const goby::acomms::ModemMessage&);

int main()
{
    //
    //  1. Initialize the QueueManager     
    //
    
    // create a QueueManager for all our queues
    // and at the same time add our message as a DCCL queue
    goby::acomms::QueueManager q_manager(QUEUE_EXAMPLES_DIR "/queue_simple/simple.xml",
                                  "../../../libdccl/message_schema.xsd", &std::cerr);
    
    unsigned our_id = 1;
    q_manager.set_modem_id(our_id);

    // set up the callback to handle received DCCL messages
    q_manager.set_callback_receive(&received_data);

    // 
    //  2. Push a message to a queue 
    //
    
    // let's make a message to store in the queue
    goby::acomms::ModemMessage msg;

    unsigned dest = 0;

    msg.set_dest(dest);
    // typically grab these data from DCCLCodec::encode, but here we'll just enter an example
    // hexadecimal string
    msg.set_data("2000802500000102030405060708091011121314151617181920212223242526");

    // push to queue 1 (which is the Simple message <id/>)
    q_manager.push_message(1, msg);

    std::cout << "pushing message to queue 1: " << msg << std::endl;
    std::cout << "\t" << "data: " <<  msg.data() << std::endl;

    msg.set_dest(goby::acomms::BROADCAST_ID);
    msg.set_data("2000802500002625242322212019181716151413121110090807060504030201");
    q_manager.push_message(1, msg);
    
    std::cout << "pushing message to queue 1: " << msg << std::endl;
    std::cout << "\t" << "data: " <<  msg.data() << std::endl;
    
    //
    //  3. Create a loopback to simulate the Link Layer (libmodemdriver & modem firmware) 
    //

    std::cout << "executing loopback (simulating sending a message to ourselves over the modem link)" << std::endl;
    
    // pretend the modem is requesting data of up to 64 bytes
    msg.clear();
    msg.set_max_size(64);
    
    q_manager.handle_modem_data_request(msg);

    // we set the incoming message equal to the outgoing message to create the loopback.

    std::cout << "link_layer: " << msg.serialize() << std::endl;
    std::cout << "link_layer: " << goby::util::hex_string2binary_string(msg.data()) << std::endl; 
    
    // 
    //  4. Pass the received message to the QueueManager 
    //
    
    q_manager.handle_modem_receive(msg);
    
    return 0;
}

//
//  5. Do something with the received messages  
//
void received_data(goby::acomms::QueueKey key, const goby::acomms::ModemMessage& app_layer_message_in)
{
    std::cout << "received message (key is " << key << "): " << app_layer_message_in << std::endl;
    std::cout << "\t" << "data: " <<  app_layer_message_in.data() << std::endl;
}

