// copyright 2011 t. schneider tes@mit.edu
// 
// the file is the goby database, part of the core goby autonomy system
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

#include <boost/thread.hpp>
#include <boost/format.hpp>

#include "goby/util/logger.h"
#include "goby/core/core_helpers.h"

#include "goby_database.h"

goby::core::protobuf::DatabaseConfig goby::core::Database::cfg_;

using goby::util::as;
using goby::util::glogger;
using goby::util::goby_time;

int main(int argc, char* argv[])
{
    goby::run<goby::core::Database>(argc, argv);
}

goby::core::Database::Database()
    : ZeroMQApplicationBase(&cfg_),
      database_server_(zmq_context(), ZMQ_REP),
      dbo_manager_(DBOManager::get_instance()),
      last_unique_id_(-1)
{
    
    
    if(cfg_.base().loop_freq() > MAX_LOOP_FREQ)
    {
        glogger() << warn << "Setting loop frequency back to MAX_LOOP_FREQ = " << MAX_LOOP_FREQ << std::endl;
        set_loop_freq(MAX_LOOP_FREQ);
    }

    
    if(!cfg_.base().using_database())
        glogger() << die << "AppBaseConfig::using_database == false. Since we aren't wanting, we aren't starting (set to true to enable use of the database)!" << std::endl;

    std::string database_binding = "tcp://*:";
    if(cfg_.base().has_database_port())
        database_binding += as<std::string>(cfg_.base().database_port());
    else
        database_binding += as<std::string>(cfg_.base().ethernet_port());

    try
    {
        database_server_.bind(database_binding.c_str());
        glogger() << debug << "bound (requests line) to: "
                  << database_binding << std::endl;
        
    }
    catch(std::exception& e)
    {
        glogger() << die << "cannot bind to: "
                  << database_binding << ": " << e.what()
                  << " check AppBaseConfig::database_port";
    }

    // subscribe for everything
    ZeroMQNode::subscribe_all();
    init_sql();


    zmq::pollitem_t item = { database_server_, 0, ZMQ_POLLIN, 0 };
    ZeroMQNode::register_poll_item(item,
                                   &goby::core::Database::handle_database_request,
                                   this);

    ZeroMQNode::connect_inbox_slot(&DBOManager::add_raw, dbo_manager_);

}

void goby::core::Database::init_sql()
{    
    try
    {
        cfg_.mutable_sqlite()->set_path(format_filename(cfg_.sqlite().path()));        
        dbo_manager_->connect(std::string(cfg_.sqlite().path()));
            
    }
    catch(std::exception& e)
    {
        cfg_.mutable_sqlite()->clear_path();
        cfg_.mutable_sqlite()->set_path(format_filename(cfg_.sqlite().path()));
        
        glogger() << warn << "db connection failed: "
                  << e.what() << std::endl;
        std::string default_file = cfg_.sqlite().path();
            
        glogger() << "trying again with defaults: "
                  << default_file << std::endl;

        dbo_manager_->connect(default_file);
    }
    
    // #include <google/protobuf/descriptor.pb.h>
    DynamicProtobufManager::add_protobuf_file(google::protobuf::FileDescriptorProto::descriptor());
    dbo_manager_->add_type(google::protobuf::FileDescriptorProto::descriptor());

}

std::string goby::core::Database::format_filename(const std::string& in)
{   
    boost::format f(in);
    // don't throw if user doesn't need some or all of format fields
    f.exceptions(boost::io::all_error_bits^(
                     boost::io::too_many_args_bit | boost::io::too_few_args_bit));
    f % cfg_.base().platform_name() % goby::util::goby_file_timestamp();
    return f.str();
}



void goby::core::Database::handle_database_request(const void* request_data,
                                                   int request_size,
                                                   int message_part)
{
    static protobuf::DatabaseRequest proto_request;
    static protobuf::DatabaseResponse proto_response;

    proto_request.ParseFromArray(request_data, request_size);
    glogger() << debug << "Got request: " << proto_request << std::endl;
    
    switch(proto_request.request_type())
    {
        case protobuf::DatabaseRequest::NEW_PUBLISH:
        {
            for(int i = 0, n = proto_request.file_descriptor_proto_size(); i < n; ++i)
            {
                DynamicProtobufManager::add_protobuf_file(proto_request.file_descriptor_proto(i));
                dbo_manager_->add_message(-1, proto_request.file_descriptor_proto(i));
                
                const google::protobuf::Descriptor* desc =
                    DynamicProtobufManager::descriptor_pool().FindMessageTypeByName(
                        proto_request.publish_protobuf_full_name());
                proto_response.Clear();
                    
                if(!desc)
                {
                    proto_response.set_response_type(
                        protobuf::DatabaseResponse::NEW_PUBLISH_DENIED);
                } 
                else
                {
                    dbo_manager_->add_type(desc);
                    proto_response.set_response_type(
                        protobuf::DatabaseResponse::NEW_PUBLISH_ACCEPTED);
                }
            }

            zmq::message_t zmq_response(proto_response.ByteSize());
            proto_response.SerializeToArray(zmq_response.data(), zmq_response.size());    
            database_server_.send(zmq_response);
            glogger() << debug << "Sent response: " << proto_response << std::endl;
        }
        break;
            
        case protobuf::DatabaseRequest::SQL_QUERY:
            // unimplemented
            break;
    }
}




void goby::core::Database::loop()
{
    dbo_manager_->commit();
}

