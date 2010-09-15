// copyright 2010 t. schneider tes@mit.edu
// 
// the file is the goby daemon, part of the core goby autonomy system
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

#include <boost/interprocess/ipc/message_queue.hpp>
#include <boost/thread.hpp>
#include <boost/shared_ptr.hpp>

#include "goby/core/libcore/proto_headers.h"
#include "goby/core/core_constants.h"

#include "gobyd.h"

using boost::interprocess::message_queue;
using namespace goby::core;
using namespace goby::util;

goby::util::FlexOstream logger_;

int main()
{
    GobyDaemon g;
    g.run();
    
    return 0;
}


GobyDaemon::GobyDaemon()
    : active_(true),
      listen_queue_(boost::interprocess::open_or_create, LISTEN_QUEUE.c_str(), MAX_NUM_MSG, MAX_MSG_BUFFER_SIZE)
{
    std::string verbosity = "scope";
    logger_.add_stream(verbosity, &std::cout);

    // nocolor, red, lt_red, green, lt_green, yellow,  lt_yellow, blue, lt_blue, magenta, lt_magenta, cyan, lt_cyan, white, lt_white
    logger_.add_group("connect", ">", "lt_magenta", "connections");
    logger_.add_group("disconnect", "<", "lt_blue", "disconnections");
}

GobyDaemon::~GobyDaemon()
{
    boost::interprocess::message_queue::remove(LISTEN_QUEUE.c_str());
}

// handled by listen_thread_
void GobyDaemon::run()
{
    while(active_)
    {
        try
        {
            char buffer [MAX_MSG_BUFFER_SIZE];

            unsigned int priority;
            std::size_t recvd_size;

            // blocks waiting for receive
            listen_queue_.receive(&buffer, MAX_MSG_BUFFER_SIZE, recvd_size, priority);
        
            ServerRequest sr;        
            sr.ParseFromArray(&buffer,recvd_size);

            if(sr.request_type() == ServerRequest::CONNECT)
                do_connect(sr.application_name());
            else if(sr.request_type() == ServerRequest::DISCONNECT)
                do_disconnect(sr.application_name());
        }
        catch(boost::interprocess::interprocess_exception &ex)
        {
            logger_ << warn << ex.what() << std::endl;
        }        
    }
}

void GobyDaemon::do_connect(const std::string& name)
{
    boost::mutex::scoped_lock lock(mutex_);

    boost::interprocess::message_queue
        from_server_queue(boost::interprocess::open_only,
                          std::string(FROM_SERVER_QUEUE_PREFIX + name).c_str());
    

    // populate server request for connection
    goby::core::ServerResponse sr;  

    if(!clients_.count(name))
    {
        boost::shared_ptr<ConnectedClient> client(new ConnectedClient(name));
        boost::shared_ptr<boost::thread> thread(new boost::thread(boost::bind(&GobyDaemon::ConnectedClient::run, client)));
        
        clients_[name] = client;
        client_threads_[name] = thread;
        
        logger_ << group("connect")
                << "`" << name << "` has connected" << std::endl;

        sr.set_response_type(goby::core::ServerRequest::CONNECTION_ACCEPTED);
    }
    else
    {
        logger_ << group("connect") << warn
                << "cannot connect: application with name `"
                << name << "` is already connected" << std::endl;

        sr.set_response_type(goby::core::ServerRequest::CONNECTION_SPURNED);
    }

    // serialize and send the server request
    char buffer [sr.ByteSize()];
    sr.SerializeToArray(&buffer, sizeof(buffer));
    from_server_queue.send(&buffer, sr.ByteSize(), 0);

}

void GobyDaemon::do_disconnect(const std::string& name)
{
    boost::mutex::scoped_lock lock(mutex_);

    if(clients_.count(name))
    {
        clients_[name]->stop();
        client_threads_[name]->join();

        clients_.erase(name);
        client_threads_.erase(name);
        
        logger_ << group("disconnect")
                << "`" << name << "` has disconnected" << std::endl;
                    
    }
    else
    {
        logger_ << group("disconnect") << warn
                << "cannot disconnect: no application with name `"
                << name << "` connected" << std::endl; 
    }
}



GobyDaemon::ConnectedClient::ConnectedClient(const std::string& name)
    : name_(name),
      active_(true),
      t_connected_(goby_time())
{ }

GobyDaemon::ConnectedClient::~ConnectedClient()
{
    active_ = false;
    
    boost::interprocess::message_queue::remove
        (std::string(TO_SERVER_QUEUE_PREFIX + name_).c_str());
    boost::interprocess::message_queue::remove
        (std::string(FROM_SERVER_QUEUE_PREFIX + name_).c_str());    
}


void GobyDaemon::ConnectedClient::run()
{
    boost::interprocess::message_queue::remove
        (std::string(TO_SERVER_QUEUE_PREFIX + name_).c_str());
    
    boost::interprocess::message_queue
        to_server_queue(boost::interprocess::create_only,
                        std::string(TO_SERVER_QUEUE_PREFIX + name_).c_str(),
                        MAX_NUM_MSG,
                        MAX_MSG_BUFFER_SIZE);

    
    while(active_)
    {
        logger_ << warn << name_ << " listener working..." << std::endl;
        sleep(1);
    }
}
