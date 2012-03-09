

// tests IPC functionality of ZeroMQSerice

#include "goby/common/zeromq_service.h"

#include <boost/thread.hpp>

void node_inbox(goby::common::MarshallingScheme marshalling_scheme,
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
    goby::glog.add_stream(goby::common::logger::DEBUG3, &std::cerr);
    goby::glog.set_name(argv[0]);
    
    goby::common::ZeroMQService node1;
    // must share context for ipc
    goby::common::ZeroMQService node2(node1.zmq_context());

    goby::common::protobuf::ZeroMQServiceConfig publisher_cfg, subscriber_cfg;
    {
            
        goby::common::protobuf::ZeroMQServiceConfig::Socket* subscriber_socket = subscriber_cfg.add_socket();
        subscriber_socket->set_socket_type(goby::common::protobuf::ZeroMQServiceConfig::Socket::SUBSCRIBE);
        subscriber_socket->set_transport(goby::common::protobuf::ZeroMQServiceConfig::Socket::IPC);
        subscriber_socket->set_connect_or_bind(goby::common::protobuf::ZeroMQServiceConfig::Socket::CONNECT);

        subscriber_socket->set_socket_id(SOCKET_SUBSCRIBE);
        subscriber_socket->set_socket_name("test3_ipc_socket");
        std::cout << subscriber_socket->DebugString() << std::endl;
    }

    {
            
        goby::common::protobuf::ZeroMQServiceConfig::Socket* publisher_socket = publisher_cfg.add_socket();
        publisher_socket->set_socket_type(goby::common::protobuf::ZeroMQServiceConfig::Socket::PUBLISH);
        publisher_socket->set_transport(goby::common::protobuf::ZeroMQServiceConfig::Socket::IPC);
        publisher_socket->set_connect_or_bind(goby::common::protobuf::ZeroMQServiceConfig::Socket::BIND);
        publisher_socket->set_socket_name("test3_ipc_socket");
        publisher_socket->set_socket_id(SOCKET_PUBLISH);
        std::cout << publisher_socket->DebugString() << std::endl;
    }
    

    
    node1.set_cfg(publisher_cfg);

    node2.set_cfg(subscriber_cfg);
    node2.connect_inbox_slot(&node_inbox);
    node2.subscribe_all(SOCKET_SUBSCRIBE);

    usleep(1e3);

    int test_count = 3;
    for(int i = 0; i < test_count; ++i)
    {
        std::cout << "publishing " << data_ << std::endl;
        node1.send(goby::common::MARSHALLING_CSTR, identifier_, &data_, 3, SOCKET_PUBLISH);
        node2.poll(1e6);
    }

    assert(inbox_count_ == test_count);
    
    std::cout << "all tests passed" << std::endl;
}


void node_inbox(goby::common::MarshallingScheme marshalling_scheme,
                 const std::string& identifier,
                 const void* data,
                 int size,
                 int socket_id)
{
    assert(identifier == identifier_);
    assert(marshalling_scheme == goby::common::MARSHALLING_CSTR);
    assert(!strcmp(static_cast<const char*>(data), data_));
    assert(socket_id == SOCKET_SUBSCRIBE);
    
    std::cout << "Received: " << static_cast<const char*>(data) << std::endl;
    ++inbox_count_;
}
