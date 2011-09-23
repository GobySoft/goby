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


// tests basic REP/REQ functionality of ZeroMQService class

#include "goby/core.h"

#include <boost/thread.hpp>

void inbox(goby::core::MarshallingScheme marshalling_scheme,
                 const std::string& identifier,
                 const void* data,
                 int size,
                 int socket_id);


const std::string reply_identifier_ = "RESPONSE/";
const std::string request_identifier_ = "REQUEST/";
int inbox_count_ = 0;
const char request_data_ [] = {'h', 'i', '\0'};    
const char reply_data_ [] = {'w', 'e', 'l', 'l',' ', 'h', 'e', 'l', 'l','o', '\0'};    

// pick some misc values
enum { SOCKET_REQUESTOR = 104952, SOCKET_REPLIER = 2304 
};

    

int main(int argc, char* argv[])
{
    goby::glog.add_stream(goby::util::logger::DEBUG3, &std::cerr);
    goby::glog.set_name(argv[0]);

    goby::core::ZeroMQService node;

    {
        using goby::core::protobuf::ZeroMQServiceConfig;
        
        ZeroMQServiceConfig requestor_cfg;
        ZeroMQServiceConfig::Socket* requestor_socket = requestor_cfg.add_socket();
        requestor_socket->set_socket_type(ZeroMQServiceConfig::Socket::REQUEST);
        requestor_socket->set_transport(ZeroMQServiceConfig::Socket::TCP);
        requestor_socket->set_socket_id(SOCKET_REQUESTOR);
        requestor_socket->set_ethernet_address("127.0.0.1");
        requestor_socket->set_ethernet_port(54321);
        requestor_socket->set_connect_or_bind(ZeroMQServiceConfig::Socket::CONNECT);
        std::cout << requestor_socket->DebugString() << std::endl;

        ZeroMQServiceConfig replier_cfg;
        ZeroMQServiceConfig::Socket* replier_socket = replier_cfg.add_socket();
        replier_socket->set_socket_type(ZeroMQServiceConfig::Socket::REPLY);
        replier_socket->set_transport(ZeroMQServiceConfig::Socket::TCP);
        replier_socket->set_ethernet_port(54321);
        replier_socket->set_connect_or_bind(ZeroMQServiceConfig::Socket::BIND);
        replier_socket->set_socket_id(SOCKET_REPLIER);

        std::cout << replier_socket->DebugString() << std::endl;
    
        node.merge_cfg(requestor_cfg);
        node.merge_cfg(replier_cfg);
    }
    
    node.connect_inbox_slot(&inbox);

    usleep(1e3);

    int test_count = 3;
    for(int i = 0; i < test_count; ++i)
    {
        std::cout << "requesting " << request_data_ << std::endl;
        node.send(goby::core::MARSHALLING_CSTR, request_identifier_, &request_data_, sizeof(request_data_)/sizeof(char), SOCKET_REQUESTOR);
        node.poll(1e6);
        std::cout << "replying " << reply_data_ << std::endl;
        node.send(goby::core::MARSHALLING_CSTR, reply_identifier_, &reply_data_,  sizeof(reply_data_)/sizeof(char), SOCKET_REPLIER);
        node.poll(1e6);
    }

    assert(inbox_count_ == 2*test_count);
    
    std::cout << "all tests passed" << std::endl;
}


void inbox(goby::core::MarshallingScheme marshalling_scheme,
           const std::string& identifier,
           const void* data,
           int size,
           int socket_id)
{

    if(socket_id == SOCKET_REPLIER)
    {        
        assert(identifier == request_identifier_);
        assert(marshalling_scheme == goby::core::MARSHALLING_CSTR);
        assert(!strcmp(static_cast<const char*>(data), request_data_));
    
        std::cout << "Replier Received: " << static_cast<const char*>(data) << std::endl;
        ++inbox_count_;
    }
    else if(socket_id == SOCKET_REQUESTOR)
    {
        assert(identifier == reply_identifier_);
        assert(marshalling_scheme == goby::core::MARSHALLING_CSTR);
        assert(!strcmp(static_cast<const char*>(data), reply_data_));
        
        std::cout << "Requestor Received: " << static_cast<const char*>(data) << std::endl;
        ++inbox_count_;
    }
    else
    {
        assert(false);
    }
    
}
