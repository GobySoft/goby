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

#include "goby/protobuf/interprocess_notification.pb.h"
#include "goby/core/core_constants.h"
#include "goby/core/libcore/message_queue_util.h"

#include "gobyd.h"

using boost::interprocess::message_queue;
using boost::shared_ptr;

using namespace goby::core;
using namespace goby::util;
using goby::util::logger_lock::lock;


Daemon::ConnectedClient::ConnectedClient(const std::string& name)
    : name_(name),
      active_(true),
      t_connected_(goby_time()),
      t_last_active_(t_connected_),
      t_next_heartbeat_(boost::posix_time::second_clock::universal_time() +
                        boost::posix_time::seconds(1))
{
    {
        boost::mutex::scoped_lock lock(glogger().mutex());
        glogger().add_group(name_, Colors::blue, name_);
    }
    
    

    message_queue::remove(name_to_server(cfg_.self().name(), name_).c_str());
    message_queue::remove(name_from_server(cfg_.self().name(), name_).c_str());
    
    message_queue to_server_queue(boost::interprocess::create_only,
                                  name_to_server(cfg_.self().name(), name_).c_str(),
                                  MAX_NUM_MSG,
                                  MAX_MSG_BUFFER_SIZE);
        
    
    message_queue from_server_queue(boost::interprocess::create_only,
                                    name_from_server(cfg_.self().name(), name_).c_str(),
                                    MAX_NUM_MSG,
                                    MAX_MSG_BUFFER_SIZE);

    glogger(lock) << group(name_) << "ConnectedClient constructed" << std::endl << unlock;
}

Daemon::ConnectedClient::~ConnectedClient()
{
    message_queue::remove(name_to_server(cfg_.self().name(), name_).c_str());
    message_queue::remove(name_from_server(cfg_.self().name(), name_).c_str());

    glogger(lock) << group(name_) << "ConnectedClient destructed" << std::endl << unlock;
}


void Daemon::ConnectedClient::run()
{
    message_queue to_server_queue(boost::interprocess::open_only,
                                  name_to_server(cfg_.self().name(), name_).c_str());
    message_queue from_server_queue(boost::interprocess::open_only,
                                    name_from_server(cfg_.self().name(), name_).c_str());
    
    while(active_)
    {
        if(timed_receive(to_server_queue,
                         notification_,
                         t_next_heartbeat_,
                         buffer_,
                         &buffer_msg_size_))
        {
            glogger(lock) << group(name_) << "> notification: "
                          << notification_.GetReflection()->GetEnum(notification_, notification_.GetDescriptor()->FindFieldByName("notification_type"))->name()
                          << std::endl << unlock;
            process_notification();
        }
        else
        {
            boost::posix_time::ptime now = goby_time();
            glogger(lock) << group(name_) << "now " <<  now << std::endl << unlock;
            
            if(now > t_last_active_ + DEAD_INTERVAL)
            {
                glogger(lock) << group(name_) << warn << "disconnecting self"
                                 << std::endl << unlock;
                disconnect_self();
            } 
            else if(now > t_last_active_ + HEARTBEAT_INTERVAL)
            {
                glogger(lock) << group(name_) << "< sending HEARTBEAT"
                                 << std::endl << unlock;
                // send request for heartbeat
                notification_.Clear();
                notification_.set_notification_type(proto::Notification::HEARTBEAT);
                if(try_send(from_server_queue, notification_, buffer_, sizeof(buffer_)))
                    glogger(lock) << group(name_) << "< sent heartbeat"
                                     << std::endl << unlock;
                else
                    glogger(lock) << group(name_) << warn << "could not send heartbeat"
                                     << std::endl << unlock;
            }
            t_next_heartbeat_ += HEARTBEAT_INTERVAL;
        }
    }
}

void Daemon::ConnectedClient::process_notification()
{
    // client sent us a message, so it's still alive
    t_last_active_ = goby_time();
    glogger(lock) << group(name_) << "last_active " << t_last_active_ << std::endl << unlock;
    
    // if the client sent along a new type, add it to our database
    for(int i = 0, n = notification_.file_descriptor_proto_size(); i < n; ++i)
    {
        boost::mutex::scoped_lock lock(dbo_mutex);
        DBOManager::get_instance()->add_file(notification_.file_descriptor_proto(i));
    }

    if(notification_.file_descriptor_proto_size())
    {
        boost::mutex::scoped_lock lock(dbo_mutex);
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
    glogger(lock) << group(name_) << "publishing: "
                  << notification_.embedded_msg().type()
                  << notification_.embedded_msg().filter()
                  << std::endl << unlock;
    
    shared_ptr<google::protobuf::Message> parsed_embedded_msg = 
        DBOManager::new_msg_from_name(notification_.embedded_msg().type());
    
    parsed_embedded_msg->ParseFromString(notification_.embedded_msg().body());
    
    // enforce read only thread-safety
    subscription_mutex.lock_shared();
    const boost::unordered_multimap<std::string, Subscriber>& const_subscriptions =
        subscriptions;
    
    typedef boost::unordered_multimap<std::string, Subscriber>::const_iterator const_iterator;
    std::pair<const_iterator, const_iterator> equal_it_pair =
        const_subscriptions.equal_range(notification_.embedded_msg().type());
    
    process_publish_subscribers(equal_it_pair.first,
                                equal_it_pair.second,
                                *parsed_embedded_msg);
    subscription_mutex.unlock_shared();

    // add to the database
    boost::mutex::scoped_lock lock(dbo_mutex);
    DBOManager::get_instance()->add_message(parsed_embedded_msg);
}

void Daemon::ConnectedClient::process_publish_subscribers(
    boost::unordered_multimap<std::string, Subscriber>::const_iterator begin_it,
    boost::unordered_multimap<std::string, Subscriber>::const_iterator end_it,
    const google::protobuf::Message& parsed_embedded_msg)
{    
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
                                               name_from_server(cfg_.self().name(),
                                                                cc->name()).c_str()))));
                it = p.first;
            }
            
            if(!timed_send(*(it->second), buffer_, buffer_msg_size_, goby_time() + PUBLISH_WAIT))
            {
                glogger(lock) << group(name_) << warn << "failed to send "
                              << notification_.embedded_msg().type() << " to "
                              << cc->name() << std::endl << unlock;
                
            }
            else
            {
                glogger(lock) << group(name_) << "\tsent " << notification_.embedded_msg().type() << " to " << cc->name() << std::endl << unlock;
            }
        }
    }
}

void Daemon::ConnectedClient::process_subscribe()
{
    subscription_mutex.lock();
    subscriptions.insert(
        make_pair(notification_.embedded_msg().type(),
                  Subscriber(shared_from_this(), notification_.embedded_msg().filter())));
    subscription_mutex.unlock();

    glogger(lock) << group(name_) << "subscribed to " << notification_.embedded_msg() << std::endl << unlock;
}


void Daemon::ConnectedClient::disconnect_self()
{
    // deceased client - disconnect ourselves
    message_queue listen_queue(
        boost::interprocess::open_only,
        name_connect_listen(cfg_.self().name()).c_str());
    notification_.Clear();
    notification_.set_notification_type(proto::Notification::DISCONNECT_REQUEST);
    notification_.set_application_name(name_);
    send(listen_queue, notification_, buffer_, sizeof(buffer_));
    stop();
}
