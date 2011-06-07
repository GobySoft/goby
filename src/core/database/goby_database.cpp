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
using goby::glog;
using goby::util::goby_time;

int main(int argc, char* argv[])
{
    goby::run<goby::core::Database>(argc, argv);
}

goby::core::Database::Database()
    : ZeroMQApplicationBase(&zeromq_service_, &cfg_),
      protobuf_node_(&zeromq_service_),
      pubsub_node_(&protobuf_node_),
      dbo_manager_(DBOManager::get_instance()),
      last_unique_id_(-1)
{
    
    
    if(cfg_.base().loop_freq() > MAX_LOOP_FREQ)
    {
        glog.is(warn) && 
            glog << "Setting loop frequency back to MAX_LOOP_FREQ = " << MAX_LOOP_FREQ << std::endl;
        set_loop_freq(MAX_LOOP_FREQ);
    }

    
    if(!cfg_.base().database_config().using_database())
    {
        glog.is(die) &&
            glog << "AppBaseConfig::using_database == false. Since we aren't wanting, we aren't starting (set to true to enable use of the database)!" << std::endl;
    }

    pubsub_node_.set_cfg(cfg_.base().pubsub_config());
    
    using goby::core::protobuf::ZeroMQNodeConfig;
    ZeroMQNodeConfig socket_cfg;
    ZeroMQNodeConfig::Socket* reply_socket = socket_cfg.add_socket();
    reply_socket->set_socket_type(ZeroMQNodeConfig::Socket::REPLY);
    reply_socket->set_transport(ZeroMQNodeConfig::Socket::TCP);
    reply_socket->set_socket_id(DATABASE_SERVER_SOCKET_ID);
    reply_socket->set_connect_or_bind(ZeroMQNodeConfig::Socket::BIND);

    if(cfg_.base().database_config().has_database_port())
        reply_socket->set_ethernet_port(cfg_.base().database_config().database_port());
    else
        reply_socket->set_ethernet_port(cfg_.base().pubsub_config().ethernet_port());

    try
    {
        zeromq_service_.merge_cfg(socket_cfg);

        glog.is(debug1) &&
            glog << "bound (requests line) to: " << *reply_socket << std::endl;
        
    }
    catch(std::exception& e)
    {
        glog.is(die) &&
            glog << "cannot bind to: " << *reply_socket << ": " << e.what()
                      << " check AppBaseConfig::database_port";
    }

    
    
    // subscribe for everything
    pubsub_node_.subscribe_all();
    init_sql();


    protobuf_node_.on_receipt<protobuf::DatabaseRequest>(DATABASE_SERVER_SOCKET_ID,
                                                         &goby::core::Database::handle_database_request,
                                                         this);
    
    zeromq_service_.connect_inbox_slot(&DBOManager::add_raw, dbo_manager_);

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

        glog.is(warn) &&
            glog << "db connection failed: " << e.what() << std::endl;
        std::string default_file = cfg_.sqlite().path();
            
        glog.is(verbose) &&
            glog << "trying again with defaults: " << default_file << std::endl;

        dbo_manager_->connect(default_file);
    }
    
    // #include <google/protobuf/descriptor.pb.h>
    DynamicProtobufManager::add_protobuf_file_with_dependencies(google::protobuf::FileDescriptorProto::descriptor()->file());
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



void goby::core::Database::handle_database_request(const protobuf::DatabaseRequest& proto_request)
{
    glog.is(debug1) &&
        glog << "Got request: " << proto_request << std::endl;
    
    static protobuf::DatabaseResponse proto_response;
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

            protobuf_node_.send(proto_response, DATABASE_SERVER_SOCKET_ID);

            glog.is(debug1) &&
                glog<< "Sent response: " << proto_response << std::endl;
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

