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

#include "goby/util/logger.h"
#include "goby/core/core_helpers.h"

#include "filter.h"
#include "subscription.h"
#include "zero_mq_node.h"

namespace goby
{
    namespace core
    {
        class ProtobufNode : public virtual ZeroMQNode
        {
          protected:
            
            ProtobufNode()
            {
                ZeroMQNode::connect_inbox_slot(&ProtobufNode::inbox, this);
            }

            
            virtual ~ProtobufNode()
            { }
            

            virtual void protobuf_inbox(const std::string& protobuf_type_name,
                                        const void* data,
                                        int size) = 0;

            void publish(const google::protobuf::Message& msg);
            
            void subscribe(const std::string& identifier);

          private:
            void inbox(MarshallingScheme marshalling_scheme,
                       const std::string& identifier,
                       const void* data,
                       int size);

        };

        class StaticProtobufNode : public ProtobufNode
        {
          protected:
            typedef goby::core::protobuf::Filter Filter;

            StaticProtobufNode()
            { }
                        
            virtual ~StaticProtobufNode()
            { }

            
            /// \brief Subscribe to a message (of any type derived from google::protobuf::Message)            
            ///
            /// \param handler Function object to be called as soon as possible upon receipt of a message of this type (and passing this filter, if provided). The signature of `handler` must match: void handler(const ProtoBufMessage& msg). if `handler` is omitted, no handler is called and only the newest message buffer is updated upon message receipt (for calls to newest<ProtoBufMessage>())
            /// \param filter Filter object to reject some subset of messages of type ProtoBufMessage. if `filter` is omitted. all messages of this type are subscribed for unfiltered
             template<typename ProtoBufMessage>
                void subscribe(
                    boost::function<void (const ProtoBufMessage&)> handler =
                    boost::function<void (const ProtoBufMessage&)>(),
                    const protobuf::Filter& filter = protobuf::Filter());
            
            /// \brief Subscribe for a type using a class member function as the handler
            /// 
            /// \param mem_func Member function (method) of class C with a signature of void C::mem_func(const ProtoBufMessage& msg)
            /// \param obj pointer to the object whose member function (mem_func) to call
            /// \param filter (optional) Filter object to reject some subset of ProtoBufMessages.
            template<typename ProtoBufMessage, class C>
                void subscribe(void(C::*mem_func)(const ProtoBufMessage&),
                               C* obj,
                               const protobuf::Filter& filter = protobuf::Filter())
            { subscribe<ProtoBufMessage>(boost::bind(mem_func, obj, _1), filter); }


            /// \brief Subscribe for a type without a handler but with a filter
            template<typename ProtoBufMessage>
                void subscribe(const protobuf::Filter& filter)
            {
                subscribe<ProtoBufMessage>(boost::function<void (const ProtoBufMessage&)>(),
                                           filter);
            }
            
            /// \name Message Accessors
            //@{
            /// \brief Fetchs the newest received message of this type (that optionally passes the Filter given)
            ///
            /// You must subscribe() for this type before using this method
            template<typename ProtoBufMessage>
                const ProtoBufMessage& newest(const Filter& filter = Filter());
            //@}
            
            
            
            /// \brief Helper function for creating a message Filter
            ///
            /// \param key name of the field for which this Filter will act on (left hand side of expression)
            /// \param op operation (EQUAL, NOT_EQUAL)
            /// \param value (right hand side of expression)
            ///
            /// For example make_filter("name", Filter::EQUAL, "joe"), makes a filter that requires the field
            /// "name" always equal "joe" (or the message is rejected).
            static Filter make_filter(const std::string& key,
                                      Filter::Operation op,
                                      const std::string& value)
            {
                Filter filter;
                filter.set_key(key);
                filter.set_operation(op);
                filter.set_value(value);
                return filter;
            }

            
          private:
            void protobuf_inbox(const std::string& protobuf_type_name,
                                const void* data,
                                int size);
            

            // returns true if the Filter provided is valid with the given descriptor
            // an example of failure (return false) would be if the descriptor given
            // does not contain the key provided by the filter
            bool __is_valid_filter(const google::protobuf::Descriptor* descriptor,
                                 const protobuf::Filter& filter);

          private:
            // key = type of var
            // value = Subscription object for all the subscriptions, containing filter,
            //    handler, newest message, etc.
            boost::unordered_map<std::string, boost::shared_ptr<SubscriptionBase> > subscriptions_;  
            
        };

        class DynamicProtobufNode : public ProtobufNode
        {
          protected:
            
            DynamicProtobufNode()
            { }
            
            virtual ~DynamicProtobufNode()
            { }

            virtual void dynamic_protobuf_inbox(boost::shared_ptr<google::protobuf::Message> msg) = 0;
            
          private:
            void protobuf_inbox(const std::string& protobuf_type_name,
                                const void* data,
                                int size);

        };
        
        
    }    
}


template<typename ProtoBufMessage>
    void goby::core::StaticProtobufNode::subscribe(
        boost::function<void (const ProtoBufMessage&)> handler
        /*= boost::function<void (const ProtoBufMessage&)>()*/,
        const protobuf::Filter& filter /* = protobuf::Filter() */)
{

    const std::string& protobuf_type_name = ProtoBufMessage::descriptor()->full_name();

    goby::util::glogger() << debug << "subscribing for " << protobuf_type_name << " with filter: " << filter << std::endl;
    
    // enforce one handler for each type / filter combination            
    typedef std::pair <std::string, boost::shared_ptr<SubscriptionBase> > P;
    BOOST_FOREACH(const P&p, subscriptions_)
    {
        if(protobuf_type_name == p.second->type_name() && p.second->filter() == filter)
        {
            goby::util::glogger() << warn <<
                "already have subscription for type: " <<
                protobuf_type_name << " and filter: " << filter << std::endl;
            return;
        }
    }
    
    // machinery so we can call the proper handler upon receipt of this type
    boost::shared_ptr<SubscriptionBase> subscription(
        new Subscription<ProtoBufMessage>(handler, filter, protobuf_type_name));
    subscriptions_.insert(std::make_pair(protobuf_type_name, subscription));

    
    ProtobufNode::subscribe(protobuf_type_name);
}

/// See goby::core::StaticProtobufNode::newest(const protobuf::Filter& filter = protobuf::Filter())
template<typename ProtoBufMessage>
const ProtoBufMessage& goby::core::StaticProtobufNode::newest(const protobuf::Filter& filter
                                                           /* = protobuf::Filter()*/)
{
    // RTTI needed so we can store subscriptions with a common (non-template) base but also
    // return the subclass requested
    const std::string full_name = ProtoBufMessage::descriptor()->full_name();
    typedef boost::unordered_map<std::string, boost::shared_ptr<SubscriptionBase> >
        Map;

    for(Map::const_iterator it = subscriptions_.begin(), n = subscriptions_.end(); it != n; ++it)
    {
        if(it->second->type_name() == full_name && it->second->filter() == filter)
            return dynamic_cast<const ProtoBufMessage&>(it->second->newest());
    }

    // this shouldn't happen if we properly create our Subscriptions
    throw(std::runtime_error("Invalid message or filter given for call to newest()"));
}


#endif
