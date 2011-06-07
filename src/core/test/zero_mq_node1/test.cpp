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


// tests basic publish/subscribe over EPGM functionality of ZeroMQNode class

#include "goby/core.h"

#include <boost/thread.hpp>

void node_inbox(goby::core::MarshallingScheme marshalling_scheme,
                 const std::string& identifier,
                 const void* data,
                 int size,
                 int socket_id);

const std::string identifier_ = "HI/";
int inbox_count_ = 0;
const char data_ [] = {'h', 'i', '\0'};

enum {
    SOCKET_SUBSCRIBE = 240, SOCKET_PUBLISH = 211
};
    

int main(int argc, char* argv[])
{
    goby::glog.add_stream(goby::util::Logger::DEBUG3, &std::cerr);
    goby::glog.set_name(argv[0]);
    
    goby::core::ZeroMQNode node;

    goby::core::protobuf::ZeroMQNodeConfig pubsub_cfg;

    {
            
        goby::core::protobuf::ZeroMQNodeConfig::Socket* subscriber_socket = pubsub_cfg.add_socket();
        subscriber_socket->set_socket_type(goby::core::protobuf::ZeroMQNodeConfig::Socket::SUBSCRIBE);
        subscriber_socket->set_socket_id(SOCKET_SUBSCRIBE);
        subscriber_socket->set_ethernet_address("127.0.0.1");
        std::cout << subscriber_socket->DebugString() << std::endl;
    }

    {
            
        goby::core::protobuf::ZeroMQNodeConfig::Socket* publisher_socket = pubsub_cfg.add_socket();
        publisher_socket->set_socket_type(goby::core::protobuf::ZeroMQNodeConfig::Socket::PUBLISH);
        publisher_socket->set_ethernet_address("127.0.0.1");
        publisher_socket->set_socket_id(SOCKET_PUBLISH);
        std::cout << publisher_socket->DebugString() << std::endl;
    }
    

    
    node.set_cfg(pubsub_cfg);
    node.connect_inbox_slot(&node_inbox);
    node.subscribe_all(SOCKET_SUBSCRIBE);

    usleep(1e3);

    int test_count = 3;
    for(int i = 0; i < test_count; ++i)
    {
        std::cout << "publishing " << data_ << std::endl;
        node.send(goby::core::MARSHALLING_CSTR, identifier_, &data_, 3, SOCKET_PUBLISH);
        node.poll(1e6);
    }

    assert(inbox_count_ == test_count);
    
    std::cout << "all tests passed" << std::endl;
}


void node_inbox(goby::core::MarshallingScheme marshalling_scheme,
                 const std::string& identifier,
                 const void* data,
                 int size,
                 int socket_id)
{
    assert(identifier == identifier_);
    assert(marshalling_scheme == goby::core::MARSHALLING_CSTR);
    assert(!strcmp(static_cast<const char*>(data), data_));
    assert(socket_id == SOCKET_SUBSCRIBE);
    
    std::cout << "Received: " << static_cast<const char*>(data) << std::endl;
    ++inbox_count_;
}
