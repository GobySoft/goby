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

#include "goby/common/logger.h"
#include "goby/common/core_helpers.h"

#include "goby_database.h"

goby::common::protobuf::DatabaseConfig goby::common::Database::cfg_;

using goby::util::as;
using goby::glog;
using goby::common::goby_time;
using namespace goby::common::logger;

int main(int argc, char* argv[])
{
    goby::run<goby::common::Database>(argc, argv);
}

goby::common::Database::Database()
    : ZeroMQApplicationBase(&zeromq_service_, &cfg_),
      req_rep_protobuf_node_(&zeromq_service_),
      pubsub_node_(&zeromq_service_, cfg_.base().pubsub_config()),
      dbo_manager_(DBOManager::get_instance()),
      last_unique_id_(-1)
{    
    if(cfg_.base().loop_freq() > MAX_LOOP_FREQ)
    {
        glog.is(WARN) && 
            glog << "Setting loop frequency back to MAX_LOOP_FREQ = " << MAX_LOOP_FREQ << std::endl;
        set_loop_freq(MAX_LOOP_FREQ);
    }
    
    // if(!cfg_.base().database_config().using_database())
    // {
    //     glog.is(DIE) &&
    //         glog << "AppBaseConfig::using_database == false. Since we aren't wanting, we aren't starting (set to true to enable use of the database)!" << std::endl;
    // }

    for(int i = 0, n = cfg_.plugin_library_size(); i < n; ++i)
        dbo_manager_->plugin_factory().add_library(cfg_.plugin_library(i));
    dbo_manager_->plugin_factory().add_plugin(&protobuf_plugin_);

    
    
    using goby::common::protobuf::ZeroMQServiceConfig;
    ZeroMQServiceConfig socket_cfg;
    ZeroMQServiceConfig::Socket* reply_socket = socket_cfg.add_socket();
    reply_socket->set_socket_type(ZeroMQServiceConfig::Socket::REPLY);
    reply_socket->set_transport(ZeroMQServiceConfig::Socket::TCP);
    reply_socket->set_socket_id(DATABASE_SERVER_SOCKET_ID);
    reply_socket->set_connect_or_bind(ZeroMQServiceConfig::Socket::BIND);

    // if(cfg_.base().database_config().has_database_port())
    //     reply_socket->set_ethernet_port(cfg_.base().database_config().database_port());
    // else
    //     reply_socket->set_ethernet_port(cfg_.base().pubsub_config().ethernet_port());

    try
    {
        zeromq_service_.merge_cfg(socket_cfg);

        glog.is(DEBUG1) &&
            glog << "bound (requests line) to: " << *reply_socket << std::endl;
        
    }
    catch(std::exception& e)
    {
        glog.is(DIE) &&
            glog << "cannot bind to: " << *reply_socket << ": " << e.what()
                      << " check AppBaseConfig::database_port";
    }

    
    
    // subscribe for everything
    pubsub_node_.subscribe_all();
    init_sql();


    req_rep_protobuf_node_.on_receipt<protobuf::DatabaseRequest>(DATABASE_SERVER_SOCKET_ID,
                                                         &goby::common::Database::handle_database_request,
                                                         this);
    
    zeromq_service_.connect_inbox_slot(&DBOManager::add_raw, dbo_manager_);

}

void goby::common::Database::init_sql()
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

        glog.is(WARN) &&
            glog << "db connection failed: " << e.what() << std::endl;
        std::string default_file = cfg_.sqlite().path();
            
        glog.is(VERBOSE) &&
            glog << "trying again with defaults: " << default_file << std::endl;

        dbo_manager_->connect(default_file);
    }
    

}

std::string goby::common::Database::format_filename(const std::string& in)
{   
    boost::format f(in);
    // don't throw if user doesn't need some or all of format fields
    f.exceptions(boost::io::all_error_bits^(
                     boost::io::too_many_args_bit | boost::io::too_few_args_bit));
    f % cfg_.base().platform_name() % goby::common::goby_file_timestamp();
    return f.str();
}



void goby::common::Database::handle_database_request(const protobuf::DatabaseRequest& proto_request)
{
    glog.is(DEBUG1) &&
        glog << "Got request: " << proto_request << std::endl;
    
    static protobuf::DatabaseResponse proto_response;
    switch(proto_request.request_type())
    {
        case protobuf::DatabaseRequest::NEW_PUBLISH:
        {
            for(int i = 0, n = proto_request.file_descriptor_proto_size(); i < n; ++i)
            {
                goby::util::DynamicProtobufManager::add_protobuf_file(proto_request.file_descriptor_proto(i));
//                dbo_manager_->add_message(-1, proto_request.file_descriptor_proto(i));
                
                const google::protobuf::Descriptor* desc =
                    goby::util::DynamicProtobufManager::descriptor_pool().FindMessageTypeByName(
                        proto_request.publish_protobuf_full_name());
                proto_response.Clear();
                    
                if(!desc)
                {
                    proto_response.set_response_type(
                        protobuf::DatabaseResponse::NEW_PUBLISH_DENIED);
                } 
                else
                {
//                    dbo_manager_->add_type(desc);
                    proto_response.set_response_type(
                        protobuf::DatabaseResponse::NEW_PUBLISH_ACCEPTED);
                }
            }

            req_rep_protobuf_node_.send(proto_response, DATABASE_SERVER_SOCKET_ID);

            glog.is(DEBUG1) &&
                glog<< "Sent response: " << proto_response << std::endl;
        }
        break;
        
        case protobuf::DatabaseRequest::SQL_QUERY:
            // unimplemented
            break;
    }
}




void goby::common::Database::loop()
{
    dbo_manager_->commit();
}

