// copyright 2010-2011 t. schneider tes@mit.edu
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

#ifndef PROTOBUFNODE20110418H
#define PROTOBUFNODE20110418H

#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/unordered_map.hpp>

#include "goby/common/logger.h"
#include "goby/util/dynamic_protobuf_manager.h"
#include "goby/common/core_helpers.h"

#include "goby/common/node_interface.h"
#include "subscription.h"

namespace goby
{
    namespace pb
    {
        class ProtobufNode : public goby::common::NodeInterface<google::protobuf::Message>
        {

          protected:
          ProtobufNode(common::ZeroMQService* service)
              : common::NodeInterface<google::protobuf::Message>(service)
            { }
            
            virtual ~ProtobufNode()
            { }


            virtual void protobuf_inbox(const std::string& protobuf_type_name,
                                        const void* data,
                                        int size,
                                        int socket_id) = 0;

            void send(const google::protobuf::Message& msg, int socket_id);
            void subscribe(const std::string& identifier, int socket_id);

            
          private:
            void inbox(common::MarshallingScheme marshalling_scheme,
                       const std::string& identifier,
                       const void* data,
                       int size,
                       int socket_id);
            

        };

        class StaticProtobufNode : public ProtobufNode
        {
          public:
          StaticProtobufNode(common::ZeroMQService* service)
                : ProtobufNode(service)
            { }
                        
            virtual ~StaticProtobufNode()
            { }
            
            
            /// \brief Subscribe to a message (of any type derived from google::protobuf::Message)            
            ///
            /// \param socket_id Unique id assigned to the SUBSCRIBE socket that you want to subscribe to
            /// \param handler Function object to be called as soon as possible upon receipt of a message of this type. The signature of `handler` must match: void handler(const ProtoBufMessage& msg). if `handler` is omitted, no handler is called and only the newest message buffer is updated upon message receipt (for calls to newest<ProtoBufMessage>())
             template<typename ProtoBufMessage>
                void subscribe(
                    int socket_id,
                    boost::function<void (const ProtoBufMessage&)> handler =
                    boost::function<void (const ProtoBufMessage&)>()
                    );

            template<typename ProtoBufMessage, class C>
                void subscribe(int socket_id,
                               void(C::*mem_func)(const ProtoBufMessage&),
                               C* obj)
            { subscribe<ProtoBufMessage>(socket_id, boost::bind(mem_func, obj, _1)); }

             
             template<typename ProtoBufMessage>
                 void on_receipt(
                     int socket_id,
                     boost::function<void (const ProtoBufMessage&)> handler =
                     boost::function<void (const ProtoBufMessage&)>()
                     );

            template<typename ProtoBufMessage, class C>
                void on_receipt(int socket_id,
                                void(C::*mem_func)(const ProtoBufMessage&),
                                C* obj)
            { on_receipt<ProtoBufMessage>(socket_id, boost::bind(mem_func, obj, _1)); }


            void send(const google::protobuf::Message& msg, int socket_id)
            {
                ProtobufNode::send(msg, socket_id);
            }
             
            
            /// \name Message Accessors
            //@{
            /// \brief Fetchs the newest received message of this type 
            ///
            /// You must subscribe() for this type before using this method
            template<typename ProtoBufMessage>
                const ProtoBufMessage& newest() const;
            
            //@}
            

            
          private:
            void protobuf_inbox(const std::string& protobuf_type_name,
                                const void* data,
                                int size,
                                int socket_id);
            

          private:
            // key = type of var
            // value = Subscription object for all the subscriptions,  handler, newest message, etc.
            boost::unordered_map<std::string, boost::shared_ptr<SubscriptionBase> > subscriptions_;  
            
        };

        class DynamicProtobufNode : public ProtobufNode
        {
          protected:  
          DynamicProtobufNode(common::ZeroMQService* service)
                : ProtobufNode(service)
            { }
            
            virtual ~DynamicProtobufNode()
            { }
            
            virtual void dynamic_protobuf_inbox(boost::shared_ptr<google::protobuf::Message> msg, int socket_id) = 0;
            
          private:
            void protobuf_inbox(const std::string& protobuf_type_name,
                                const void* data,
                                int size,
                                int socket_id);

        };
        
        
    }    
}


template<typename ProtoBufMessage>
void goby::pb::StaticProtobufNode::on_receipt(
    int socket_id,
    boost::function<void (const ProtoBufMessage&)> handler
    /*= boost::function<void (const ProtoBufMessage&)>()*/)
{    
    using goby::glog;
    
    const std::string& protobuf_type_name = ProtoBufMessage::descriptor()->full_name();

    glog.is(goby::common::logger::DEBUG1) && 
        glog << "subscribing for " << protobuf_type_name  << std::endl;
    
    // enforce one handler for each type 
    typedef std::pair <std::string, boost::shared_ptr<SubscriptionBase> > P;
    BOOST_FOREACH(const P&p, subscriptions_)
    {
        if(protobuf_type_name == p.second->type_name())
        {
            glog.is(goby::common::logger::WARN) &&
                glog << "already have subscription for type: "
                     << protobuf_type_name << std::endl;
            return;
        }
    }
    
    // machinery so we can call the proper handler upon receipt of this type
    boost::shared_ptr<SubscriptionBase> subscription(
        new Subscription<ProtoBufMessage>(handler, protobuf_type_name));
    subscriptions_.insert(std::make_pair(protobuf_type_name, subscription));
}

template<typename ProtoBufMessage>
void goby::pb::StaticProtobufNode::subscribe(
    int socket_id,
    boost::function<void (const ProtoBufMessage&)> handler
    /*= boost::function<void (const ProtoBufMessage&)>()*/)
{
    const std::string& protobuf_type_name = ProtoBufMessage::descriptor()->full_name();
    on_receipt(socket_id, handler);
    ProtobufNode::subscribe(protobuf_type_name, socket_id);
}

/// See goby::pb::StaticProtobufNode::newest()
template<typename ProtoBufMessage>
const ProtoBufMessage& goby::pb::StaticProtobufNode::newest() const 
{
    // RTTI needed so we can store subscriptions with a common (non-template) base but also
    // return the subclass requested
    const std::string full_name = ProtoBufMessage::descriptor()->full_name();
    typedef boost::unordered_map<std::string, boost::shared_ptr<SubscriptionBase> >
        Map;

    for(Map::const_iterator it = subscriptions_.begin(), n = subscriptions_.end(); it != n; ++it)
    {
        if(it->second->type_name() == full_name)
            return dynamic_cast<const ProtoBufMessage&>(it->second->newest());
    }

    // this shouldn't happen if we properly create our Subscriptions
    throw(std::runtime_error("Invalid message given for call to newest()"));
}


#endif
