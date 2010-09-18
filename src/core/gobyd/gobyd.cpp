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

boost::mutex GobyDaemon::dbo_mutex;
boost::mutex GobyDaemon::subscription_mutex;

std::map<std::string, std::set<boost::shared_ptr<GobyDaemon::ConnectedClient> > > GobyDaemon::subscriptions;

goby::util::FlexOstream GobyDaemon::glogger;

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
    dbo_manager_->set_logger(&glogger);
    dbo_manager_->add_flex_groups(glogger);
    
    std::string verbosity = "verbose";
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
            proto::NotificationToServer request;
            receive(listen_queue_, request);

            switch(request.notification_type())
            {
                case proto::NotificationToServer::CONNECT:
                    connect(request.application_name());
                    break;
                case proto::NotificationToServer::DISCONNECT:
                    disconnect(request.application_name());
                    break;

                    // ignore other requests on the connection line
                default: break;
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
    boost::interprocess::message_queue
        connect_response_queue(boost::interprocess::open_only,
                               std::string(CONNECT_RESPONSE_QUEUE_PREFIX + name).c_str());
    

    // populate server request for connection
    proto::NotificationToClient response;  

    if(!clients_.count(name))
    {
        boost::shared_ptr<ConnectedClient> client(new ConnectedClient(name));
        boost::shared_ptr<boost::thread> thread(new boost::thread(boost::bind(&GobyDaemon::ConnectedClient::run, client)));
        
        clients_[name] = client;
        client_threads_[name] = thread;
        
        glogger << group("connect")
                << "`" << name << "` has connected" << std::endl;

        response.set_notification_type(proto::NotificationToClient::CONNECTION_ACCEPTED);
    }
    else
    {
        glogger << group("connect") << warn
                << "cannot connect: application with name `"
                << name << "` is already connected" << std::endl;

        response.set_notification_type(proto::NotificationToClient::CONNECTION_DENIED);
        response.set_explanation("name in use");
    }

    send(connect_response_queue, response);
}

void GobyDaemon::disconnect(const std::string& name)
{
    if(clients_.count(name))
    {
        clients_[name]->stop();
        client_threads_[name]->join();        

        boost::mutex::scoped_lock lock(subscription_mutex);
        // remove entries from subscription database
        for(std::map<std::string, std::set<boost::shared_ptr<ConnectedClient> > >::iterator it = subscriptions.begin(), n = subscriptions.end(); it != n; ++it)
        {
            it->second.erase(clients_[name]);
        }
    
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
      t_connected_(goby_time()),
      t_last_active_(t_connected_)
{
    glogger.add_group(name_, ">", "blue", name_);

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
    boost::interprocess::message_queue::remove
        (std::string(TO_SERVER_QUEUE_PREFIX + name_).c_str());
    boost::interprocess::message_queue::remove
        (std::string(FROM_SERVER_QUEUE_PREFIX + name_).c_str());    

    glogger << group(name_) << "ConnectedClient destructed" << std::endl;

}


void GobyDaemon::ConnectedClient::run()
{
    try
    {
        boost::interprocess::message_queue
            to_server_queue(boost::interprocess::open_only,
                            std::string(TO_SERVER_QUEUE_PREFIX + name_).c_str());
        while(active_)
        {
            proto::NotificationToServer notification;
            if(timed_receive(to_server_queue, notification, boost::posix_time::seconds(1)))
                process_notification(notification);
        }
    }
    catch(boost::interprocess::interprocess_exception &ex)
    {
        glogger << warn << ex.what() << std::endl;
    }    
}

void GobyDaemon::ConnectedClient::process_notification(const proto::NotificationToServer& notification)
{
    glogger << group(name_) << "> " << notification.ShortDebugString() << std::endl;

    // client sent us a message, so it's still alive
    t_last_active_ = goby_time();

    // if the client sent along a new type, add it to our database
    if(notification.has_file_descriptor_proto())
    {
        boost::mutex::scoped_lock lock(dbo_mutex);
        DBOManager::get_instance()->add_file(notification.file_descriptor_proto());
    }

    
    switch(notification.notification_type())
    {
        case proto::NotificationToServer::PUBLISH_REQUEST: 
            process_publish(notification);
            break;

        case proto::NotificationToServer::SUBSCRIBE_REQUEST:
            process_subscribe(notification);
            break;

        case proto::NotificationToServer::HEARTBEAT:
        default: return;
    }
}


void GobyDaemon::ConnectedClient::process_publish(const goby::core::proto::NotificationToServer& incoming_message)
{
    typedef std::map<std::string, std::set<boost::shared_ptr<ConnectedClient> > > Subscriptions;
    // enforce thread-safety
    const Subscriptions& s = subscriptions;
    Subscriptions::const_iterator it = s.find(incoming_message.message_type());
    if(it != s.end())
    {
        // prepare outgoing message
        proto::NotificationToClient outgoing_message;
        outgoing_message.set_message_type(incoming_message.message_type());
        outgoing_message.set_serialized_message(incoming_message.serialized_message());
        outgoing_message.set_notification_type(proto::NotificationToClient::INCOMING_MESSAGE);
                
        // send it to everyone on the subscription list
        BOOST_FOREACH(const boost::shared_ptr<ConnectedClient>& cc, it->second)
        {
            // don't send mail back to yourself!
            if(cc != shared_from_this())
            {
                boost::interprocess::message_queue
                    from_server_queue(boost::interprocess::open_only,
                                      std::string(FROM_SERVER_QUEUE_PREFIX + cc->name()).c_str());

                if(try_send(from_server_queue, outgoing_message))
                    glogger << group(name_) << "sent message to " << cc->name() << std::endl;
                else
                    glogger << group(name_) << "failed to send message to " << cc->name() << std::endl;
            }
        }
    }
    
    // finally publish this to the SQL database
    boost::mutex::scoped_lock lock(dbo_mutex);
    DBOManager::get_instance()->add_message(incoming_message.message_type(), incoming_message.serialized_message());
}

void GobyDaemon::ConnectedClient::process_subscribe(const goby::core::proto::NotificationToServer& incoming_message)
{
    boost::mutex::scoped_lock lock(subscription_mutex);
    subscriptions[incoming_message.message_type()].insert(shared_from_this());
    glogger << group(name_) << "subscribed to " << incoming_message.message_type() << std::endl;
}

