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
using boost::shared_ptr;

boost::posix_time::time_duration GobyAppBase::CONNECTION_WAIT_INTERVAL =
    boost::posix_time::seconds(1);

GobyAppBase::GobyAppBase(const std::string& application_name /* = "" */,
                         boost::posix_time::time_duration loop_period /*= boost::posix_time::milliseconds(100)*/)
    : application_name_(application_name),
      loop_period_(loop_period),
      connected_(false)
{
    // connect and some other initialization duties
    if(!application_name.empty()) start(); 
}

GobyAppBase::~GobyAppBase()
{
    if(connected_) end();
}

void GobyAppBase::start()
{
    std::string verbosity = "verbose";
    glogger.add_stream(verbosity, &std::cout);
    glogger.name(application_name());

    connect();
    
    to_server_queue_ = shared_ptr<message_queue>(new message_queue(boost::interprocess::open_only, std::string(goby::core::TO_SERVER_QUEUE_PREFIX + application_name_).c_str()));

    from_server_queue_ = shared_ptr<message_queue>(new message_queue(boost::interprocess::open_only, std::string(goby::core::FROM_SERVER_QUEUE_PREFIX + application_name_).c_str()));
    
    t_start_ = goby_time();
    // start on the next even second
    t_next_loop_ = boost::posix_time::second_clock::universal_time() + boost::posix_time::seconds(1);
}

void GobyAppBase::end()
{
    disconnect();
}

void GobyAppBase::connect()

{
    try
    {
        glogger <<  "trying to connect..." <<  std::endl;
        
        // set up queues to wait for response from server
        message_queue::remove
            (std::string(CONNECT_RESPONSE_QUEUE_PREFIX + application_name_).c_str());

        message_queue response_queue
            (boost::interprocess::create_only,
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
        send(listen_queue,request, buffer_, sizeof(buffer_));  
        
        // wait for response
        proto::NotificationToClient response;  
        while(!timed_receive(response_queue, response, goby_time() + CONNECTION_WAIT_INTERVAL, buffer_))
            glogger << warn << "waiting for server to respond..." <<  std::endl;        

        if(response.notification_type() == proto::NotificationToClient::CONNECTION_ACCEPTED)
        {
            connected_ = true;
            glogger <<  "connection succeeded." <<  std::endl;
        }
        else
        {
            glogger << die << "server did not connect us: " << "\n"
                    << response.ShortDebugString() << std::endl;
        }
    }
    catch(boost::interprocess::interprocess_exception &ex)
    {
        glogger << warn << ex.what() << std::endl;
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
        send(listen_queue, sr, buffer_, sizeof(buffer_));
        connected_ = false;
    }
    catch(boost::interprocess::interprocess_exception &ex)
    {
        glogger << warn << ex.what() << std::endl;
    }
}


void GobyAppBase::run()
{

    while(connected_)
    {
        proto::NotificationToClient in_msg;
        if(timed_receive(*from_server_queue_, in_msg, t_next_loop_, buffer_))
        {
            glogger << "> " << in_msg.DebugString() << std::endl;
            
            switch(in_msg.notification_type())
            {
                case proto::NotificationToClient::HEARTBEAT_REQUEST:
                {
                    proto::NotificationToServer heartbeat;
                    heartbeat.set_notification_type
                        (proto::NotificationToServer::HEARTBEAT);
                    send(*to_server_queue_, heartbeat, buffer_, sizeof(buffer_));
                }
                break;
                        
                case proto::NotificationToClient::INCOMING_MESSAGE:
                {
                    if(in_msg.embedded_msg().has_name())
                        subscriptions_[in_msg.embedded_msg().type()][in_msg.embedded_msg().name()]->post(in_msg.embedded_msg().body());
                    else
                    {
                        NameHandlerMap& name_map = subscriptions_[in_msg.embedded_msg().type()];
                        for(NameHandlerMap::iterator it = name_map.begin(), n = name_map.end(); it !=n; ++it)
                        {
                            it->second->post(in_msg.embedded_msg().body());
                        }
                        
                    }
                }
                break;
                        
                default: break;
            }
        }
        else
        {
            loop();
            t_next_loop_ += loop_period_;
        }            
    }
}


void GobyAppBase::server_notify_subscribe(const google::protobuf::Descriptor* descriptor, const std::string& variable_name)
{
    const std::string& type_name = descriptor->full_name();
    proto::NotificationToServer notification;
    
    if(!registered_protobuf_types_.count(type_name))
    {
        // copy descriptor for the new subscription type to the notification message
        descriptor->file()->CopyTo(notification.mutable_file_descriptor_proto());
        registered_protobuf_types_.insert(type_name);
    }
    notification.mutable_embedded_msg()->set_type(type_name);
    if(!variable_name.empty())
        notification.mutable_embedded_msg()->set_name(variable_name);
    
    notification.set_notification_type(proto::NotificationToServer::SUBSCRIBE_REQUEST);
    send(*to_server_queue_, notification, buffer_, sizeof(buffer_));
}


void GobyAppBase::server_notify_publish(const google::protobuf::Descriptor* descriptor, const std::string& serialized_message, const std::string& variable_name)
{
    const std::string& type_name = descriptor->full_name();
    proto::NotificationToServer notification;
    
    if(!registered_protobuf_types_.count(type_name))
    {
        // copy descriptor for the new subscription type to the notification message
        descriptor->file()->CopyTo(notification.mutable_file_descriptor_proto());
        registered_protobuf_types_.insert(type_name);
    }

    notification.mutable_embedded_msg()->set_type(type_name);
    if(!variable_name.empty())
        notification.mutable_embedded_msg()->set_name(variable_name);

    
    notification.set_notification_type(proto::NotificationToServer::PUBLISH_REQUEST);
    notification.mutable_embedded_msg()->set_body(serialized_message);

    glogger << "< " << notification.DebugString() << std::endl;    

    send(*to_server_queue_, notification, buffer_, sizeof(buffer_));
}
