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

int main (int argc, char *argv[])
{
    zmq::context_t context (1);

    //  Socket to talk to server
    std::cout << "Collecting updates from weather server…\n" << std::endl;
    zmq::socket_t subscriber (context, ZMQ_SUB);

    try
    {
        subscriber.connect("epgm://wlan0;239.192.1.1:5555");
    }
    catch(std::exception& e)
    {
        std::cout << "error: " << e.what() << std::endl;
    }

    subscriber.setsockopt(ZMQ_SUBSCRIBE, 0, 0);

    for (;;)
    {
        zmq::message_t update;
        int zipcode, temperature, relhumidity;

        subscriber.recv(&update);

        std::istringstream iss(static_cast<char*>(update.data()));
        iss >> zipcode >> temperature >> relhumidity ;

        std::cout     << "temperature: " << temperature << std::endl;
    }
    return 0;
}
