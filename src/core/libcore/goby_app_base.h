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
#include <boost/interprocess/ipc/message_queue.hpp>
#include <boost/date_time.hpp>
#include <boost/unordered_map.hpp>

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
    GobyAppBase(const std::string& application_name = "",
                boost::posix_time::time_duration loop_period = boost::posix_time::milliseconds(100));
    virtual ~GobyAppBase();

    // call this to make everything happen
    // blocks until end() is called
    // or object is destroyed
    void run();
    
  protected:
    // here's where any synchronous work happens
    virtual void loop() { }
    
    // publish a message to all subscribers to that type (and variable name, if given)
    template<typename ProtoBufMessage>
        void publish(const ProtoBufMessage& msg, const std::string& var_name = "");

    // subscribes by binding a handler to a
    // generic function object (boost::function)
    // void handler(const ProtoBufMessage& msg)
    // ProtoBufMessage is any google::protobuf::Message derivatives
    // if name is omitted. all messages of this type are provided
    template<typename ProtoBufMessage>
        void subscribe(boost::function<void (const ProtoBufMessage&)> handler, const std::string& name = "");

    // overload subscribe for member functions of a class object
    // void C::mem_func(const ProtoBufMessage& msg)    
    template<class C, typename ProtoBufMessage>
        void subscribe(void(C::*mem_func)(const ProtoBufMessage&), C* obj, const std::string& name = "")
    { subscribe<ProtoBufMessage>(boost::bind(mem_func, obj, _1), name); }

    goby::util::FlexOstream glogger;

    // setters
    void set_application_name(std::string s)
    { application_name_ = s; }
    void set_loop_period(boost::posix_time::time_duration p)
    { loop_period_ = p; }
    void set_loop_period(long milliseconds)
    { loop_period_ = boost::posix_time::milliseconds(milliseconds); }
    void set_loop_freq(long hertz)
    { loop_period_ = boost::posix_time::milliseconds(1000/hertz); }
    
    
    // getters
    std::string application_name()
    { return application_name_; }
    boost::posix_time::time_duration loop_period()
    { return loop_period_; }
    long loop_freq()
    { return 1000/loop_period_.total_milliseconds(); }
    bool connected()
    { return connected_; }
    boost::posix_time::ptime t_start()
    { return t_start_; }
    
    // if constructed with a name, this is called from the constructor
    // otherwise you must call it yourself
    void start();

    // this is called by the destructor, call this yourself if you wish to keep the object around but want to disconnect and cleanup
    void end();

    
  private:
    
    void connect();
    void disconnect();
    
    void server_notify_subscribe(const google::protobuf::Descriptor* descriptor, const std::string& variable_name);
    void server_notify_publish(const google::protobuf::Descriptor* descriptor, const std::string& serialized_message, const std::string& variable_name);

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
      Subscription(boost::function<void (const ProtoBufMessage&)>& handler) : handler_(handler) { }
        
        void post(const std::string& serialized_message)
        {
            ProtoBufMessage msg;
            msg.ParseFromString(serialized_message);
            handler_(msg);
        }
        
      private:
        boost::function<void (const ProtoBufMessage&)> handler_;
        
    };
    
    static boost::posix_time::time_duration CONNECTION_WAIT_INTERVAL;

    boost::shared_ptr<boost::interprocess::message_queue> from_server_queue_;
    boost::shared_ptr<boost::interprocess::message_queue> to_server_queue_;
    
    bool connected_;

    // types we have informed the server of already
    std::set<std::string> registered_protobuf_types_;
    std::string application_name_;
    boost::posix_time::time_duration loop_period_;
    
    // handlers for all the subscriptions (keyed by protobuf message type name)
    typedef boost::shared_ptr<SubscriptionBase> Handler;
    // key = goby variable name
    typedef boost::unordered_map<std::string, Handler>
        NameHandlerMap;
    // key = protobuf message type name
    typedef boost::unordered_map<std::string, NameHandlerMap>
                TypeNameHandlerMap;
    TypeNameHandlerMap subscriptions_;

    boost::posix_time::ptime t_start_;
    boost::posix_time::ptime t_next_loop_;
};


template<typename ProtoBufMessage>
void GobyAppBase::publish(const ProtoBufMessage& msg, const std::string& var_name /* = "" */)
{
    std::string serialized_message;
    msg.SerializeToString(&serialized_message);
    server_notify_publish(ProtoBufMessage::descriptor(), serialized_message, var_name);
}



template<typename ProtoBufMessage>
void GobyAppBase::subscribe(boost::function<void (const ProtoBufMessage&)> handler, const std::string& variable_name /* = "" */)
{
    const std::string& type_name = ProtoBufMessage::descriptor()->full_name(); 

    // make sure we don't already have a handler for this type
    if(subscriptions_.count(type_name) &&
       subscriptions_[type_name].count(variable_name))
    {
        glogger << die << "handler already given for subscription to GobyVariable: " << type_name << " " << variable_name << std::endl;
    }
    
    // machinery so we can call the proper handler upon receipt of this type
    boost::shared_ptr<SubscriptionBase> subscription
        (new Subscription<ProtoBufMessage>(handler));
    subscriptions_[type_name][variable_name] = subscription;

    // tell the server about our subscription
    server_notify_subscribe(ProtoBufMessage::descriptor(), variable_name);
}


std::ostream& operator<<(std::ostream& out, const google::protobuf::Message& msg)
{ return (out << msg.ShortDebugString());}





#endif
