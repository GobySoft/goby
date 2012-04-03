// Copyright 2009-2012 Toby Schneider (https://launchpad.net/~tes)
//                     Massachusetts Institute of Technology (2007-)
//                     Woods Hole Oceanographic Institution (2007-)
//                     Goby Developers Team (https://launchpad.net/~goby-dev)
// 
//
// This file is part of the Goby Underwater Autonomy Project Libraries
// ("The Goby Libraries").
//
// The Goby Libraries are free software: you can redistribute them and/or modify
// them under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// The Goby Libraries are distributed in the hope that they will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with Goby.  If not, see <http://www.gnu.org/licenses/>.

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
        publisher.connect("epgm://eth0;239.255.7.15:11142");
    }
    catch(std::exception& e)
    {
        std::cout << "error: " << e.what() << std::endl;
    }
    
    int64_t rate = 100;
    publisher.setsockopt(ZMQ_RATE, &rate, sizeof(rate));
    

    //  int i = 0;

    for(int i = 0, n = 2; i < n; ++i)
    {
        
        zmq_msg_t part1;
        int rc = zmq_msg_init_size (&part1, 1);
        assert (rc == 0);
        /* Fill in message content with unsigned char 1 */
        memset (zmq_msg_data (&part1), 1, 1);
        /* Send the message to the socket */
        rc = zmq_send (publisher, &part1, ZMQ_SNDMORE);
        assert (rc == 0);
        
        zmq_msg_t part2;
        rc = zmq_msg_init_size (&part2, 1);
        assert (rc == 0);
/* Fill in message content with unsigned char 2 */
        memset (zmq_msg_data (&part2), 2, 1);
/* Send the message to the socket */
        rc = zmq_send (publisher, &part2, ZMQ_SNDMORE);
        assert (rc == 0);

        zmq_msg_t part3;
        rc = zmq_msg_init_size (&part3, 1);
        assert (rc == 0);
/* Fill in message content with unsigned char 2 */
        memset (zmq_msg_data (&part3), 3, 1);
/* Send the message to the socket */
        rc = zmq_send (publisher, &part3, 0);
        assert (rc == 0);

    }
    
    return 0;
}
