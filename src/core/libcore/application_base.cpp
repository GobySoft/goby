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
using boost::shared_ptr;

const boost::posix_time::time_duration goby::core::ApplicationBase::CONNECTION_WAIT_INTERVAL =
    boost::posix_time::seconds(1);

int goby::core::ApplicationBase::argc_ = 0;
char** goby::core::ApplicationBase::argv_ = 0;

goby::core::ApplicationBase::ApplicationBase(
    google::protobuf::Message* cfg /*= 0*/,
    bool skip_cfg_checks /* = false */)
    : loop_period_(boost::posix_time::milliseconds(100)),
      context_(1),
      alive_(true)
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
        // we are started
        t_start_ = goby_time();
        // start the loop() on the next even second
        t_next_loop_ = boost::posix_time::second_clock::universal_time() +
            boost::posix_time::seconds(1);
        
        // notify the server of our configuration for logging purposes
        if(cfg) publish(*cfg);
    }
    
}

goby::core::ApplicationBase::~ApplicationBase()
{
    glogger() << debug <<"ApplicationBase destructing..." << std::endl;    
}


void goby::core::ApplicationBase::run()
{
    // continue to run while we are alive (quit() has not been called)
    while(alive_)
    {
        //  Initialize poll set
        zmq::pollitem_t items [subscribers_.size()];
        for(int i = 0, n = subscribers_.size(); i < n; ++i)
        {
            items[i].socket = *subscribers_[i]; 
            items[i].fd = 0;
            items[i].events = ZMQ_POLLIN;
            items[i].revents = 0;
        }
        
        // sit and wait on a message until the next time to call loop() is up
        
        long timeout = (t_next_loop_-goby_time()).total_microseconds();
        if(timeout < 0)
            timeout = 0;
        
        bool had_events = false;
        try
        {
            zmq::poll (&items [0], subscribers_.size(), timeout);
            for(int i = 0, n = subscribers_.size(); i < n; ++i)
            {
                if (items[i].revents & ZMQ_POLLIN) 
                {
                    zmq::message_t message;
                    subscribers_[i]->recv(&message);
                    subscriptions_.find(subscribers_[i])->second->post(message.data(), message.size());
                    had_events = true;
                }
            }
            if(!had_events)
            {
                // no message, time to call loop()            
                loop();
                t_next_loop_ += loop_period_;
            }
            
        }
        catch(std::exception& e)
        {
            glogger() << warn << e.what() << std::endl;
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
        //file_descriptor->CopyTo(notification_.add_file_descriptor_proto());
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
