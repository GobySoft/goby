
#ifndef PUBSUBNODE20110506H
#define PUBSUBNODE20110506H

#include <google/protobuf/message.h>
#include "goby/common/node_interface.h"

namespace goby
{
    namespace common
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
                if(!using_pubsub())
                {
                    glog.is(goby::common::logger::WARN) && glog << "Ignoring publish since we have `using_pubsub`=false" << std::endl;
                    return;
                }
                
                zeromq_service_.send(marshalling_scheme, identifier, data, size, SOCKET_PUBLISH);
            }

            void subscribe(MarshallingScheme marshalling_scheme,
                           const std::string& identifier)
            {
                if(!using_pubsub())
                {
                    glog.is(goby::common::logger::WARN) && glog << "Ignoring subscribe since we have `using_pubsub`=false" << std::endl;
                    return;
                }
                
                zeromq_service_.subscribe(marshalling_scheme, identifier, SOCKET_SUBSCRIBE);
            }
            
            
            void subscribe_all()
            {
                if(!using_pubsub())
                {
                    glog.is(goby::common::logger::WARN) && glog << "Ignoring subscribe since we have `using_pubsub`=false" << std::endl;
                    return;
                }
                
                zeromq_service_.subscribe_all(SOCKET_SUBSCRIBE);
            }

          protected:
            const protobuf::PubSubSocketConfig& cfg() const
            { return cfg_; }
            
            enum {
                SOCKET_SUBSCRIBE = 103998, SOCKET_PUBLISH = 103999
            };

            bool using_pubsub() const
            { return cfg_.has_publish_socket() && cfg_.has_subscribe_socket(); }
            
          private:

            void set_cfg(const protobuf::PubSubSocketConfig& cfg)
            {
                cfg_ = cfg;
                
                goby::common::protobuf::ZeroMQServiceConfig pubsub_cfg;

                using goby::glog;
                if(using_pubsub())
                {
                    glog.is(goby::common::logger::DEBUG1) && glog << "Using publish / subscribe." << std::endl;
                    
                    goby::common::protobuf::ZeroMQServiceConfig::Socket* subscriber_socket = pubsub_cfg.add_socket();
                    subscriber_socket->CopyFrom(cfg_.subscribe_socket());
                    subscriber_socket->set_socket_type(goby::common::protobuf::ZeroMQServiceConfig::Socket::SUBSCRIBE);
                    subscriber_socket->set_socket_id(SOCKET_SUBSCRIBE);
                    
                    glog.is(goby::common::logger::DEBUG1) && glog << "Subscriber socket: " << subscriber_socket->DebugString() << std::endl;

                    goby::common::protobuf::ZeroMQServiceConfig::Socket* publisher_socket = pubsub_cfg.add_socket();
                    publisher_socket->CopyFrom(cfg_.publish_socket());
                    publisher_socket->set_socket_type(goby::common::protobuf::ZeroMQServiceConfig::Socket::PUBLISH);
                    publisher_socket->set_socket_id(SOCKET_PUBLISH);

                    glog.is(goby::common::logger::DEBUG1) && glog << "Publisher socket: " << publisher_socket->DebugString() << std::endl;

                }
                else
                {
                    glog.is(goby::common::logger::DEBUG1) && glog << "Not using publish / subscribe. Set publish_socket and subscribe_socket to enable publish / subscribe." << std::endl;
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
                if(!using_pubsub())
                {
                    glog.is(goby::common::logger::WARN) && glog << "Ignoring publish since we have `using_pubsub`=false" << std::endl;
                    return;
                }
                
                node_.send(msg, SOCKET_PUBLISH);
            }


            void subscribe(const std::string& identifier)
            {
                if(!using_pubsub())
                {
                    glog.is(goby::common::logger::WARN) && glog << "Ignoring subscribe since we have `using_pubsub`=false" << std::endl;
                    return;
                }
                 
                node_.subscribe(identifier, SOCKET_SUBSCRIBE);
            }

          protected:
            
          private:
            NodeInterface<NodeTypeBase>& node_;
        };

    }
}

#endif
