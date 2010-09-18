// copyright 2010 t. schneider tes@mit.edu
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

#ifndef GOBYAPPBASE20100908H
#define GOBYAPPBASE20100908H

#include <vector>

#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/interprocess/ipc/message_queue.hpp>
#include <boost/date_time.hpp>

#include <google/protobuf/message.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/descriptor.pb.h>

#include "goby/util/logger.h"
#include "goby/core/core_constants.h"
#include "message_queue_util.h"
#include "server_request.pb.h"

class GobyAppBase
{
  public:
    GobyAppBase(const std::string& application_name);
    virtual ~GobyAppBase();

  protected:
    // publish a message to all subscribers to that type
    template<typename ProtoBufMessage>
        void publish(const ProtoBufMessage& msg);

    // subscribes by binding a handler to a
    // generic function object (boost::function)
    // void handler(const ProtoBufMessage& msg)
    // ProtoBufMessage is any google::protobuf::Message derivatives
    template<typename ProtoBufMessage>
        void subscribe(boost::function<void (const ProtoBufMessage&)> handler);

    // overload subscribe for member functions of a class object
    // void C::mem_func(const ProtoBufMessage& msg)    
    template<class C, typename ProtoBufMessage>
        void subscribe(void(C::*mem_func)(const ProtoBufMessage&), C* obj)
    { subscribe<ProtoBufMessage>(boost::bind(mem_func, obj, _1)); }

    goby::util::FlexOstream glogger_;
    
  private:    
    void connect();
    void disconnect();
    
    void server_listen();
    void server_notify_subscribe(const google::protobuf::Descriptor* descriptor);
    void server_notify_publish(const google::protobuf::Descriptor* descriptor, const std::string& serialized_message);

  private:
    class SubscriptionBase
    {
      public:
        virtual void post(const std::string& serialized_message) = 0;
    };

    template<typename ProtoBufMessage>
        class Subscription : public SubscriptionBase
    {

      public:
        Subscription(boost::function<void (const ProtoBufMessage&)>& handler)
            : handler_(handler)
        { }

        void post(const std::string& serialized_message)
        {
            ProtoBufMessage msg;
            msg.ParseFromString(serialized_message);
            handler_(msg);
        }
        
      private:
        boost::function<void (const ProtoBufMessage&)> handler_;
        
    };
    
    static boost::posix_time::time_duration CONNECTION_WAIT_TIME;

    boost::shared_ptr<boost::thread> server_listen_thread_;
        
    boost::mutex mutex_;
    bool connected_;

    // types we have informed the server of already
    std::set<std::string> registered_protobuf_types_;
    std::string application_name_;
    
    // handlers for all the subscriptions (keyed by protobuf message type name)
    std::map<std::string, boost::shared_ptr<SubscriptionBase> > subscriptions_;
};

template<typename ProtoBufMessage>
void GobyAppBase::publish(const ProtoBufMessage& msg)
{
    std::string serialized_message;
    msg.SerializeToString(&serialized_message);
    server_notify_publish(ProtoBufMessage::descriptor(), serialized_message);
}



template<typename ProtoBufMessage>
void GobyAppBase::subscribe(boost::function<void (const ProtoBufMessage&)> handler)
    {
        const std::string& type_name = ProtoBufMessage::descriptor()->full_name(); 

        // make sure we don't already have a handler for this type
        if(subscriptions_.count(type_name))
            glogger_ << die << "handler already given for subscription to protobuf type name: " << type_name << std::endl;

        // machinery so we can call the proper handler upon receipt of this type
        boost::shared_ptr<SubscriptionBase> subscription(new Subscription<ProtoBufMessage>(handler));
        subscriptions_.insert(make_pair(type_name, subscription));

        // tell the server about our subscription
        server_notify_subscribe(ProtoBufMessage::descriptor());
    }


#endif
