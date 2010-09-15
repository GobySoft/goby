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

#include "server_request.pb.h"

#include "goby/core/core_constants.h"
#include "goby/core/libcore/message_queue_util.h"

#include "gobyd.h"

using boost::interprocess::message_queue;
using namespace goby::core;
using namespace goby::util;

boost::mutex gmutex;

// defined in libcore
extern goby::util::FlexOstream glogger;

int main()
{
    GobyDaemon g;
    g.run();
    
    return 0;
}


GobyDaemon::GobyDaemon()
    : active_(true),
      listen_queue_(boost::interprocess::open_or_create, CONNECT_LISTEN_QUEUE.c_str(), MAX_NUM_MSG, MAX_MSG_BUFFER_SIZE),
      dbo_manager_(goby::core::DBOManager::get_instance())
{
    dbo_manager_->connect("gobyd_test.db");
    
    std::string verbosity = "scope";
    glogger.add_stream(verbosity, &std::cout);
    glogger.name("gobyd");
    
    // nocolor, red, lt_red, green, lt_green, yellow,  lt_yellow, blue, lt_blue, magenta, lt_magenta, cyan, lt_cyan, white, lt_white
    glogger.add_group("connect", ">", "lt_magenta", "connections");
    glogger.add_group("disconnect", "<", "lt_blue", "disconnections");
}

GobyDaemon::~GobyDaemon()
{
    boost::interprocess::message_queue::remove(CONNECT_LISTEN_QUEUE.c_str());
}

void GobyDaemon::run()
{
    while(active_)
    {
        try
        {
            // blocks waiting for receive
            ConnectionRequest request;
            receive(listen_queue_, request);

            switch(request.request_type())
            {
                case ConnectionRequest::CONNECT:
                    connect(request.application_name());
                    break;
                case ConnectionRequest::DISCONNECT:
                    disconnect(request.application_name());
                    break;
            }
        }
        catch(boost::interprocess::interprocess_exception &ex)
        {
            glogger << warn << ex.what() << std::endl;
        }        
    }
}

void GobyDaemon::connect(const std::string& name)
{
    boost::mutex::scoped_lock lock(gmutex);

    boost::interprocess::message_queue
        connect_response_queue(boost::interprocess::open_only,
                          std::string(CONNECT_RESPONSE_QUEUE_PREFIX + name).c_str());
    

    // populate server request for connection
    goby::core::ConnectionResponse response;  

    if(!clients_.count(name))
    {
        boost::shared_ptr<ConnectedClient> client(new ConnectedClient(name));
        boost::shared_ptr<boost::thread> thread(new boost::thread(boost::bind(&GobyDaemon::ConnectedClient::run, client)));
        
        clients_[name] = client;
        client_threads_[name] = thread;
        
        glogger << group("connect")
                << "`" << name << "` has connected" << std::endl;

        response.set_response_type(goby::core::ConnectionResponse::CONNECTION_ACCEPTED);
    }
    else
    {
        glogger << group("connect") << warn
                << "cannot connect: application with name `"
                << name << "` is already connected" << std::endl;

        response.set_response_type(goby::core::ConnectionResponse::CONNECTION_DENIED);
        response.set_denial_reason(goby::core::ConnectionResponse::NAME_IN_USE);
    }

    send(connect_response_queue, response);
}

void GobyDaemon::disconnect(const std::string& name)
{
    boost::mutex::scoped_lock lock(gmutex);

    if(clients_.count(name))
    {
        clients_[name]->stop();
        client_threads_[name]->join();

        clients_.erase(name);
        client_threads_.erase(name);
        
        glogger << group("disconnect")
                << "`" << name << "` has disconnected" << std::endl;
                    
    }
    else
    {
        glogger << group("disconnect") << warn
                << "cannot disconnect: no application with name `"
                << name << "` connected" << std::endl; 
    }
}



GobyDaemon::ConnectedClient::ConnectedClient(const std::string& name)
    : name_(name),
      active_(true),
      t_connected_(goby_time())
{
        boost::interprocess::message_queue::remove
            (std::string(TO_SERVER_QUEUE_PREFIX + name_).c_str());
        
        boost::interprocess::message_queue::remove
            (std::string(FROM_SERVER_QUEUE_PREFIX + name_).c_str());
        
        boost::interprocess::message_queue
            to_server_queue(boost::interprocess::create_only,
                            std::string(TO_SERVER_QUEUE_PREFIX + name_).c_str(),
                            MAX_NUM_MSG,
                            MAX_MSG_BUFFER_SIZE);
        
        
        boost::interprocess::message_queue
            from_server_queue(boost::interprocess::create_only,
                              std::string(FROM_SERVER_QUEUE_PREFIX + name_).c_str(),
                              MAX_NUM_MSG,
                              MAX_MSG_BUFFER_SIZE);
}

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
    try
    {
        boost::interprocess::message_queue
            to_server_queue(boost::interprocess::open_only,
                            std::string(TO_SERVER_QUEUE_PREFIX + name_).c_str());
        boost::interprocess::message_queue
            from_server_queue(boost::interprocess::open_only,
                              std::string(FROM_SERVER_QUEUE_PREFIX + name_).c_str());

        
        while(active_)
        {
            ServerNotification notification;
            if(timed_receive(to_server_queue, notification, boost::posix_time::seconds(1)))
            {
                glogger << notification.ShortDebugString() << std::endl;

                boost::mutex::scoped_lock lock(gmutex);
                goby::core::DBOManager::get_instance()->add_file(notification.file_descriptor_proto());
            }
        }
    }
    catch(boost::interprocess::interprocess_exception &ex)
    {
        glogger << warn << ex.what() << std::endl;
    }    
}
