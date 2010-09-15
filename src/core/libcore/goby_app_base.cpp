// copyright 2010 t. schneider tes@mit.edu
//
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

#include <iostream>

#include "goby/util/time.h"


#include "goby_app_base.h"
#include "proto_headers.h"

using namespace goby::core;
using namespace goby::util;

boost::posix_time::time_duration GobyAppBase::MAX_CONNECTION_TIME = boost::posix_time::seconds(10);

GobyAppBase::GobyAppBase(const std::string& application_name)
    : application_name_(application_name)
{
    try
    {
        // set up queues to wait for response from server
        boost::interprocess::message_queue::remove
            (std::string(FROM_SERVER_QUEUE_PREFIX + application_name_).c_str());

        boost::interprocess::message_queue
            from_server_queue(boost::interprocess::create_only,
                              std::string(FROM_SERVER_QUEUE_PREFIX + application_name_).c_str(),
                              MAX_NUM_MSG,
                              MAX_MSG_BUFFER_SIZE);    

        // make connection
        boost::interprocess::message_queue listen_queue(boost::interprocess::open_only,
                                                        goby::core::LISTEN_QUEUE.c_str());

        
        // populate server request for connection
        goby::core::ServerRequest request;
        request.set_request_type(goby::core::ServerRequest::CONNECT);
        request.set_application_name(application_name_);

        // serialize and send the server request
        char send_buffer [request.ByteSize()];
        request.SerializeToArray(&send_buffer, sizeof(send_buffer));
        listen_queue.send(&send_buffer, request.ByteSize(), 0);        
        
        // wait for response

        char receive_buffer [MAX_MSG_BUFFER_SIZE];
        unsigned int priority;
        std::size_t recvd_size;
        from_server_queue.timed_receive(&receive_buffer, MAX_MSG_BUFFER_SIZE, recvd_size, priority, goby_time() + MAX_CONNECTION_TIME);
        
        goby::core::ServerResponse response;  
        response.ParseFromArray(&receive_buffer,recvd_size);
        
        
    }
    catch(boost::interprocess::interprocess_exception &ex)
    {
        std::cout << ex.what() << std::endl;
    }
}

GobyAppBase::~GobyAppBase()
{
    try
    {
        // make disconnection
        boost::interprocess::message_queue listen_queue(boost::interprocess::open_only,
                                                    goby::core::LISTEN_QUEUE.c_str());
    
    
        // populate server request for connection
        goby::core::ServerRequest sr;
        sr.set_request_type(goby::core::ServerRequest::DISCONNECT);
        sr.set_application_name(application_name_);
        
        // serialize and send the server request
        char buffer [sr.ByteSize()];
        sr.SerializeToArray(&buffer, sizeof(buffer));
        listen_queue.send(&buffer, sr.ByteSize(), 0);
    }
    catch(boost::interprocess::interprocess_exception &ex)
    {
        std::cout << ex.what() << std::endl;
    }

    std::cout << "cleanup" << std::endl;

}

