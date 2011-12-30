// copyright 2010-2011 t. schneider tes@mit.edu
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

#include "goby/util/time.h"

#include "application.h"

using namespace goby::core;
using namespace goby::util;
using namespace goby::util::logger;
using boost::shared_ptr;
using goby::glog;

goby::core::Application::Application(google::protobuf::Message* cfg /*= 0*/)
    : ZeroMQApplicationBase(&zeromq_service_, cfg)
{
    
    __set_up_sockets();
    
    // notify others of our configuration for logging purposes
    if(cfg) publish(*cfg);
}

goby::core::Application::~Application()
{
    glog.is(DEBUG1) &&
        glog << "Application destructing..." << std::endl;    
}



void goby::core::Application::__set_up_sockets()
{
    // if(!base_cfg().database_config().using_database())        
    // {
    //     glog.is(WARN) &&
    //         glog << "Not using `goby_database`. You will want to ensure you are logging your runtime data somehow" << std::endl;
    // }
    // else
    // {
    //     database_client_.reset(new DatabaseClient(&zeromq_service_));
    //     protobuf::DatabaseClientConfig database_config = base_cfg().database_config();
    //     if(!database_config.has_database_address())
    //         database_config.set_database_address(base_cfg().pubsub_config().ethernet_address());

    //     if(!database_config.has_database_port())
    //         database_config.set_database_port(base_cfg().pubsub_config().ethernet_port());

    //     database_client_->set_cfg(database_config);
    // }

    if(!base_cfg().pubsub_config().using_pubsub()) 
    {
        glog.is(WARN) &&
            glog << "Not using publish subscribe config. You will need to set up your nodes manually" << std::endl;
    }
    else
    {
        protobuf_node_.reset(new StaticProtobufNode(&zeromq_service_));
        pubsub_node_.reset(new StaticProtobufPubSubNodeWrapper(protobuf_node_.get(), base_cfg().pubsub_config()));
    }
    
    zeromq_service_.merge_cfg(base_cfg().additional_socket_config());
}



void goby::core::Application::__finalize_header(
    google::protobuf::Message* msg,
    const goby::core::Application::PublishDestination dest_type,
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

            if(!header)
            {
                glog.is(WARN) && glog << "Dynamic cast of Header failed!" << std::endl;
                return;
            }
            
            // derived app has not set time, use current time
            if(!header->has_time())
                header->set_time(goby::util::as<std::string>(goby::util::goby_time()));

            
            if(!header->has_source_app())
                header->set_source_app(base_cfg().app_name());

            if(!header->has_source_platform())
                header->set_source_platform(base_cfg().platform_name());

            header->set_dest_type(static_cast<Header::PublishDestination>(dest_type));
        }
    }
}

void goby::core::Application::__publish(google::protobuf::Message& msg, const std::string& platform_name, PublishDestination dest)
{
    // adds, as needed, required fields of Header
    __finalize_header(&msg, dest, platform_name);

    glog.is(DEBUG1) &&
        glog << "< " << msg << std::endl;

    if(pubsub_node_)
        pubsub_node_->publish(msg);

}
