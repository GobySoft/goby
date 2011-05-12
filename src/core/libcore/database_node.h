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

#ifndef DATABASENODE20110506H
#define DATABASENODE20110506H

#include "goby/acomms/connect.h"
#include "goby/core/libdbo/dbo_manager.h"
#include "goby/protobuf/core_database_request.pb.h"

namespace goby
{
    namespace core
    {
        // provides hooks for using the Goby Database
        class DatabaseNode
        {
            DatabaseNode()
            {
                goby::acomms::connect(&ZeroMQNode::get()->pre_send_hooks, this, &DatabaseNode::pre_send);
            }
            virtual ~DatabaseNode()
            { }

            void set_cfg(const protobuf::DatabaseConfig& cfg)
            {
                cfg_ = cfg;
            }
            
            
          private:
            void pre_send(MarshallingScheme marshalling_scheme,
                          const std::string& identifier,
                          int socket_id)
            {
                if(marshalling_scheme == MARSHALLING_PROTOBUF)
                {
                    const std::string& protobuf_type_name = msg.GetDescriptor()->full_name();
                    
                    if(cfg_.using_database() && !registered_file_descriptors_.count(msg.GetDescriptor()->file()))
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

                        goby::glog.is(debug1) &&
                            goby::glog << "Sending request to goby_database: " << proto_request << "\n"
                                       << "...waiting on response" << std::endl;
                        
                        zmq::message_t zmq_response;
                        database_client_.recv(&zmq_response);
                        proto_response.ParseFromArray(zmq_response.data(), zmq_response.size());

                        goby::glog.is(debug1) &&
                            goby::glog << "Got response: " << proto_response << std::endl;

                        if(!proto_response.response_type() == protobuf::DatabaseResponse::NEW_PUBLISH_ACCEPTED)
                        {
                            goby::glog.is(die) && goby::glog << die << "Database publish was denied!" << std::endl;
                        }
                    }
                }

              private:
                protobuf::DatabaseConfig cfg_;
                // database related things
                std::set<const google::protobuf::FileDescriptor*> registered_file_descriptors_;
            
            }
            
            
        }
    }
}

#endif
