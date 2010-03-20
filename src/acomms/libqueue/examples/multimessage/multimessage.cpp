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

#include "acomms/queue.h"
#include <iostream>

using queue::operator<<;

void received_data(queue::QueueKey, const modem::Message&);

int main()
{
    //
    //  1. Initialize the QueueManager     
    //
    
    // create a QueueManager for all our queues
    // and at the same time add our message as a DCCL queue
    queue::QueueManager q_manager(QUEUE_EXAMPLES_DIR "/queue_simple/simple.xml",
                                  "../../../libdccl/message_schema.xsd", &std::cerr);
    
    unsigned our_id = 1;
    q_manager.set_modem_id(our_id);

    // set up the callback to handle received DCCL messages
    q_manager.set_receive_cb(&received_data);
    
    // see what our QueueManager contains
    std::cout << q_manager << std::endl;

    // 
    //  2. Push a message to a queue 
    //
    
    // let's make a message to store in the queue
    modem::Message app_layer_message_out;

    // we're making a loopback in this simple example, so make this message's destination our id
    unsigned dest = 1;

    app_layer_message_out.set_dest(dest);
    // typically grab these data from DCCLCodec::encode, but here we'll just enter an example
    // hexadecimal string
    app_layer_message_out.set_data("2000800000001234");

    // push to queue 1 (which is the Simple message <id/>)
    q_manager.push_message(1, app_layer_message_out);

    std::cout << "pushing message to queue 1: " << app_layer_message_out << std::endl;
    std::cout << "\t" << "data: " <<  app_layer_message_out.data() << std::endl;

    app_layer_message_out.set_dest(acomms::BROADCAST_ID);
    app_layer_message_out.set_data("2000800000005678");
    q_manager.push_message(1, app_layer_message_out);
    
    std::cout << "pushing message to queue 1: " << app_layer_message_out << std::endl;
    std::cout << "\t" << "data: " <<  app_layer_message_out.data() << std::endl;
    
    //
    //  3. Create a loopback to simulate the Link Layer (libmodemdriver & modem firmware) 
    //

    std::cout << "executing loopback (simulating sending a message to ourselves over the modem link)" << std::endl;
    
    // pretend the modem is requesting data of up to 32 bytes
    modem::Message data_request_message;
    data_request_message.set_size(32);
    
    modem::Message link_layer_message_out;
    q_manager.provide_outgoing_modem_data(data_request_message, link_layer_message_out);

    // we set the incoming message equal to the outgoing message to create the loopback.

    std::cout << "link_layer: " << link_layer_message_out.serialize() << std::endl;
    std::cout << "link_layer: " << tes_util::hex_string2binary_string(link_layer_message_out.data()) << std::endl;
    
    
    modem::Message link_layer_message_in = link_layer_message_out;
    
    // 
    //  4. Pass the received message to the QueueManager 
    //
    
    q_manager.receive_incoming_modem_data(link_layer_message_in);
    
    return 0;
}

//
//  5. Do something with the received messages  
//
void received_data(queue::QueueKey key, const modem::Message& app_layer_message_in)
{
    std::cout << "received message (key is " << key << "): " << app_layer_message_in << std::endl;
    std::cout << "\t" << "data: " <<  app_layer_message_in.data() << std::endl;
}

