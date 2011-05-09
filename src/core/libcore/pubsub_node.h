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

#include "protobuf_node.h"
#include "goby/protobuf/header.pb.h"
namespace goby
{
    namespace core
    {
        // provides hooks for using the Goby Database
        class PubSubNode : public StaticProtobufNode
        {
          public:
            PubSubNode()
            {
            }
            virtual ~PubSubNode()
            { }
            
            void set_cfg(const protobuf::PubSubSocketConfig& cfg)
            {
                cfg_ = cfg;

                goby::core::protobuf::ZeroMQNodeConfig pubsub_cfg;

                using goby::glog;
                if(cfg.using_pubsub())
                {
                    glog.is(debug1) && glog << "Using publish / subscribe." << std::endl;
                    goby::core::protobuf::ZeroMQNodeConfig::Socket* subscriber_socket = pubsub_cfg.add_socket();
                    subscriber_socket->set_socket_type(goby::core::protobuf::ZeroMQNodeConfig::Socket::SUBSCRIBE);
                    subscriber_socket->set_socket_id(SOCKET_SUBSCRIBE);
                    subscriber_socket->set_ethernet_address(cfg.ethernet_address());
                    subscriber_socket->set_multicast_address(cfg.multicast_address());
                    subscriber_socket->set_ethernet_port(cfg.ethernet_port());
                    std::cout << subscriber_socket->DebugString() << std::endl;

                    goby::core::protobuf::ZeroMQNodeConfig::Socket* publisher_socket = pubsub_cfg.add_socket();
                    publisher_socket->set_socket_type(goby::core::protobuf::ZeroMQNodeConfig::Socket::PUBLISH);
                    publisher_socket->set_socket_id(SOCKET_PUBLISH);
                    publisher_socket->set_ethernet_address(cfg.ethernet_address());
                    publisher_socket->set_multicast_address(cfg.multicast_address());
                    publisher_socket->set_ethernet_port(cfg.ethernet_port());
                }
                else
                {
                    glog.is(debug1) && glog << "Not using publish / subscribe." << std::endl;
                }
                
                ZeroMQNode::get()->merge_cfg(pubsub_cfg);
            }


            void subscribe_all()
            { ZeroMQNode::get()->subscribe_all(SOCKET_SUBSCRIBE); }
            
            
            /// \name Publish / Subscribe
            //@{

            /// \brief Publish a message (of any type derived from google::protobuf::Message)
            ///
            /// \param msg Message to publish
            void publish(google::protobuf::Message& msg)
            {
                if(!cfg_.using_pubsub())
                {
                    glog.is(warn) && glog << "Ignoring publish since we have `using_pubsub`=false" << std::endl;
                    return;
                }
                
                StaticProtobufNode::send(msg, SOCKET_PUBLISH);
            }



            /// \brief Subscribe to a message (of any type derived from google::protobuf::Message)            
            ///
            /// \param handler Function object to be called as soon as possible upon receipt of a message of this type. The signature of `handler` must match: void handler(const ProtoBufMessage& msg). if `handler` is omitted, no handler is called and only the newest message buffer is updated upon message receipt (for calls to newest<ProtoBufMessage>())
             template<typename ProtoBufMessage>
                void subscribe(
                    boost::function<void (const ProtoBufMessage&)> handler =
                    boost::function<void (const ProtoBufMessage&)>()
                    )
             {
                 if(!cfg_.using_pubsub())
                 {
                     glog.is(warn) && glog << "Ignoring subscribe since we have `using_pubsub`=false" << std::endl;
                     return;
                 }
    
                 StaticProtobufNode::subscribe<ProtoBufMessage>(SOCKET_SUBSCRIBE, handler);
             }
            //@}
             

          private:
            
            enum {
                SOCKET_SUBSCRIBE = 103998, SOCKET_PUBLISH = 103999
            };
            
            protobuf::PubSubSocketConfig cfg_;
        };  
    }
}

#endif
