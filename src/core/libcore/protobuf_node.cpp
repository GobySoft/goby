// copyright 2010-2011 t. schneider tes@mit.edu
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

#include "protobuf_node.h"

#include "goby/util/logger.h"
#include "dynamic_protobuf_manager.h"

using goby::glog;
using goby::util::as;

void goby::core::ProtobufNode::inbox(MarshallingScheme marshalling_scheme,
                                     const std::string& identifier,
                                     const void* data,
                                     int size,
                                     int socket_id)
{
    if(marshalling_scheme == MARSHALLING_PROTOBUF)
        protobuf_inbox(identifier.substr(0, identifier.find("/")), data, size, socket_id);
}


void goby::core::ProtobufNode::send(const google::protobuf::Message& msg, int socket_id)
{
    DynamicProtobufManager::add_protobuf_file_with_dependencies(msg.GetDescriptor()->file());
    
    int size = msg.ByteSize();
    char buffer[size];
    msg.SerializeToArray(&buffer, size);
    ZeroMQNode::get()->send(MARSHALLING_PROTOBUF, msg.GetDescriptor()->full_name() + "/",
                            &buffer, size, socket_id);
}
            
void goby::core::ProtobufNode::subscribe(const std::string& identifier, int socket_id)
{
    glog.is(debug1) && glog << "Subscribing for MARSHALLING_PROTOBUF type: " << identifier << std::endl;
    ZeroMQNode::get()->subscribe(MARSHALLING_PROTOBUF, identifier, socket_id);
}

void goby::core::StaticProtobufNode::protobuf_inbox(const std::string& protobuf_type_name,
                                                    const void* data,
                                                    int size,
                                                    int socket_id)
{
    boost::unordered_map<std::string, boost::shared_ptr<SubscriptionBase> >::iterator it = subscriptions_.find(protobuf_type_name);
    
    if(it != subscriptions_.end())
        it->second->post(data, size);
    else
        glog.is(debug1) && glog << warn << "No handler for static protobuf type: " << protobuf_type_name << " from socket id: " << socket_id << std::endl;
}




void goby::core::DynamicProtobufNode::protobuf_inbox(const std::string& protobuf_type_name,
                                                     const void* data,
                                                     int size,
                                                     int socket_id)
{
    boost::shared_ptr<google::protobuf::Message> msg =
        DynamicProtobufManager::new_protobuf_message(protobuf_type_name);
    msg->ParseFromArray(data, size);
    dynamic_protobuf_inbox(msg, socket_id);
}


