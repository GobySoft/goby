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
#include <boost/format.hpp>

#include "goby/core/proto/interprocess_notification.pb.h"
#include "goby/core/proto/option_extensions.pb.h"
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

goby::core::proto::Config Daemon::cfg_;
std::string Daemon::self_name_;

// how long a client can be quiet before we ask for a heartbeat
const boost::posix_time::time_duration Daemon::HEARTBEAT_INTERVAL =
    boost::posix_time::seconds(2);

// how long a client can be quiet before we send it to the morgue
const boost::posix_time::time_duration Daemon::DEAD_INTERVAL =
    Daemon::HEARTBEAT_INTERVAL + Daemon::HEARTBEAT_INTERVAL;


int Daemon::argc_ = 0;
char** Daemon::argv_ = 0;

Daemon::Daemon()
    : active_(true),
      dbo_manager_(DBOManager::get_instance())
{
    if(!ConfigReader::read_cfg(argc_, argv_,
                               &cfg_,
                               &application_name_,
                               &self_name_,
                               &command_line_map_))
    {
        throw(std::runtime_error("did not successfully read configuration"));
    }
    
    {
        boost::mutex::scoped_lock lock(logger_mutex);

        switch(cfg_.verbosity())
        {
            case proto::Config::quiet:
            case proto::Config::warning:
                break;
            case proto::Config::verbose:
                glogger.add_stream("verbose", &std::cout);
                break;
            case proto::Config::gui:
                glogger.add_stream("scope", &std::cout);
                break;
        }
        
        
        fout_.open("gobyd.txt");
        glogger.add_stream("verbose", &fout_);
        
        glogger.name(application_name_);
        
        // nocolor, red, lt_red, green, lt_green, yellow,  lt_yellow, blue, lt_blue, magenta, lt_magenta, cyan, lt_cyan, white, lt_white
        glogger.add_group("connect", ">", "lt_magenta", "connections");
        glogger.add_group("disconnect", "<", "lt_blue", "disconnections");
    }

    
    glogger << cfg_ << std::endl;
    
    message_queue::remove(name_connect_listen(self_name_).c_str());    

    listen_queue_ = boost::shared_ptr<message_queue>
        (new message_queue(
            boost::interprocess::create_only,
            name_connect_listen(self_name_).c_str(),
            MAX_NUM_MSG,
            MAX_MSG_BUFFER_SIZE));
    
    {        
        boost::mutex::scoped_lock lock(dbo_mutex);

        
        try
        {
            cfg_.mutable_log()->mutable_sqlite()->set_path(
                format_filename(cfg_.log().sqlite().path()));
            
            dbo_manager_->connect(std::string(cfg_.log().sqlite().path()));
            
        }
        catch(std::exception& e)
        {
            cfg_.mutable_log()->mutable_sqlite()->clear_path();
            cfg_.mutable_log()->mutable_sqlite()->set_path(
                format_filename(cfg_.log().sqlite().path()));
            
            glogger << warn << "db connection failed: " << e.what() << std::endl;
            std::string default_file = cfg_.log().sqlite().path();
            
            glogger << "trying again with defaults: " << default_file << std::endl;
            dbo_manager_->connect(default_file);
        }
        
        
        dbo_manager_->set_logger(&glogger);
        dbo_manager_->add_flex_groups(glogger);

        // add files of core static types for dynamic messages
        // #include <google/protobuf/descriptor.pb.h>
        dbo_manager_->add_file(google::protobuf::FileDescriptorProto::descriptor());
        // #include "goby/core/proto/option_extensions.pb.h"
        dbo_manager_->add_file(proto::extend::descriptor());
        // #include "goby/core/proto/interprocess_notification.pb.h"
        dbo_manager_->add_file(proto::Notification::descriptor());
        // #include "goby/core/proto/config.pb.h"
        dbo_manager_->add_file(proto::Config::descriptor());
    }

    
    t_start_ = goby_time();
    t_next_db_commit_ = boost::posix_time::second_clock::universal_time() + boost::posix_time::seconds(1);
}

Daemon::~Daemon()
{
    message_queue::remove(name_connect_listen(self_name_).c_str());
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
    
    message_queue response_queue(
        boost::interprocess::open_only,
        name_connect_response(self_name_, name).c_str());

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
        // send configuration along
        notification_.mutable_embedded_msg()->set_type(cfg_.GetDescriptor()->full_name());
        google::protobuf::io::StringOutputStream os(
            notification_.mutable_embedded_msg()->mutable_body());
        cfg_.SerializeToZeroCopyStream(&os);

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

    send(response_queue, notification_, buffer_, sizeof(buffer_));
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


std::string Daemon::format_filename(const std::string& in)
{   
    boost::format f(in);
    // don't thrown if user doesn't need some or all of format fields
    f.exceptions(boost::io::all_error_bits^(
                     boost::io::too_many_args_bit | boost::io::too_few_args_bit));
    f % goby::util::goby_file_timestamp();
    return f.str();
}


