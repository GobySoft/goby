//
//  Weather update client in C++
//  Connects SUB socket to tcp://localhost:5556
//  Collects weather updates and finds avg temp in zipcode
//
//  Olivier Chamoux <olivier.chamoux@fr.thalesgroup.com>
//
#include <zmq.hpp>
#include <iostream>
#include <sstream>
#include <stdio.h>
#include "goby/acomms/acomms_helpers.h"

int main (int argc, char *argv[])
{
    zmq::context_t context (1);

    //  Socket to talk to server
    zmq::socket_t subscriber (context, ZMQ_SUB);
//    zmq::socket_t publisher (context, ZMQ_PUB);

    try
    {
        //      publisher.connect("epgm://127.0.0.1;239.255.7.15:11142");
        subscriber.connect("epgm://127.0.0.1;239.255.7.15:11142");
    }
    catch(std::exception& e)
    {
        std::cout << "error: " << e.what() << std::endl;
    }

    subscriber.setsockopt(ZMQ_SUBSCRIBE, 0, 0);
    
    for(;;)
    {
        
        zmq::message_t update;
        subscriber.recv(&update);
        
        std::string in(static_cast<const char*>(update.data()), update.size());
        std::cout << goby::acomms::hex_encode(in) << std::endl;
    }
    
    return 1;
    
    
    // int i;
    // sscanf(in.c_str(), "%d", &i);
    
    // std::cout << i << std::endl;

    // assert(i == 1);
    
    return 0;
}
