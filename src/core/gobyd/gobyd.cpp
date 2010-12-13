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
#include "goby/core/proto/app_base_config.pb.h"

#include "goby/util/logger.h"

#include "gobyd.h"

using boost::interprocess::message_queue;
using boost::shared_ptr;

using namespace goby::core;
using namespace goby::util;
using goby::util::logger_lock::lock;

boost::mutex Daemon::dbo_mutex;
boost::shared_mutex Daemon::subscription_mutex;

boost::unordered_multimap<std::string, Daemon::Subscriber > Daemon::subscriptions;

goby::core::proto::Config Daemon::cfg_;

// how long a client can be quiet before we ask for a heartbeat
const boost::posix_time::time_duration Daemon::HEARTBEAT_INTERVAL =
    boost::posix_time::seconds(1);

// how long a client can be quiet before we send it to the morgue
const boost::posix_time::time_duration Daemon::DEAD_INTERVAL =
    boost::posix_time::seconds(3);

// how long to try to send a message to any given client
// before giving up
const boost::posix_time::time_duration Daemon::PUBLISH_WAIT =
    boost::posix_time::milliseconds(10);

int Daemon::argc_ = 0;
char** Daemon::argv_ = 0;

Daemon::Daemon()
    : active_(true),
      dbo_manager_(DBOManager::get_instance()),
      t_start_(goby_time()),
      t_next_db_commit_(boost::posix_time::second_clock::universal_time() +
                        boost::posix_time::seconds(1))
{    
    init_cfg();
    init_logger();    
    init_sql();
    init_listener();
}

Daemon::~Daemon()
{
    message_queue::remove(name_connect_listen(cfg_.self().name()).c_str());
}

void Daemon::init_cfg()
{
    boost::program_options::variables_map var_map;
    boost::program_options::options_description od("Allowed options");
    try
    {
        // determine the application name
        std::string app_name;
        ConfigReader::read_cfg(argc_, argv_, &cfg_, &app_name, &od, &var_map);        
        cfg_.mutable_base()->set_app_name(app_name);

        // determine the name of this platform
        std::string self_name;
        if(var_map.count("platform_name"))
            self_name = var_map["platform_name"].as<std::string>();
        else if(cfg_.self().has_name() && cfg_.base().has_platform_name() &&
           cfg_.base().platform_name() != cfg_.self().name())
            throw(ConfigException("base.platform and self.name both specified and do not match"));
        else if(cfg_.self().has_name())
            self_name = cfg_.self().name();
        else if(cfg_.base().has_platform_name())
            self_name = cfg_.base().platform_name();
        else
            throw(ConfigException("missing base.platform_name (self.name)"));                
        cfg_.mutable_base()->set_platform_name(self_name);
        cfg_.mutable_self()->set_name(self_name);

        // merge the rest of the config shared with ApplicationBase
        ConfigReader::merge_app_base_cfg(cfg_.mutable_base(), var_map);
    }
    catch(ConfigException& e)
    {
        std::cerr << od << "\n";
        
        if(e.error())
            std::cerr << "Problem parsing command-line configuration: \n" << e.what() << "\n";
        
        throw;
    }
}

void Daemon::init_logger()
{
    boost::mutex::scoped_lock lock(glogger().mutex());
    
    glogger().set_name(cfg_.base().app_name());
    switch(cfg_.base().verbosity())
    {
        case AppBaseConfig::quiet:
            glogger().add_stream(Logger::quiet, &std::cout);
            break;            
        case AppBaseConfig::warn:
            glogger().add_stream(Logger::warn, &std::cout);
            break;
        case AppBaseConfig::verbose:
            glogger().add_stream(Logger::verbose, &std::cout);
            break;
        case AppBaseConfig::debug:
            glogger().add_stream(Logger::debug, &std::cout);
            break;
        case AppBaseConfig::gui:
            glogger().add_stream(Logger::gui, &std::cout);
            break;    
    }
    
    fout_.open("gobyd.txt");
    glogger().add_stream(Logger::verbose, &fout_);

// nocolor, red, lt_red, green, lt_green, yellow,  lt_yellow, blue, lt_blue, magenta, lt_magenta, cyan, lt_cyan, white, lt_white
    glogger().add_group("connect", Colors::lt_magenta, "connections");
    glogger().add_group("disconnect", Colors::lt_blue, "disconnections");

    glogger() << cfg_ << std::endl;
}

void Daemon::init_sql()
{
    boost::mutex::scoped_lock db_lock(dbo_mutex);
    
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
        
        glogger(lock) << warn << "db connection failed: "
                      << e.what() << std::endl << unlock;
        std::string default_file = cfg_.log().sqlite().path();
            
        glogger(lock) << "trying again with defaults: "
                         << default_file << std::endl << unlock;

        dbo_manager_->connect(default_file);
    }    
    
    // add files of core static types for dynamic messages
    // order matters! must add a proto before adding any for which it is an import
    // #include <google/protobuf/descriptor.pb.h>
    dbo_manager_->add_file(google::protobuf::FileDescriptorProto::descriptor());

    // #include "goby/core/proto/option_extensions.pb.h"
    // dbo_manager_->add_file(::extend::descriptor());
    // #include "goby/core/proto/interprocess_notification.pb.h"
    // dbo_manager_->add_file(proto::Notification::descriptor());
    // #include "goby/core/proto/app_base_config.pb.h"
    // dbo_manager_->add_file(::AppBaseConfig::descriptor());
    // #include "goby/core/proto/config.pb.h"
    // dbo_manager_->add_file(proto::Config::descriptor());
}

void Daemon::init_listener()
{
    message_queue::remove(name_connect_listen(cfg_.self().name()).c_str());    

    listen_queue_ = boost::shared_ptr<message_queue>
        (new message_queue(
            boost::interprocess::create_only,
            name_connect_listen(cfg_.self().name()).c_str(),
            MAX_NUM_MSG,
            MAX_MSG_BUFFER_SIZE));   
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
            glogger(lock) << "> " << notification_.DebugString()
                             << std::endl << unlock;
            
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
            boost::mutex::scoped_lock lock(dbo_mutex);
            dbo_manager_->commit();
            // TODO(tes): make this configurable
            t_next_db_commit_ += boost::posix_time::seconds(1);            
        }
    }
}

void Daemon::connect()
{
    const std::string& name = notification_.application_name();

    message_queue response_queue(boost::interprocess::open_only,
                                 name_connect_response(cfg_.self().name(), name).c_str());

    if(!clients_.count(name))
    {
        shared_ptr<ConnectedClient> client(new ConnectedClient(name));
        shared_ptr<boost::thread> thread(
            new boost::thread(boost::bind(&Daemon::ConnectedClient::run, client)));
        
        clients_[name] = client;
        client_threads_[name] = thread;

        glogger(lock) << group("connect") << "`" << name << "` has connected"
                         << std::endl << unlock;
        
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
        glogger(lock) << group("connect") << warn
                         << "cannot connect: application with name `"
                         << name << "` is already connected" << std::endl << unlock;
        
        notification_.Clear();
        notification_.set_notification_type(proto::Notification::CONNECTION_DENIED);
        notification_.set_explanation("name in use");
    }
    
    send(response_queue, notification_, buffer_, sizeof(buffer_));
}

void Daemon::disconnect()
{
    const std::string& name = notification_.application_name();

    glogger(lock) << "disconnecting " << name << std::endl << unlock;
    
    if(clients_.count(name))
    {
        glogger(lock) << group("disconnect") << "stopping " << name << std::endl << unlock;
        clients_[name]->stop();
        glogger(lock) << group("disconnect") << "waiting for join " << name << std::endl << unlock;
        glogger(lock) << group("disconnect") << "joinable? " << client_threads_[name]->joinable() << std::endl << unlock;
        client_threads_[name]->join();
        
        glogger(lock) << group("disconnect") << "joined" << std::endl << unlock;
        
        subscription_mutex.lock();
        // remove entries from subscription database
        typedef boost::unordered_multimap<std::string, Subscriber>::iterator iterator;

        glogger(lock) << group("disconnect") << "removing subscriptions" << std::endl << unlock;

        for(iterator it = subscriptions.begin(), n = subscriptions.end(); it != n;)
        {
            iterator erase_it = it++;
            
            if(erase_it->second.client() == clients_[name])
                subscriptions.erase(erase_it);
        }
        subscription_mutex.unlock();
        
        glogger(lock) << group("disconnect") << "erasing clients" << std::endl << unlock;

        clients_.erase(name);
        client_threads_.erase(name);

        glogger(lock) << group("disconnect")
                << "`" << name << "` has disconnected" << std::endl << unlock;
        
    }
    else
    {
        glogger(lock) << group("disconnect") << warn
                << "cannot disconnect: no application with name `"
                << name << "` connected" << std::endl << unlock;
    
    }
}


std::string Daemon::format_filename(const std::string& in)
{   
    boost::format f(in);
    // don't thrown if user doesn't need some or all of format fields
    f.exceptions(boost::io::all_error_bits^(
                     boost::io::too_many_args_bit | boost::io::too_few_args_bit));
    f % cfg_.self().name() % goby::util::goby_file_timestamp();
    return f.str();
}


