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

#include <iostream>

#include "goby/util/time.h"

#include "goby_app_base.h"

using namespace goby::core;
using namespace goby::util;
using boost::interprocess::message_queue;

boost::posix_time::time_duration GobyAppBase::CONNECTION_WAIT_TIME = boost::posix_time::seconds(1);

GobyAppBase::GobyAppBase(const std::string& application_name)
    : application_name_(application_name),
      connected_(false)
{
    std::string verbosity = "verbose";
    glogger_.add_stream(verbosity, &std::cout);
    glogger_.name(application_name);

    connect();

    server_listen_thread_ =
        boost::shared_ptr<boost::thread>
        (new boost::thread(boost::bind(&GobyAppBase::server_listen, this)));
}

GobyAppBase::~GobyAppBase()
{
    disconnect();
    server_listen_thread_->join();
}

void GobyAppBase::connect()
{
    try
    {
        glogger_ <<  "trying to connect..." <<  std::endl;
        
        // set up queues to wait for response from server
        message_queue::remove
            (std::string(CONNECT_RESPONSE_QUEUE_PREFIX + application_name_).c_str());

        message_queue response_queue(boost::interprocess::create_only,
                                     std::string(CONNECT_RESPONSE_QUEUE_PREFIX
                                                 + application_name_).c_str(),
                                     1,
                                     MAX_MSG_BUFFER_SIZE);    
        
        // make connection
        message_queue listen_queue(boost::interprocess::open_or_create, CONNECT_LISTEN_QUEUE.c_str(), MAX_NUM_MSG, MAX_MSG_BUFFER_SIZE);
        
        
        // populate server request for connection
        proto::NotificationToServer request;
        request.set_notification_type(proto::NotificationToServer::CONNECT);
        request.set_application_name(application_name_);

        // serialize and send the server request
        char send_buffer [request.ByteSize()];
        request.SerializeToArray(&send_buffer, sizeof(send_buffer));
        listen_queue.send(&send_buffer, request.ByteSize(), 0);        
        
        // wait for response
        proto::NotificationToClient response;  
        while(!timed_receive(response_queue, response, CONNECTION_WAIT_TIME))
            glogger_ << warn << "waiting for server to respond..." <<  std::endl;        

        if(response.notification_type() == proto::NotificationToClient::CONNECTION_ACCEPTED)
        {
            connected_ = true;
            glogger_ <<  "connection succeeded." <<  std::endl;
        }
        else
        {
            glogger_ << die << "server did not connect us: " << "\n"
                    << response.ShortDebugString() << std::endl;
        }
    }
    catch(boost::interprocess::interprocess_exception &ex)
    {
        glogger_ << warn << ex.what() << std::endl;
    }
}


void GobyAppBase::disconnect()
{
    try
    {
        // make disconnection
        message_queue listen_queue(boost::interprocess::open_only,
                                   CONNECT_LISTEN_QUEUE.c_str());
    
    
        // populate server request for connection
        proto::NotificationToServer sr;
        sr.set_notification_type(proto::NotificationToServer::DISCONNECT);
        sr.set_application_name(application_name_);
        
        // serialize and send the server request
        send(listen_queue, sr);
        connected_ = false;
    }
    catch(boost::interprocess::interprocess_exception &ex)
    {
        glogger_ << warn << ex.what() << std::endl;
    }
}


void GobyAppBase::server_listen()
{
    try
    {
        boost::interprocess::message_queue
            from_server_queue(boost::interprocess::open_only,
                              std::string(FROM_SERVER_QUEUE_PREFIX + application_name_).c_str());
        
        while(connected_)
        {
            proto::NotificationToClient notification;
            if(timed_receive(from_server_queue, notification, boost::posix_time::seconds(1)))
            {
                boost::mutex::scoped_lock lock(mutex_);
                subscriptions_[notification.message_type()]->post(notification.serialized_message());
            }
        }
    }
    catch(boost::interprocess::interprocess_exception &ex)
    {
        glogger_ << warn << ex.what() << std::endl;
    }    
}


void GobyAppBase::server_notify_subscribe(const google::protobuf::Descriptor* descriptor)
{
    const std::string& type_name = descriptor->full_name();
    
    boost::interprocess::message_queue
        to_server_queue(boost::interprocess::open_only,
                        std::string(goby::core::TO_SERVER_QUEUE_PREFIX + application_name_).c_str());
    
    proto::NotificationToServer notification;
    
    if(!registered_protobuf_types_.count(type_name))
    {
        // copy descriptor for the new subscription type to the notification message
        descriptor->file()->CopyTo(notification.mutable_file_descriptor_proto());
        registered_protobuf_types_.insert(type_name);
    }
    notification.set_message_type(type_name);
    notification.set_notification_type(proto::NotificationToServer::SUBSCRIBE_REQUEST);
    send(to_server_queue, notification);
}


void GobyAppBase::server_notify_publish(const google::protobuf::Descriptor* descriptor, const std::string& serialized_message)
{
    const std::string& type_name = descriptor->full_name();
    
    boost::interprocess::message_queue
        to_server_queue(boost::interprocess::open_only,
                        std::string(goby::core::TO_SERVER_QUEUE_PREFIX + application_name_).c_str());
    
    proto::NotificationToServer notification;
    
    if(!registered_protobuf_types_.count(type_name))
    {
        // copy descriptor for the new subscription type to the notification message
        descriptor->file()->CopyTo(notification.mutable_file_descriptor_proto());
        registered_protobuf_types_.insert(type_name);
    }

    notification.set_message_type(type_name);
    notification.set_notification_type(proto::NotificationToServer::PUBLISH_REQUEST);
    notification.set_serialized_message(serialized_message);
    send(to_server_queue, notification);
}
