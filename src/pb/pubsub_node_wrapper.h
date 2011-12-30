// copyright 2011 t. schneider tes@mit.edu
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

#ifndef PUBSUBNODE20110506H
#define PUBSUBNODE20110506H

#include <google/protobuf/message.h>
#include "goby/common/node_interface.h"
#include "protobuf_node.h"
#include "goby/pb/protobuf/header.pb.h"

namespace goby
{
    namespace core
    {

        class PubSubNodeWrapperBase
        {
          public:
          PubSubNodeWrapperBase(ZeroMQService* service, const protobuf::PubSubSocketConfig& cfg)
              : zeromq_service_(*service)
            {
                set_cfg(cfg);
            }
            
            virtual ~PubSubNodeWrapperBase()
            { }

            void publish(MarshallingScheme marshalling_scheme,
                         const std::string& identifier,
                         const void* data,
                         int size)
            {
                if(!cfg().using_pubsub())
                {
                    glog.is(goby::util::logger::WARN) && glog << "Ignoring publish since we have `using_pubsub`=false" << std::endl;
                    return;
                }
                
                zeromq_service_.send(marshalling_scheme, identifier, data, size, SOCKET_PUBLISH);
            }

            void subscribe(MarshallingScheme marshalling_scheme,
                           const std::string& identifier)
            {
                if(!cfg().using_pubsub())
                {
                    glog.is(goby::util::logger::WARN) && glog << "Ignoring subscribe since we have `using_pubsub`=false" << std::endl;
                    return;
                }
                
                zeromq_service_.subscribe(marshalling_scheme, identifier, SOCKET_SUBSCRIBE);
            }
            
            
            void subscribe_all()
            { zeromq_service_.subscribe_all(SOCKET_SUBSCRIBE); }

          protected:
            const protobuf::PubSubSocketConfig& cfg() const
            { return cfg_; }
            
            enum {
                SOCKET_SUBSCRIBE = 103998, SOCKET_PUBLISH = 103999
            };

          private:

            void set_cfg(const protobuf::PubSubSocketConfig& cfg)
            {
                cfg_ = cfg;
              
                goby::core::protobuf::ZeroMQServiceConfig pubsub_cfg;
                
                using goby::glog;
                if(cfg.using_pubsub())
                {
                    glog.is(goby::util::logger::DEBUG1) && glog << "Using publish / subscribe." << std::endl;
                    goby::core::protobuf::ZeroMQServiceConfig::Socket* subscriber_socket = pubsub_cfg.add_socket();
                    subscriber_socket->set_socket_type(goby::core::protobuf::ZeroMQServiceConfig::Socket::SUBSCRIBE);
                    subscriber_socket->set_socket_id(SOCKET_SUBSCRIBE);
                    subscriber_socket->set_ethernet_address(cfg.ethernet_address());
                    subscriber_socket->set_multicast_address(cfg.multicast_address());
                    subscriber_socket->set_ethernet_port(cfg.ethernet_port());
                    std::cout << subscriber_socket->DebugString() << std::endl;

                    goby::core::protobuf::ZeroMQServiceConfig::Socket* publisher_socket = pubsub_cfg.add_socket();
                    publisher_socket->set_socket_type(goby::core::protobuf::ZeroMQServiceConfig::Socket::PUBLISH);
                    publisher_socket->set_socket_id(SOCKET_PUBLISH);
                    publisher_socket->set_ethernet_address(cfg.ethernet_address());
                    publisher_socket->set_multicast_address(cfg.multicast_address());
                    publisher_socket->set_ethernet_port(cfg.ethernet_port());
                }
                else
                {
                    glog.is(goby::util::logger::DEBUG1) && glog << "Not using publish / subscribe." << std::endl;
                }
                
                zeromq_service_.merge_cfg(pubsub_cfg);
            }
            
          private:
            ZeroMQService& zeromq_service_;
            protobuf::PubSubSocketConfig cfg_;
        };

        
        template<typename NodeTypeBase>
            class PubSubNodeWrapper : public PubSubNodeWrapperBase
        {
          public:
            PubSubNodeWrapper(NodeInterface<NodeTypeBase>* node, const protobuf::PubSubSocketConfig& cfg)
                : PubSubNodeWrapperBase(node->zeromq_service(), cfg),
                node_(*node)
                { }

            virtual ~PubSubNodeWrapper()
            { }
            
            
            /// \name Publish / Subscribe
            //@{

            /// \brief Publish a message (of any type derived from google::protobuf::Message)
            ///
            /// \param msg Message to publish
            void publish(const NodeTypeBase& msg)
            {
                if(!cfg().using_pubsub())
                {
                    glog.is(goby::util::logger::WARN) && glog << "Ignoring publish since we have `using_pubsub`=false" << std::endl;
                    return;
                }
                
                node_.send(msg, SOCKET_PUBLISH);
            }


            void subscribe(const std::string& identifier)
            {
                if(!cfg().using_pubsub())
                {
                    glog.is(goby::util::logger::WARN) && glog << "Ignoring subscribe since we have `using_pubsub`=false" << std::endl;
                    return;
                }
                 
                node_.subscribe(identifier, SOCKET_SUBSCRIBE);
            }

          protected:
            
          private:
            NodeInterface<NodeTypeBase>& node_;
        };
        
        class StaticProtobufPubSubNodeWrapper : public PubSubNodeWrapper<google::protobuf::Message>
        {
          public:
          StaticProtobufPubSubNodeWrapper(StaticProtobufNode* node, const protobuf::PubSubSocketConfig& cfg)
              : PubSubNodeWrapper<google::protobuf::Message>(node, cfg),
                node_(*node)
                { }
            
            ~StaticProtobufPubSubNodeWrapper()
            { }

            template<typename ProtoBufMessage>
                void subscribe(boost::function<void (const ProtoBufMessage&)> handler)
            {
                if(!cfg().using_pubsub())
                {
                    glog.is(goby::util::logger::WARN) && glog << "Ignoring subscribe since we have `using_pubsub`=false" << std::endl;
                    return;
                }    
                node_.subscribe<ProtoBufMessage>(SOCKET_SUBSCRIBE, handler);
            }

            template<typename ProtoBufMessage>
                const ProtoBufMessage& newest() const
            {
                return node_.newest<ProtoBufMessage>();
            }
            

          private:
            StaticProtobufNode& node_;
        };
        
        

    }
}

#endif
