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

boost::mutex Daemon::dbo_mutex;
boost::mutex Daemon::subscription_mutex;
boost::mutex Daemon::logger_mutex;
goby::util::FlexOstream Daemon::glogger;

boost::unordered_multimap<std::string, Daemon::Subscriber > Daemon::subscriptions;
goby::core::Daemon* goby::core::Daemon::inst_ = 0;

// how long a client can be quiet before we ask for a heartbeat
const boost::posix_time::time_duration Daemon::HEARTBEAT_INTERVAL =
    boost::posix_time::seconds(2);

// how long a client can be quiet before we send it to the morgue
const boost::posix_time::time_duration Daemon::DEAD_INTERVAL =
    Daemon::HEARTBEAT_INTERVAL + Daemon::HEARTBEAT_INTERVAL;


// singleton class, use this to get pointer
goby::core::Daemon* goby::core::Daemon::get_instance()
{
    if(!inst_) inst_ = new goby::core::Daemon();
    return(inst_);
}

void goby::core::Daemon::shutdown()
{
    if(inst_) delete inst_;
}

int main()
{
    Daemon::get_instance()->run();
    Daemon::shutdown();
    DBOManager::shutdown();
    google::protobuf::ShutdownProtobufLibrary();
    return 0;
}


Daemon::Daemon()
    : active_(true),
      dbo_manager_(DBOManager::get_instance())
{
    message_queue::remove(CONNECT_LISTEN_QUEUE.c_str());

    listen_queue_ = boost::shared_ptr<message_queue>
        (new message_queue(
            boost::interprocess::create_only,
            CONNECT_LISTEN_QUEUE.c_str(),
            MAX_NUM_MSG,
            MAX_MSG_BUFFER_SIZE));
    
    {        
        boost::mutex::scoped_lock lock(dbo_mutex);
        
        dbo_manager_->connect("gobyd_test.db");
        dbo_manager_->set_logger(&glogger);
        dbo_manager_->add_flex_groups(glogger);
    }

    {
        boost::mutex::scoped_lock lock(logger_mutex);

        fout_.open("gobyd.txt");
        glogger.add_stream("verbose", &fout_);        
        
        glogger.name("gobyd");
        
        // nocolor, red, lt_red, green, lt_green, yellow,  lt_yellow, blue, lt_blue, magenta, lt_magenta, cyan, lt_cyan, white, lt_white
        glogger.add_group("connect", ">", "lt_magenta", "connections");
        glogger.add_group("disconnect", "<", "lt_blue", "disconnections");
    }
    
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
        if(timed_receive(*listen_queue_,
                         notification_,
                         t_next_db_commit_,
                         buffer_,
                         &buffer_msg_size_))
        {
            // if(!glogger.quiet())
            // {
            //     boost::mutex::scoped_lock lock(logger_mutex);
            //     glogger << "> " << notification_.DebugString() << std::endl;
            // }
            
            switch(notification_.notification_type())
            {
                case proto::Notification::CONNECT_REQUEST:
                    connect();
                    break;
                case proto::Notification::DISCONNECT_REQUEST:
                    disconnect();
                    break;

                    // ignore other requests on the connection line
                default: break;
            }
        }
        else
        {    
            // write database entries to actual db
            boost::mutex::scoped_lock lock1(logger_mutex);
            boost::mutex::scoped_lock lock2(dbo_mutex);
            dbo_manager_->commit();
            // TODO(tes): make this configurable
            t_next_db_commit_ += boost::posix_time::seconds(1);            
        }
    }
}

void Daemon::connect()
{
    const std::string& name = notification_.application_name();

    std::cout << "name: " << name << std::endl;
    
    message_queue connect_response_queue
        (boost::interprocess::open_only,
         std::string(CONNECT_RESPONSE_QUEUE_PREFIX + name).c_str());

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
        
        notification_.Clear();
        notification_.set_notification_type(proto::Notification::CONNECTION_ACCEPTED);
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
        
        notification_.Clear();
        notification_.set_notification_type(proto::Notification::CONNECTION_DENIED);
        notification_.set_explanation("name in use");
    }

    send(connect_response_queue, notification_, buffer_, sizeof(buffer_));
}

void Daemon::disconnect()
{
    const std::string& name = notification_.application_name();
    
    if(clients_.count(name))
    {
        clients_[name]->stop();
        client_threads_[name]->join();        
        
        boost::mutex::scoped_lock lock(subscription_mutex);
        // remove entries from subscription database
        typedef boost::unordered_multimap<std::string, Subscriber>::iterator iterator;
        for(iterator it = subscriptions.begin(), n = subscriptions.end(); it != n;)
        {
            iterator erase_it = it++;

            if(erase_it->second.client() == clients_[name])
                subscriptions.erase(erase_it);
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



