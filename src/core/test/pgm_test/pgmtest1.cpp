//
 //  Weather update server in C++
//  Binds PUB socket to tcp://*:5556
//  Publishes random weather updates
//
//  Olivier Chamoux <olivier.chamoux@fr.thalesgroup.com>
//
#include <zmq.hpp>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <iostream>
#include <sstream>

int main (int argc, char *argv[])
{
    //  Prepare our context and publisher
    zmq::context_t context (1);
    zmq::socket_t publisher (context, ZMQ_PUB);

    try
    {
        publisher.connect("epgm://127.0.0.1;239.255.7.15:11142");
    }
    catch(std::exception& e)
    {
        std::cout << "error: " << e.what() << std::endl;
    }
    
    int64_t rate = 100;
    publisher.setsockopt(ZMQ_RATE, &rate, sizeof(rate));
    

    int i = 0;
    while (1)
    {
        ++i;
        zmq::message_t message(20);
        sprintf((char *)message.data(), "%05d", i);
        publisher.send(message);
        std::cout << i << std::endl;
        usleep(10000);
    }
    return 0;
}
