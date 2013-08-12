// Copyright 2009-2013 Toby Schneider (https://launchpad.net/~tes)
//                     Massachusetts Institute of Technology (2007-)
//                     Woods Hole Oceanographic Institution (2007-)
//                     Goby Developers Team (https://launchpad.net/~goby-dev)
// 
//
// This file is part of the Goby Underwater Autonomy Project Libraries
// ("The Goby Libraries").
//
// The Goby Libraries are free software: you can redistribute them and/or modify
// them under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// The Goby Libraries are distributed in the hope that they will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with Goby.  If not, see <http://www.gnu.org/licenses/>.


#include "protobuf_node.h"

#include "goby/common/logger.h"

using goby::glog;
using goby::util::as;
using namespace goby::common::logger;

void goby::pb::ProtobufNode::inbox(common::MarshallingScheme marshalling_scheme,
                                   const std::string& identifier,
                                   const void* data,
                                   int size,
                                   int socket_id)
{
    if(marshalling_scheme == common::MARSHALLING_PROTOBUF)
    {
        std::string::size_type first_slash = identifier.find("/");
        std::string group = (first_slash + 1) < identifier.size() ? identifier.substr(first_slash + 1) : "";
        protobuf_inbox(identifier.substr(0, first_slash), data, size, socket_id, group);
    }
    
}


void goby::pb::ProtobufNode::send(const google::protobuf::Message& msg, int socket_id, const std::string& group)
{
    int size = msg.ByteSize();
    char buffer[size];
    msg.SerializeToArray(&buffer, size);

    std::string identifier =  msg.GetDescriptor()->full_name() + "/";
    if(!group.empty())
        identifier += group + "/";
    
    zeromq_service()->send(common::MARSHALLING_PROTOBUF, identifier, &buffer, size, socket_id);
}
            
void goby::pb::ProtobufNode::subscribe(const std::string& identifier, int socket_id)
{
    glog.is(DEBUG1) && glog << "Subscribing for MARSHALLING_PROTOBUF type: " << identifier << std::endl;
    zeromq_service()->subscribe(common::MARSHALLING_PROTOBUF, identifier, socket_id);
}

void goby::pb::StaticProtobufNode::protobuf_inbox(const std::string& protobuf_type_name,
                                                  const void* data,
                                                  int size,
                                                  int socket_id,
                                                  const std::string& group
    )
{
    boost::unordered_map<std::string, boost::shared_ptr<SubscriptionBase> >::iterator it = subscriptions_.find(protobuf_type_name);
    
    if(it != subscriptions_.end())
        it->second->post(data, size, group);
    else
        glog.is(DEBUG1) && glog << warn << "No handler for static protobuf type: " << protobuf_type_name << " from socket id: " << socket_id << std::endl;
}




void goby::pb::DynamicProtobufNode::protobuf_inbox(const std::string& protobuf_type_name,
                                                   const void* data,
                                                   int size,
                                                   int socket_id,
                                                   const std::string& group
    )
{
    boost::shared_ptr<google::protobuf::Message> msg =
        goby::util::DynamicProtobufManager::new_protobuf_message(protobuf_type_name);
    msg->ParseFromArray(data, size);
    dynamic_protobuf_inbox(msg, socket_id, group);
}


