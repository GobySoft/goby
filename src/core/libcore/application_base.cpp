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

#include <boost/program_options.hpp>

#include <Wt/Dbo/Dbo>
#include <Wt/Dbo/Query>
#include <Wt/Dbo/backend/Sqlite3>
#include <Wt/Dbo/Exception>

#include "goby/util/time.h"
#include "goby/core/libdbo/dbo_manager.h"
#include "goby/protobuf/app_base_config.pb.h"

#include "application_base.h"

using namespace goby::core;
using namespace goby::util;
using boost::shared_ptr;
using goby::glog;

goby::core::ApplicationBase::ApplicationBase(google::protobuf::Message* cfg /*= 0*/)
    : ZeroMQApplicationBase(cfg),
      database_client_(zmq_context(), ZMQ_REQ)
{
    
    __set_up_sockets();
    
    // notify others of our configuration for logging purposes
    if(cfg) publish(*cfg);
}

goby::core::ApplicationBase::~ApplicationBase()
{
    glog.is(debug1) &&
        glog << "ApplicationBase destructing..." << std::endl;    
}



void goby::core::ApplicationBase::__set_up_sockets()
{
    if(!base_cfg().using_database())        
    {
        glog.is(warn) &&
            glog << "Not using `goby_database`. You will want to ensure you are logging your runtime data somehow" << std::endl;
    }
    else
    {
        std::string database_connection = "tcp://";

        if(base_cfg().has_database_address())
            database_connection += base_cfg().database_address();
        else
            database_connection += base_cfg().ethernet_address();

        database_connection += ":";
        
        if(base_cfg().has_database_port())
            database_connection += as<std::string>(base_cfg().database_port());
        else
            database_connection += as<std::string>(base_cfg().ethernet_port());

        try
        {
            database_client_.connect(database_connection.c_str());
            glog.is(debug1) &&
                glog << "connected (database requests line) to: "
                     << database_connection << std::endl;
        }
        catch(std::exception& e)
        {
            glog.is(die) &&
                glog << "cannot bind to: "
                     << database_connection << ": " << e.what()
                     << " check AppBaseConfig::database_address, AppBaseConfig::database_port" << std::endl;
        }
    }
}


void goby::core::ApplicationBase::__insert_file_descriptor_proto(const google::protobuf::FileDescriptor* file_descriptor, protobuf::DatabaseRequest* request)
{
    // copy file descriptor for all dependencies of the new file
    for(int i = 0, n = file_descriptor->dependency_count(); i < n; ++i)
        // recursively add dependencies
        __insert_file_descriptor_proto(file_descriptor->dependency(i), request);
    
    // copy descriptor for the new subscription type to the notification message
    if(!registered_file_descriptors_.count(file_descriptor))
    {
        file_descriptor->CopyTo(request->add_file_descriptor_proto());
        registered_file_descriptors_.insert(file_descriptor);
    }    
}

void goby::core::ApplicationBase::__finalize_header(
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

            if(!header)
            {
                glog.is(warn) && glog << "Dynamic cast of Header failed!" << std::endl;
                return;
            }
            
            // derived app has not set neither time, use current time
            if(!header->has_time())
                header->set_time(goby::util::as<std::string>(goby_time()));

            
            if(!header->has_source_app())
                header->set_source_app(base_cfg().app_name());

            if(!header->has_source_platform())
                header->set_source_platform(base_cfg().platform_name());

            header->set_dest_type(static_cast<Header::PublishDestination>(dest_type));
        }
    }
}

void goby::core::ApplicationBase::__publish(google::protobuf::Message& msg, const std::string& platform_name, PublishDestination dest)
{
    const std::string& protobuf_type_name = msg.GetDescriptor()->full_name();

    if(base_cfg().using_database() && !registered_file_descriptors_.count(msg.GetDescriptor()->file()))
    {
        // request permission to begin publishing
        // (so that we *know* the database has all entries)
        static protobuf::DatabaseRequest proto_request;
        static protobuf::DatabaseResponse proto_response;
        proto_request.Clear();
        __insert_file_descriptor_proto(msg.GetDescriptor()->file(), &proto_request);
        proto_request.set_request_type(protobuf::DatabaseRequest::NEW_PUBLISH);
        proto_request.set_publish_protobuf_full_name(protobuf_type_name);
        
        zmq::message_t zmq_request(proto_request.ByteSize());
        proto_request.SerializeToArray(zmq_request.data(), zmq_request.size());    
        database_client_.send(zmq_request);

        glog.is(debug1) &&
            glog << "Sending request to goby_database: " << proto_request << "\n"
                 << "...waiting on response" << std::endl;

        zmq::message_t zmq_response;
        database_client_.recv(&zmq_response);
        proto_response.ParseFromArray(zmq_response.data(), zmq_response.size());

        glog.is(debug1) &&
            glog << "Got response: " << proto_response << std::endl;

        if(!proto_response.response_type() == protobuf::DatabaseResponse::NEW_PUBLISH_ACCEPTED)
        {
            glog.is(die) && glog << die << "Database publish was denied!" << std::endl;
        }
        

    }

    
    // adds, as needed, required fields of Header
    __finalize_header(&msg, dest, platform_name);

    glog.is(debug1) &&
        glog << "< " << msg << std::endl;
    ProtobufNode::publish(msg);
}
