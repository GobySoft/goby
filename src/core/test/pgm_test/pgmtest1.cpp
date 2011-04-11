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

#define within(num) (int) ((float) num * random () / (RAND_MAX + 1.0))

int main (int argc, char *argv[])
{

    //  Prepare our context and publisher
    zmq::context_t context (2);
    zmq::socket_t publisher (context, ZMQ_PUB);
    zmq::socket_t subscriber (context, ZMQ_SUB);

    try
    {
        publisher.connect("epgm://localhost;239.192.1.1:5555");
        subscriber.connect("epgm://localhost;239.192.1.1:5555");
    }
    catch(std::exception& e)
    {
        std::cout << "error: " << e.what() << std::endl;
    }

    const char *filter = "";
    subscriber.setsockopt(ZMQ_SUBSCRIBE, filter, strlen(filter));

    int64_t rate = 100;
    publisher.setsockopt(ZMQ_RATE, &rate, sizeof(rate));
    
        
    //  Initialize random number generator
    srandom ((unsigned) time (NULL));

    //  Initialize poll set
    zmq_pollitem_t items [] = {
        { publisher, 0, ZMQ_POLLOUT, 0 },
        { subscriber, 0, ZMQ_POLLIN, 0 }
    };
    while (1) {

        zmq_poll (items, 2, -1);
        if (items [0].revents & ZMQ_POLLOUT)
        {
            int zipcode, temperature, relhumidity;
            
            //  Get values that will fool the boss
            zipcode     = within (100000);
            temperature = within (215) - 80;
            relhumidity = within (50) + 10;

            //  Send message to all subscribers
            zmq::message_t message(20);
            snprintf ((char *) message.data(), 20 ,
                      "%05d %d %d", zipcode, temperature, relhumidity);
            publisher.send(message);
            usleep(1);
        }
        if (items [1].revents & ZMQ_POLLIN)
        {
            int zipcode, temperature, relhumidity;
            zmq::message_t update;
            subscriber.recv(&update);

            std::istringstream iss(static_cast<char*>(update.data()));
            iss >> zipcode >> temperature >> relhumidity ;


            std::cout << iss.str() << std::endl;
        
//        std::cout << "zip: " << zipcode << " | temperature: " << temperature << std::endl;
        }
    }
    return 0;
}
