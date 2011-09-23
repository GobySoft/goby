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

#ifndef DATABASECLIENT20110506H
#define DATABASECLIENT20110506H

#include "goby/acomms/connect.h"
#include "goby/core/libdbo/dbo_manager.h"
#include "goby/protobuf/core_database_request.pb.h"
#include "goby/protobuf/zero_mq_node_config.pb.h"
#include "goby/util/dynamic_protobuf_manager.h"

#include "protobuf_node.h"

namespace goby
{
    namespace core
    {
        // provides hooks for using the Goby Database
        class DatabaseClient
        {
          public:
            DatabaseClient(ZeroMQService* service)
                : zeromq_service_(service),
                protobuf_node_(service)
                {
                    goby::acomms::connect(&zeromq_service_->pre_send_hooks, this, &DatabaseClient::pre_send);

                    if(cfg_.using_database())
                        protobuf_node_.on_receipt<protobuf::DatabaseResponse>(DATABASE_REQUEST_SOCKET_ID,
                                                                              &DatabaseClient::response_inbox,
                                                                              this);
                
                }
            virtual ~DatabaseClient()
            { }
            
            void set_cfg(const protobuf::DatabaseClientConfig& cfg)
            {
                cfg_ = cfg;

                if(cfg_.using_database())
                {
                    
                    using goby::core::protobuf::ZeroMQServiceConfig;
                    ZeroMQServiceConfig socket_cfg;
                    ZeroMQServiceConfig::Socket* request_socket = socket_cfg.add_socket();
                    request_socket->set_socket_type(ZeroMQServiceConfig::Socket::REQUEST);
                    request_socket->set_transport(ZeroMQServiceConfig::Socket::TCP);
                    request_socket->set_socket_id(DATABASE_REQUEST_SOCKET_ID);
                    request_socket->set_connect_or_bind(ZeroMQServiceConfig::Socket::CONNECT);
                    request_socket->set_ethernet_address(cfg_.database_address());
                    request_socket->set_ethernet_port(cfg_.database_port());
                    zeromq_service_->merge_cfg(socket_cfg);
                    
                    try
                    {
                        zeromq_service_->merge_cfg(socket_cfg);
                    }
                    catch(std::exception& e)
                    {
                        glog.is(goby::util::logger::DIE) &&
                            glog << "cannot connect to: "
                                 << *request_socket << ": " << e.what() << std::endl;
                    }
                }
                
            }
            
            
          private:
            void response_inbox(const protobuf::DatabaseResponse& response)
            {
                goby::glog.is(goby::util::logger::DEBUG1) &&
                    goby::glog << "Got response: " << response << std::endl;

                if(!response.response_type() == protobuf::DatabaseResponse::NEW_PUBLISH_ACCEPTED)
                {
                    goby::glog.is(goby::util::logger::DIE) && goby::glog<< "Database publish was denied!" << std::endl; 
                }
                waiting_on_response_.pop_back();
            }
            

            void pre_send(MarshallingScheme marshalling_scheme,
                          const std::string& identifier,
                          int socket_id)
            {
                if(marshalling_scheme == MARSHALLING_PROTOBUF)
                {
                    const std::string& protobuf_type_name = identifier.substr(0, identifier.find_first_of("/"));
                    boost::shared_ptr<google::protobuf::Message> msg = goby::util::DynamicProtobufManager::new_protobuf_message(protobuf_type_name);
                    
                    if(cfg_.using_database() && !registered_file_descriptors_.count(msg->GetDescriptor()->file()))
                    {
                        // request permission to begin publishing
                        // (so that we *know* the database has all entries)
                        static protobuf::DatabaseRequest proto_request;
                        static protobuf::DatabaseResponse proto_response;
                        proto_request.Clear();
                        insert_file_descriptor_proto(msg->GetDescriptor()->file(), &proto_request);
                        proto_request.set_request_type(protobuf::DatabaseRequest::NEW_PUBLISH);
                        proto_request.set_publish_protobuf_full_name(protobuf_type_name);

                        protobuf_node_.send(proto_request, DATABASE_REQUEST_SOCKET_ID);


                        goby::glog.is(goby::util::logger::DEBUG1) &&
                            goby::glog << "Sending request to goby_database: " << proto_request << "\n"
                                       << "...waiting on response" << std::endl;
                        
                        waiting_on_response_.push_back(protobuf_type_name);
                        while(waiting_on_response_.size() != 0 && waiting_on_response_.back() == protobuf_type_name)
                            zeromq_service_->poll();
                    }
                }
            }
            
            // add the protobuf description of the given descriptor (essentially the
            // instructions on how to make the descriptor or message meta-data)
            // to the notification_ message. 
            void insert_file_descriptor_proto(
                const google::protobuf::FileDescriptor* file_descriptor,
                protobuf::DatabaseRequest* request)
            {
                // copy file descriptor for all dependencies of the new file
                for(int i = 0, n = file_descriptor->dependency_count(); i < n; ++i)
                    // recursively add dependencies
                    insert_file_descriptor_proto(file_descriptor->dependency(i), request);
    
                // copy descriptor for the new subscription type to the notification message
                if(!registered_file_descriptors_.count(file_descriptor))
                {
                    file_descriptor->CopyTo(request->add_file_descriptor_proto());
                    registered_file_descriptors_.insert(file_descriptor);
                }    
            }
                
          private:
            ZeroMQService* zeromq_service_;

            enum { DATABASE_REQUEST_SOCKET_ID = 103997 };
            
            std::vector<std::string> waiting_on_response_;
            protobuf::DatabaseClientConfig cfg_;
            // database related things
            std::set<const google::protobuf::FileDescriptor*> registered_file_descriptors_;
            StaticProtobufNode protobuf_node_;
        };  
    }
}

#endif
