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
#include <boost/program_options.hpp>

#include <google/protobuf/message.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/io/zero_copy_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>

#include <Wt/Dbo/Dbo>
#include <Wt/Dbo/Query>
#include <Wt/Dbo/backend/Sqlite3>
#include <Wt/Dbo/Exception>

#include "goby/core/libdbo/wt_dbo_overloads.h"
#include "goby/util/logger.h"
#include "goby/core/core_constants.h"
#include "message_queue_util.h"
#include "goby/core/proto/interprocess_notification.pb.h"
#include "goby/core/proto/config.pb.h"
#include "filter.h"

namespace goby
{

    // friend function that creates and runs the
    // Goby Application
    // blocks caller until ApplicationBase::run() returns
    template<typename App>
        static int run(int argc, char* argv[]);
        
    namespace core
    {
        class ApplicationBase
        {
          protected:
            // make this more accessible
            typedef goby::core::proto::Filter Filter;
            
            ApplicationBase(google::protobuf::Message* cfg = 0);
            virtual ~ApplicationBase();

            // here's where any synchronous work happens
            virtual void loop() { }
            
            // publish a message to all subscribers to that
            // type (and variable name, if given)
            template<typename ProtoBufMessage>
                void publish(const ProtoBufMessage& msg);

            // subscribes by binding a handler to a
            // generic function object (boost::function)
            // void handler(const ProtoBufMessage& msg)
            // ProtoBufMessage is any google::protobuf::Message derivatives
            // if `filter` is omitted. all messages of this type are provided (unfiltered)
            // if `handler` is omitted, no handler is called and only the newest message buffer
            // is updated
            template<typename ProtoBufMessage>
                void subscribe(
                    boost::function<void (const ProtoBufMessage&)> handler,
                    const proto::Filter& filter = proto::Filter());

            // overload subscribe for member functions of a class object
            // void C::mem_func(const ProtoBufMessage& msg)    
            template<class C, typename ProtoBufMessage>
                void subscribe(void(C::*mem_func)(const ProtoBufMessage&),
                               C* obj,
                               const proto::Filter& filter = proto::Filter())
            { subscribe<ProtoBufMessage>(boost::bind(mem_func, obj, _1), filter); }


            // setters
            void set_application_name(std::string s)
            { application_name_ = s; }
            void set_loop_period(boost::posix_time::time_duration p)
            {
                std::cout << "loop period is " << p << std::endl;
                loop_period_ = p;
            }
            void set_loop_period(long milliseconds)
            { set_loop_period(boost::posix_time::milliseconds(milliseconds)); }
            void set_loop_freq(long hertz)
            { set_loop_period(boost::posix_time::milliseconds(1000/hertz)); }
            
    
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

            Wt::Dbo::backend::Sqlite3& db_connection() { return *db_connection_; }
            Wt::Dbo::Session& db_session() { return *db_session_; }
            

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
            
            goby::util::FlexOstream& glogger() { return glogger_; }
            
            
          private:
            ApplicationBase(const ApplicationBase&);
            ApplicationBase& operator= (const ApplicationBase&);

            // main loop that exits on disconnect
            void run();
            // called in Ctor if possible
            void connect();
            void disconnect();    

            bool is_valid_filter(const google::protobuf::Descriptor* descriptor,
                                 const proto::Filter& filter);

            void insert_descriptor_proto(const google::protobuf::Descriptor* descriptor);

            template<typename App>
                friend int ::goby::run(int argc, char* argv[]);
            friend class CMOOSApp;
            
          private:
            class SubscriptionBase
            {
              public:
                virtual void post(const std::string& serialized_message) = 0;
                virtual const proto::Filter& filter() const = 0;
            };

            template<typename ProtoBufMessage>
                class Subscription : public SubscriptionBase
            {
              public:
              Subscription(boost::function<void (const ProtoBufMessage&)>& handler,
                           const proto::Filter& filter)
                  : handler_(handler),
                    filter_(filter)
                    { }
                void post(const std::string& serialized_message)
                {
                    latest_msg_.ParseFromString(serialized_message);
                    if(clears_filter(latest_msg_, filter_))
                        handler_(latest_msg_);
                }

                const proto::Filter& filter() const { return filter_; }
                
              private:
                boost::function<void (const ProtoBufMessage&)> handler_;
                ProtoBufMessage latest_msg_;
                const proto::Filter filter_;
            };
    
            static boost::posix_time::time_duration CONNECTION_WAIT_INTERVAL;

            boost::shared_ptr<boost::interprocess::message_queue> from_server_queue_;
            boost::shared_ptr<boost::interprocess::message_queue> to_server_queue_;
    

            // types we have informed the server of already
            std::set<std::string> registered_protobuf_types_;
            // name of this application (e.g. garmin_gps_g)
            std::string application_name_;
            // name of this platform (e.g. name of the AUV)
            std::string self_name_;
            boost::posix_time::time_duration loop_period_;
            bool connected_;            
            
            // key = protobuf message type name
            // value = handler for all the subscriptions (keyed by protobuf message type name)
            boost::unordered_multimap<std::string, boost::shared_ptr<SubscriptionBase> >
                subscriptions_;

            boost::posix_time::ptime t_start_;
            boost::posix_time::ptime t_next_loop_;

            proto::Notification notification_;
            char buffer_ [goby::core::MAX_MSG_BUFFER_SIZE];
            std::size_t buffer_msg_size_;

            // database objects
            Wt::Dbo::backend::Sqlite3* db_connection_;
            Wt::Dbo::Session* db_session_;

            goby::util::FlexOstream glogger_;

            static int argc_;
            static char** argv_;

            boost::program_options::variables_map command_line_map_;

            // gobyd/global configuration
            // populated at connection time
            goby::core::proto::Config daemon_cfg_;
        };
    }
}


             
template<typename ProtoBufMessage>
void goby::core::ApplicationBase::publish(const ProtoBufMessage& msg)
{
    // clear the global notification message
    notification_.Clear();
    // we are publishing
    notification_.set_notification_type(proto::Notification::PUBLISH_REQUEST);
    // contents of the message
    google::protobuf::io::StringOutputStream os(
        notification_.mutable_embedded_msg()->mutable_body());
    msg.SerializeToZeroCopyStream(&os);
    // name of the message
    notification_.mutable_embedded_msg()->set_type(msg.GetDescriptor()->full_name());
    // appends, if needed, the meta data of this type to notification_
    insert_descriptor_proto(msg.GetDescriptor());

    glogger() << "< " << notification_ << std::endl;
    send(*to_server_queue_, notification_, buffer_, sizeof(buffer_));
}

template<typename ProtoBufMessage>
void goby::core::ApplicationBase::subscribe(boost::function<void (const ProtoBufMessage&)> handler, const proto::Filter& filter /* = proto::Filter() */)
{
    const std::string& type_name = ProtoBufMessage::descriptor()->full_name();

    // enforce one handler for each type / filter combination            
    typedef std::pair <std::string, boost::shared_ptr<SubscriptionBase> > P;
    BOOST_FOREACH(const P&p, subscriptions_)
    {
        if(p.second->filter() == filter)
        {
            glogger() << warn << "already have subscription for type: " << type_name
                      << " and filter: " << filter;
            return;
        }
    }
            
    // machinery so we can call the proper handler upon receipt of this type
    boost::shared_ptr<SubscriptionBase> subscription(
        new Subscription<ProtoBufMessage>(handler, filter));
            
    subscriptions_.insert(make_pair(type_name, subscription));
    // clear the global notification message
    notification_.Clear();
    // we are subscribing
    notification_.set_notification_type(proto::Notification::SUBSCRIBE_REQUEST);
    // subscribe to this type
    notification_.mutable_embedded_msg()->set_type(type_name);
    // appends, if needed, the meta data of this type to notification_
    insert_descriptor_proto(ProtoBufMessage::descriptor());
    // if a filter, append it
    if(filter.IsInitialized() && is_valid_filter(ProtoBufMessage::descriptor(), filter))
        notification_.mutable_embedded_msg()->mutable_filter()->CopyFrom(filter);
            
    glogger() << notification_ << std::endl;    
            
    send(*to_server_queue_, notification_, buffer_, sizeof(buffer_));
            
}


template<typename App>
static int goby::run(int argc, char* argv[])
{
    // avoid making the user pass these through their Ctor...
    App::argc_ = argc;
    App::argv_ = argv;

    try
    {
        App app;
        app.run();
    }
    catch(std::exception& e)
    {
        std::cerr << "uncaught exception: " << e.what() << std::endl;
        return 1;
    }
}


#endif
