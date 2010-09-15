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
    template<typename ProtoBufMessage>
        void publish(const ProtoBufMessage& msg)
    { }

    // subscribes by binding a handler to a
    // generic function object (boost::function)
    // void handler(const ProtoBufMessage& msg)
    // ProtoBufMessage is any google::protobuf::Message derivatives
    template<typename ProtoBufMessage>
        void subscribe(boost::function<void (const ProtoBufMessage&)> handler)
    {
        boost::interprocess::message_queue
            to_server_queue(boost::interprocess::open_only,
                            std::string(goby::core::TO_SERVER_QUEUE_PREFIX + application_name_).c_str());

        goby::core::ServerNotification notification;
        // copy descriptor for the new subscription type to the notification message
        ProtoBufMessage::descriptor()->file()->CopyTo(notification.mutable_file_descriptor_proto());
        notification.set_notification_type(goby::core::ServerNotification::SUBSCRIBE_REQUEST);
        goby::core::send(to_server_queue, notification);
    }

    // overload subscribe for member functions of a class object
    // void C::mem_func(const ProtoBufMessage& msg)    
    template<class C, typename ProtoBufMessage>
        void subscribe(void(C::*mem_func)(const ProtoBufMessage&), C* obj)
    { subscribe<ProtoBufMessage>(boost::bind(mem_func, obj, _1)); }

  private:
    static boost::posix_time::time_duration CONNECTION_WAIT_TIME;
    
    void connect();
    void disconnect();
    
    void server_listen();
    
  private:
    boost::shared_ptr<boost::thread> server_listen_thread_;
        
    std::vector< boost::shared_ptr<boost::thread> > subscription_threads_;
    boost::mutex mutex_;
    std::string application_name_;
    bool connected_;
};


#endif
