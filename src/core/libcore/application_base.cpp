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

#include <boost/program_options.hpp>

#include <Wt/Dbo/Dbo>
#include <Wt/Dbo/Query>
#include <Wt/Dbo/backend/Sqlite3>
#include <Wt/Dbo/Exception>

#include "goby/util/time.h"
#include "goby/core/libcore/configuration_reader.h"
#include "goby/core/libdbo/dbo_manager.h"
#include "goby/protobuf/app_base_config.pb.h"
#include "goby/protobuf/header.pb.h"

#include "application_base.h"

using namespace goby::core;
using namespace goby::util;
using boost::interprocess::message_queue;
using boost::shared_ptr;

const boost::posix_time::time_duration goby::core::ApplicationBase::CONNECTION_WAIT_INTERVAL =
    boost::posix_time::seconds(1);

int goby::core::ApplicationBase::argc_ = 0;
char** goby::core::ApplicationBase::argv_ = 0;

goby::core::ApplicationBase::ApplicationBase(
    google::protobuf::Message* cfg /*= 0*/,
    bool skip_cfg_checks /* = false */)
    : loop_period_(boost::posix_time::milliseconds(100)),
      connected_(false),
      db_connection_(0),
      db_session_(0)
{
    //
    // read the configuration
    //
    boost::program_options::options_description od("Allowed options");
    boost::program_options::variables_map var_map;
    try
    {
        std::string application_name;
        ConfigReader::read_cfg(argc_, argv_, cfg, &application_name, &od, &var_map);
        base_cfg_.set_app_name(application_name);

        // extract the AppBaseConfig assuming the user provided it in their configuration
        // .proto file
        if(cfg)
        {
            const google::protobuf::Descriptor* desc = cfg->GetDescriptor();
            for (int i = 0, n = desc->field_count(); i < n; ++i)
            {
                const google::protobuf::FieldDescriptor* field_desc = desc->field(i);
                if(field_desc->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE && field_desc->message_type() == ::AppBaseConfig::descriptor())
                {
                    base_cfg_.MergeFrom(cfg->GetReflection()->GetMessage(*cfg, field_desc));
                }
            }
        }

        
        // determine the name of this platform
        std::string self_name;
        if(var_map.count("platform_name"))
            self_name = var_map["platform_name"].as<std::string>();
        else if(base_cfg_.has_platform_name())
            self_name = base_cfg_.platform_name();
        else
            throw(ConfigException("missing platform_name"));
        base_cfg_.set_platform_name(self_name);
        
        // incorporate some parts of the AppBaseConfig that are common
        // with gobyd (e.g. Verbosity)
        ConfigReader::merge_app_base_cfg(&base_cfg_, var_map);
        
        set_loop_freq(base_cfg_.loop_freq());
    }
    catch(ConfigException& e)
    {
        if(!skip_cfg_checks)
        {
            // output all the available command line options
            std::cerr << od << "\n";
            if(e.error())
                std::cerr << "Problem parsing command-line configuration: \n"
                          << e.what() << "\n";
            
            throw;
        }
    }
    
    // set up the logger
    glogger().set_name(application_name());

    switch(base_cfg_.verbosity())
    {
        case AppBaseConfig::QUIET:
            glogger().add_stream(Logger::quiet, &std::cout);
            break;            
        case AppBaseConfig::WARN:
            glogger().add_stream(Logger::warn, &std::cout);
            break;
        case AppBaseConfig::VERBOSE:
            glogger().add_stream(Logger::verbose, &std::cout);
            break;
        case AppBaseConfig::DEBUG:
            glogger().add_stream(Logger::debug, &std::cout);
            break;
        case AppBaseConfig::GUI:
            glogger().add_stream(Logger::gui, &std::cout);
            break;    
    }

    if(base_cfg_.IsInitialized())
    {
        connect();

        // notify the server of our configuration for logging purposes
        if(cfg) publish(*cfg);
        
        // start up SQL
        db_connection_ = new Wt::Dbo::backend::Sqlite3(daemon_cfg_.log().sqlite().path());
        db_session_ = new Wt::Dbo::Session;
        db_session_->setConnection(*db_connection_);
    }
    
}

goby::core::ApplicationBase::~ApplicationBase()
{
    glogger() << debug <<"ApplicationBase destructing..." << std::endl;
    
    if(db_connection_) delete db_connection_;
    if(db_session_) delete db_session_;

    if(connected_) disconnect();
}

void goby::core::ApplicationBase::connect()
{
    try
    {
        glogger() << debug << "trying to connect..." <<  std::endl;
        
        // set up queues to wait for response from server
        message_queue::remove(name_connect_response(base_cfg_.platform_name(),
                                                    base_cfg_.app_name()).c_str());

        message_queue response_queue(
            boost::interprocess::create_only,
            name_connect_response(base_cfg_.platform_name(), base_cfg_.app_name()).c_str(),
            1, MAX_MSG_BUFFER_SIZE);
        
        // make connection
        message_queue listen_queue(boost::interprocess::open_only,
                                   name_connect_listen(base_cfg_.platform_name()).c_str());
        
        // populate server request for connection
        notification_.Clear();
        notification_.set_notification_type(proto::Notification::CONNECT_REQUEST);
        notification_.set_application_name(base_cfg_.app_name());

        // serialize and send the server request
        send(listen_queue, notification_, buffer_, sizeof(buffer_));  
        
        // wait for response
        if(!timed_receive(response_queue,
                          notification_,
                          goby_time() + CONNECTION_WAIT_INTERVAL,
                          buffer_,
                          &buffer_msg_size_))
        {
            // no response
            glogger() << die << "no response to connection request. make sure gobyd is alive."
                    <<  std::endl;        
        }        

        // good response
        if(notification_.notification_type() == proto::Notification::CONNECTION_ACCEPTED)
        {
            connected_ = true;
            glogger() << debug << "connection succeeded." <<  std::endl;
            daemon_cfg_.ParseFromString(notification_.embedded_msg().body());
            glogger() << debug <<"Daemon configuration: \n" << daemon_cfg_ << std::endl;
        }
        else // bad response
        {
            glogger() << die << "server did not connect us: " << "\n"
                    << notification_ << std::endl;
        }

        // remove this so that future applications with the same name can (try) to connect
        message_queue::remove(name_connect_response(base_cfg_.platform_name(),
                                                    base_cfg_.app_name()).c_str());

    }
    catch(boost::interprocess::interprocess_exception &ex)
    {
        // no queues set up by gobyd
        glogger() << die << "could not open message queue(s) with gobyd. check to make sure gobyd is alive. " << ex.what() << std::endl;
    }
    

    // open the queues that gobyd promises to create upon connection
    to_server_queue_ = shared_ptr<message_queue>(
        new message_queue(
            boost::interprocess::open_only,
            name_to_server(base_cfg_.platform_name(), base_cfg_.app_name()).c_str()));
    
    from_server_queue_ = shared_ptr<message_queue>(
        new message_queue(
            boost::interprocess::open_only,
            name_from_server(base_cfg_.platform_name(), base_cfg_.app_name()).c_str()));

    // we are started
    t_start_ = goby_time();
    // start the loop() on the next even second
    t_next_loop_ = boost::posix_time::second_clock::universal_time() +
        boost::posix_time::seconds(1);

}


void goby::core::ApplicationBase::disconnect()
{
    try
    {
        // make disconnection
        message_queue listen_queue(boost::interprocess::open_only,
                                   name_connect_listen(base_cfg_.platform_name()).c_str());
    
        // populate server request for connection
        notification_.Clear();
        notification_.set_notification_type(proto::Notification::DISCONNECT_REQUEST);
        notification_.set_application_name(base_cfg_.app_name());

        glogger() << debug <<notification_ << std::endl;
        
        // serialize and send the server request
        send(listen_queue, notification_, buffer_, sizeof(buffer_));
        connected_ = false;
    }
    catch(boost::interprocess::interprocess_exception &ex)
    {
        glogger() << warn << ex.what() << std::endl;
    }
}


void goby::core::ApplicationBase::run()
{
    // continue to run while we have a connection
    while(connected_)
    {
        // sit and wait on a message until the next time to call loop() is up
        if(timed_receive(*from_server_queue_,
                         notification_,
                         t_next_loop_,
                         buffer_,
                         &buffer_msg_size_))
        {
            // we have a message from gobyd
            glogger() << debug <<"> " << notification_ << std::endl;

            // what type of message?
            switch(notification_.notification_type())
            {
                case proto::Notification::HEARTBEAT:
                {
                    // reply with same message to let gobyd know we're ok
                    send(*to_server_queue_, buffer_, buffer_msg_size_);
                }
                break;

                // someone else's publish, this is our subscription
                case proto::Notification::PUBLISH_REQUEST:
                {
                    typedef boost::unordered_multimap<std::string, shared_ptr<SubscriptionBase> >::const_iterator c_it;
                    std::pair<c_it, c_it> equal_it_pair =
                        subscriptions_.equal_range(notification_.embedded_msg().type());
    
                    for(c_it it = equal_it_pair.first; it != equal_it_pair.second; ++it)
                        it->second->post(notification_.embedded_msg().body());
                }
                break;
                        
                default: break;
            }
        }
        else
        {
            // no message, time to call loop()            
            loop();
            t_next_loop_ += loop_period_;
        }            
    }

}

bool goby::core::ApplicationBase::is_valid_filter(const google::protobuf::Descriptor* descriptor, const proto::Filter& filter)
{
    using namespace google::protobuf;
    // check filter for exclusions
    const FieldDescriptor* field_descriptor = descriptor->FindFieldByName(filter.key());
    if(!field_descriptor // make sure it exists
       || (field_descriptor->cpp_type() == FieldDescriptor::CPPTYPE_ENUM || // exclude enums
           field_descriptor->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) // exclude embedded
       || field_descriptor->is_repeated()) // no repeated fields for filter
    {
        glogger() << die << "bad filter: " << filter << "for message descriptor: " << descriptor->DebugString() << std::endl;
        return false;
    }
    return true;
}


void goby::core::ApplicationBase::insert_file_descriptor_proto(const google::protobuf::FileDescriptor* file_descriptor)
{
    // copy file descriptor for all dependencies of the new file
    for(int i = 0, n = file_descriptor->dependency_count(); i < n; ++i)
        // recursively add dependencies
        insert_file_descriptor_proto(file_descriptor->dependency(i));
    
    // copy descriptor for the new subscription type to the notification message
    if(!registered_file_descriptors_.count(file_descriptor))
    {
        file_descriptor->CopyTo(notification_.add_file_descriptor_proto());
        registered_file_descriptors_.insert(file_descriptor);
    }
}

void goby::core::ApplicationBase::finalize_header(
    google::protobuf::Message* msg,
    const goby::core::ApplicationBase::PublishDestination dest_type,
    const std::string& dest_platform)
{
    const google::protobuf::Descriptor* desc = msg->GetDescriptor();
    const google::protobuf::Reflection* refl = msg->GetReflection();    

    
    for (int i = 0, n = desc->field_count(); i < n; ++i)
    {
        const google::protobuf::FieldDescriptor* field_desc = desc->field(i);
        if(field_desc->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE && field_desc->message_type() == ::Header::descriptor())
        {
            
            Header* header = dynamic_cast<Header*>(refl->MutableMessage(msg, field_desc));
            

            bool has_unix_time = header->has_unix_time();
            bool has_iso_time = header->has_iso_time();

            // derived app has set neither time, use current time
            if(!(has_unix_time || has_iso_time))
            {
                boost::posix_time::ptime now = goby_time();
                header->set_unix_time(ptime2unix_double(now));
                header->set_iso_time(boost::posix_time::to_iso_string(now));
            }
            // derived app has set iso time, use this to set unix time
            else if(has_iso_time)
            {
                header->set_unix_time(ptime2unix_double(
                                          boost::posix_time::from_iso_string(
                                              header->iso_time())));
            }
            // derived app has set unix time, use this to set iso time
            else if(has_unix_time)
            {
                header->set_iso_time(boost::posix_time::to_iso_string(
                                         unix_double2ptime(
                                             header->unix_time())));
            }
            
            
            if(!header->has_source_app())
                header->set_source_app(base_cfg_.app_name());

            if(!header->has_source_platform())
                header->set_source_platform(base_cfg_.platform_name());

            switch(dest_type)
            {
                case goby::core::ApplicationBase::self:
                    header->set_dest_type(Header::self);
                    break;
                    
                case goby::core::ApplicationBase::other:
                    header->set_dest_type(Header::other);
                    header->set_dest_platform(dest_platform);
                    break;

                case goby::core::ApplicationBase::all:
                    header->set_dest_type(Header::all);
                    break;

            }
            
            
        }
    }
}
