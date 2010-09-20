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
using boost::shared_ptr;

using namespace goby::core;
using namespace goby::util;

boost::mutex Daemon::dbo_mutex;
boost::mutex Daemon::subscription_mutex;
boost::mutex Daemon::logger_mutex;
goby::util::FlexOstream Daemon::glogger;

Daemon::TypeNameSubscribersMap Daemon::subscriptions;


// how long a client can be quiet before we ask for a heartbeat
boost::posix_time::time_duration Daemon::HEARTBEAT_INTERVAL = boost::posix_time::seconds(2);

// how long a client can be quiet before we send it to the morgue
boost::posix_time::time_duration Daemon::DEAD_INTERVAL = Daemon::HEARTBEAT_INTERVAL + Daemon::HEARTBEAT_INTERVAL;


int main()
{
    Daemon g;
    g.run();
    
    return 0;
}


Daemon::Daemon()
    : active_(true),
      dbo_manager_(DBOManager::get_instance())
{
    message_queue::remove(CONNECT_LISTEN_QUEUE.c_str());

    listen_queue_ = boost::shared_ptr<message_queue>
        (new message_queue
         (boost::interprocess::create_only, CONNECT_LISTEN_QUEUE.c_str(), MAX_NUM_MSG, MAX_MSG_BUFFER_SIZE));
    
    
    dbo_manager_->connect("gobyd_test.db");
    dbo_manager_->set_logger(&glogger);
    dbo_manager_->add_flex_groups(glogger);
    
    std::string verbosity = "verbose";
    glogger.add_stream(verbosity, &std::cout);
    glogger.name("gobyd");
    
    // nocolor, red, lt_red, green, lt_green, yellow,  lt_yellow, blue, lt_blue, magenta, lt_magenta, cyan, lt_cyan, white, lt_white
    glogger.add_group("connect", ">", "lt_magenta", "connections");
    glogger.add_group("disconnect", "<", "lt_blue", "disconnections");

    t_start_ = goby_time();
    t_next_db_commit_ = boost::posix_time::second_clock::universal_time() + boost::posix_time::seconds(1);
}

Daemon::~Daemon()
{
    message_queue::remove(CONNECT_LISTEN_QUEUE.c_str());
}

void Daemon::run()
{
    while(active_)
    {
        // blocks waiting for receive
        proto::NotificationToServer request;
        
        if(timed_receive(*listen_queue_,
                         request,
                         t_next_db_commit_))
        {
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
        else
        {    
            // write database entries to actual db
            boost::mutex::scoped_lock lock(dbo_mutex);
            dbo_manager_->commit();
            // TODO(tes): make this configurable
            t_next_db_commit_ += boost::posix_time::seconds(1);
        }
    }
}

void Daemon::connect(const std::string& name)
{
    message_queue connect_response_queue
        (boost::interprocess::open_only,
         std::string(CONNECT_RESPONSE_QUEUE_PREFIX + name).c_str());
    

    // populate server request for connection
    proto::NotificationToClient response;  

    if(!clients_.count(name))
    {
        shared_ptr<ConnectedClient> client(new ConnectedClient(name));
        shared_ptr<boost::thread> thread(new boost::thread(boost::bind(&Daemon::ConnectedClient::run, client)));
        
        clients_[name] = client;
        client_threads_[name] = thread;

        if(!glogger.quiet())
        {
            boost::mutex::scoped_lock lock(logger_mutex);
            glogger << group("connect") << "`" << name << "` has connected" << std::endl;
        }

        response.set_notification_type(proto::NotificationToClient::CONNECTION_ACCEPTED);
    }
    else
    {
        if(!glogger.quiet())
        {
            boost::mutex::scoped_lock lock(logger_mutex); 
            glogger << group("connect") << warn
                    << "cannot connect: application with name `"
                    << name << "` is already connected" << std::endl;
        }
        
        response.set_notification_type(proto::NotificationToClient::CONNECTION_DENIED);
        response.set_explanation("name in use");
    }

    send(connect_response_queue, response);
}

void Daemon::disconnect(const std::string& name)
{
    if(clients_.count(name))
    {
        clients_[name]->stop();
        client_threads_[name]->join();        

        boost::mutex::scoped_lock lock(subscription_mutex);
        // remove entries from subscription database
        for(TypeNameSubscribersMap::iterator it = subscriptions.begin(),
                n = subscriptions.end(); it != n; ++it)
        {
            for(NameSubscribersMap::iterator jt = it->second.begin(),
                    o = it->second.end(); jt != o; ++jt)
                jt->second.erase(clients_[name]);
        }
    
        clients_.erase(name);
        client_threads_.erase(name);

        if(!glogger.quiet())
        {
            boost::mutex::scoped_lock lock(logger_mutex);
            glogger << group("disconnect")
                    << "`" << name << "` has disconnected" << std::endl;
        }
        
    }
    else
    {
        if(!glogger.quiet())
        {
            boost::mutex::scoped_lock lock(logger_mutex);
            glogger << group("disconnect") << warn
                    << "cannot disconnect: no application with name `"
                    << name << "` connected" << std::endl;
        }
    }
}



Daemon::ConnectedClient::ConnectedClient(const std::string& name)
    : name_(name),
      active_(true),
      t_connected_(goby_time()),
      t_last_active_(t_connected_),
      t_next_heartbeat_(boost::posix_time::second_clock::universal_time() + boost::posix_time::seconds(1))
{
    if(!glogger.quiet())
    {
        boost::mutex::scoped_lock lock(logger_mutex);
        glogger.add_group(name_, ">", "blue", name_);
    }
    
    message_queue::remove
        (std::string(TO_SERVER_QUEUE_PREFIX + name_).c_str());
        
    message_queue::remove
        (std::string(FROM_SERVER_QUEUE_PREFIX + name_).c_str());
        
    message_queue to_server_queue(boost::interprocess::create_only,
                                  std::string(TO_SERVER_QUEUE_PREFIX + name_).c_str(),
                                  MAX_NUM_MSG,
                                  MAX_MSG_BUFFER_SIZE);
        
        
    message_queue from_server_queue(boost::interprocess::create_only,
                                    std::string(FROM_SERVER_QUEUE_PREFIX + name_).c_str(),
                                    MAX_NUM_MSG,
                                    MAX_MSG_BUFFER_SIZE);
}

Daemon::ConnectedClient::~ConnectedClient()
{
    message_queue::remove(std::string(TO_SERVER_QUEUE_PREFIX + name_).c_str());
    message_queue::remove(std::string(FROM_SERVER_QUEUE_PREFIX + name_).c_str());    

    if(!glogger.quiet())
    {
        boost::mutex::scoped_lock lock(logger_mutex);
        glogger << group(name_) << "ConnectedClient destructed" << std::endl;
    }
    

}


void Daemon::ConnectedClient::run()
{
    message_queue to_server_queue(boost::interprocess::open_only,
                                  std::string(TO_SERVER_QUEUE_PREFIX + name_).c_str());
    message_queue from_server_queue(boost::interprocess::open_only,
                                    std::string(FROM_SERVER_QUEUE_PREFIX + name_).c_str());
    
    while(active_)
    {
        proto::NotificationToServer notification;
        if(timed_receive(to_server_queue, notification, t_next_heartbeat_)) 
            process_notification(notification);
        else
        {
            boost::posix_time::ptime now = goby_time();
            
            if(now > t_last_active_ + DEAD_INTERVAL)
            {
                // deceased client - disconnect ourselves
                message_queue listen_queue(boost::interprocess::open_only, CONNECT_LISTEN_QUEUE.c_str());
                
                proto::NotificationToServer sr;
                sr.set_notification_type(proto::NotificationToServer::DISCONNECT);
                sr.set_application_name(name_);
                send(listen_queue, sr);
                stop();
            } 
            else if(now > t_last_active_ + HEARTBEAT_INTERVAL)
            {
                // send request for heartbeat
                proto::NotificationToClient request;
                request.set_notification_type(proto::NotificationToClient::HEARTBEAT_REQUEST);
                send(from_server_queue, request);
            }
            
            t_next_heartbeat_ += boost::posix_time::seconds(5);
        }        
    }
}

void Daemon::ConnectedClient::process_notification(const proto::NotificationToServer& notification)
{
    if(!glogger.quiet())
    {
        boost::mutex::scoped_lock lock(logger_mutex);
        glogger << group(name_) << "> " << notification.ShortDebugString() << std::endl;
    }
    
    
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


void Daemon::ConnectedClient::process_publish(const proto::NotificationToServer& msg_in)
{
    
    // enforce thread-safety
    const TypeNameSubscribersMap& type_map = subscriptions;
    TypeNameSubscribersMap::const_iterator it = type_map.find(msg_in.embedded_msg().type());  
    if(it != type_map.end())
        process_publish_name(msg_in, it->second);
    
    
    // finally publish this to the SQL database
    boost::mutex::scoped_lock lock(dbo_mutex);
    DBOManager::get_instance()->add_message
        (msg_in.embedded_msg().type(),
         msg_in.embedded_msg().body());
}


void Daemon::ConnectedClient::process_publish_name(const proto::NotificationToServer& msg_in,
                                                   const NameSubscribersMap& name_map)
{

    if(!msg_in.embedded_msg().has_name())
    {
        // send to all subscribers with this type
        typedef std::pair<std::string, Subscribers> P;
        BOOST_FOREACH(const P&p, name_map)
            process_publish_subscribers(msg_in, p.second);
    }
    else
    {
        // send to those subscribed to your name
        NameSubscribersMap::const_iterator it_incoming_name = name_map.find(msg_in.embedded_msg().name());  
        if(it_incoming_name != name_map.end())
            process_publish_subscribers(msg_in,
                                        it_incoming_name->second);

        // ... and those subscribed to all names
        NameSubscribersMap::const_iterator it_all_name = name_map.find("");  
        if(it_all_name != name_map.end())
            process_publish_subscribers(msg_in,
                                        it_all_name->second);
    }
}

void Daemon::ConnectedClient::process_publish_subscribers(const proto::NotificationToServer& msg_in,
                                                          const Subscribers& subscribers)
{
    // prepare outgoing message
    proto::NotificationToClient msg_out;
    msg_out.mutable_embedded_msg()->CopyFrom(msg_in.embedded_msg());
    msg_out.set_notification_type
        (proto::NotificationToClient::INCOMING_MESSAGE);    
    
    // send it to everyone on the subscription list
    BOOST_FOREACH(const shared_ptr<ConnectedClient>& cc, subscribers)
    {
        // don't send mail back to yourself!
        if(cc != shared_from_this())
        {
            typedef boost::unordered_map< shared_ptr<ConnectedClient>, shared_ptr<message_queue> > QMap;
            
            QMap::iterator it = open_queues_.find(cc);
            
            if(it == open_queues_.end())
            {
                std::pair<QMap::iterator,bool> p =
                    open_queues_.insert
                    (std::make_pair
                     (cc, shared_ptr<message_queue>
                      (new message_queue
                       (boost::interprocess::open_only,
                        std::string(FROM_SERVER_QUEUE_PREFIX
                                    + cc->name()).c_str()))));
                it = p.first;
            }
                 
            if(try_send(*(it->second), msg_out))
            {
                if(!glogger.quiet())
                {
                    boost::mutex::scoped_lock lock(logger_mutex);
                    glogger << group(name_) << "sent message to " << cc->name() << std::endl;
                }
            }
            else
            {
                if(!glogger.quiet())
                {
                    boost::mutex::scoped_lock lock(logger_mutex);
                    glogger << group(name_) << "failed to send message to " << cc->name() << std::endl;
                }
            }             
        }
    }
}



void Daemon::ConnectedClient::process_subscribe(const proto::NotificationToServer& msg_in)
{
    boost::mutex::scoped_lock lock(subscription_mutex);

    subscriptions[msg_in.embedded_msg().type()][msg_in.embedded_msg().name()].insert(shared_from_this());
    
    if(!glogger.quiet())
    {
        boost::mutex::scoped_lock lock(logger_mutex);
        glogger << group(name_) << "subscribed to " << msg_in.embedded_msg().type() << std::endl;
    }
    
}

