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

#include "goby/core/proto/interprocess_notification.pb.h"
#include "goby/core/core_constants.h"
#include "goby/core/libcore/message_queue_util.h"

#include "gobyd.h"

using boost::interprocess::message_queue;
using boost::shared_ptr;

using namespace goby::core;
using namespace goby::util;

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
    
    message_queue::remove(
        name_to_server(self_name_, name_).c_str());
        
    message_queue::remove(
        name_from_server(self_name_, name_).c_str());
    
    message_queue to_server_queue(boost::interprocess::create_only,
                                  name_to_server(self_name_, name_).c_str(),
                                  MAX_NUM_MSG,
                                  MAX_MSG_BUFFER_SIZE);
        
    
    message_queue from_server_queue(boost::interprocess::create_only,
                                    name_from_server(self_name_, name_).c_str(),
                                    MAX_NUM_MSG,
                                    MAX_MSG_BUFFER_SIZE);
}

Daemon::ConnectedClient::~ConnectedClient()
{
    message_queue::remove(name_to_server(self_name_, name_).c_str());
    message_queue::remove(name_from_server(self_name_, name_).c_str());

    if(!glogger.quiet())
    {
        boost::mutex::scoped_lock lock(logger_mutex);
        glogger << group(name_) << "ConnectedClient destructed" << std::endl;
    }
    

}


void Daemon::ConnectedClient::run()
{
    message_queue to_server_queue(boost::interprocess::open_only,
                                  name_to_server(self_name_, name_).c_str());
    message_queue from_server_queue(boost::interprocess::open_only,
                                    name_from_server(self_name_, name_).c_str());
    
    while(active_)
    {
        if(timed_receive(to_server_queue,
                         notification_,
                         t_next_heartbeat_,
                         buffer_,
                         &buffer_msg_size_))
        {
            process_notification();
        }
        else
        {
            boost::posix_time::ptime now = goby_time();
            
            if(now > t_last_active_ + DEAD_INTERVAL)
            {
                // deceased client - disconnect ourselves
                message_queue listen_queue(
                    boost::interprocess::open_only,
                    name_connect_listen(self_name_).c_str());
                notification_.Clear();
                notification_.set_notification_type(proto::Notification::DISCONNECT_REQUEST);
                notification_.set_application_name(name_);
                send(listen_queue, notification_, buffer_, sizeof(buffer_));
                stop();
            } 
            else if(now > t_last_active_ + HEARTBEAT_INTERVAL)
            {
                // send request for heartbeat
                notification_.Clear();
                notification_.set_notification_type(proto::Notification::HEARTBEAT);
                send(from_server_queue, notification_, buffer_, sizeof(buffer_));
            }
            
            t_next_heartbeat_ += HEARTBEAT_INTERVAL;
        }        
    }
}

void Daemon::ConnectedClient::process_notification()
{
    // if(!glogger.quiet())
    // {
    //     boost::mutex::scoped_lock lock(logger_mutex);
    //     glogger << group(name_) << "> " << notification_.DebugString() << std::endl;
    // }
    
    
    // client sent us a message, so it's still alive
    t_last_active_ = goby_time();

    // if the client sent along a new type, add it to our database
    if(notification_.has_file_descriptor_proto())
    {
        //TODO(tes): Make liblogger thread safe and thus remove logger_mutex
        // from this application
        boost::mutex::scoped_lock lock1(logger_mutex);
        boost::mutex::scoped_lock lock2(dbo_mutex);
        DBOManager::get_instance()->add_file(notification_.file_descriptor_proto());
        DBOManager::get_instance()->add_type(notification_.embedded_msg().type());
    }
    
    switch(notification_.notification_type())
    {
        case proto::Notification::PUBLISH_REQUEST: 
            process_publish();
            break;
            
        case proto::Notification::SUBSCRIBE_REQUEST:
            process_subscribe();
            break;

        case proto::Notification::HEARTBEAT:
        default: return;
    }
}


void Daemon::ConnectedClient::process_publish()
{    
    shared_ptr<google::protobuf::Message> parsed_embedded_msg = 
        DBOManager::new_msg_from_name(notification_.embedded_msg().type());
    
    parsed_embedded_msg->ParseFromString(notification_.embedded_msg().body());    

    // if(!glogger.quiet())
    // {
    //     boost::mutex::scoped_lock lock1(logger_mutex);
    //     glogger << group(name_) << "embedded_message: " << *parsed_embedded_msg << std::endl;
    // }
    
    // enforce thread-safety
    const boost::unordered_multimap<std::string, Subscriber>& const_subscriptions =
        subscriptions;
    
    typedef boost::unordered_multimap<std::string, Subscriber>::const_iterator const_iterator;
    std::pair<const_iterator, const_iterator> equal_it_pair =
        const_subscriptions.equal_range(notification_.embedded_msg().type());
    
    process_publish_subscribers(equal_it_pair.first,
                                equal_it_pair.second,
                                *parsed_embedded_msg);

    // add to the database
    boost::mutex::scoped_lock lock1(logger_mutex);
    boost::mutex::scoped_lock lock2(dbo_mutex);
    DBOManager::get_instance()->add_message(parsed_embedded_msg);
}

void Daemon::ConnectedClient::process_publish_subscribers(
    boost::unordered_multimap<std::string, Subscriber>::const_iterator begin_it,
    boost::unordered_multimap<std::string, Subscriber>::const_iterator end_it,
    const google::protobuf::Message& parsed_embedded_msg)
{
    if(!glogger.quiet())
    {
        boost::mutex::scoped_lock lock(logger_mutex);
        glogger << group(name_) << "distributing message: " << parsed_embedded_msg << std::endl;
    }
    
    // send it to everyone on the subscription list
    for(boost::unordered_multimap<std::string, Subscriber>::const_iterator it = begin_it;
        it != end_it;
        ++it)
    {
        const Subscriber& subscriber = it->second;
        const boost::shared_ptr<ConnectedClient> cc = subscriber.client();
        
        if(cc != shared_from_this() //  not sending message back to publisher
           && subscriber.check_filter(parsed_embedded_msg)) // filter does not exclude message
        {
            typedef boost::unordered_map<shared_ptr<ConnectedClient>, shared_ptr<message_queue> > QMap;            
            QMap::iterator it = open_queues_.find(cc);
            
            if(it == open_queues_.end())
            {
                std::pair<QMap::iterator,bool> p =
                    open_queues_.insert(
                        std::make_pair(cc,
                                       shared_ptr<message_queue>(
                                           new message_queue(
                                               boost::interprocess::open_only,
                                               name_from_server(self_name_,
                                                                cc->name()).c_str()))));
                it = p.first;
            }
            
            // last thing we received is what we want to send out
            // so don't reserialize this...
            if(try_send(*(it->second), buffer_, buffer_msg_size_))
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



void Daemon::ConnectedClient::process_subscribe()
{
    boost::mutex::scoped_lock lock(subscription_mutex);
    
    subscriptions.insert(
        make_pair(notification_.embedded_msg().type(),
                  Subscriber(shared_from_this(), notification_.embedded_msg().filter())));
    
    if(!glogger.quiet())
    {
        boost::mutex::scoped_lock lock(logger_mutex);
        glogger << group(name_) << "subscribed to " << notification_.embedded_msg() << std::endl;
    }
    
}
