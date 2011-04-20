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

using goby::util::glogger;
using goby::util::as;

void goby::core::ProtobufNode::inbox(MarshallingScheme marshalling_scheme,
                                     const std::string& identifier,
                                     const void* data,
                                     int size)
{
    if(marshalling_scheme == MARSHALLING_PROTOBUF)
        protobuf_inbox(identifier.substr(0, identifier.find("/")), data, size);
}


void goby::core::ProtobufNode::publish(const google::protobuf::Message& msg)
{
    int size = msg.ByteSize();
    char buffer[size];
    msg.SerializeToArray(&buffer, size);
    ZeroMQNode::publish(MARSHALLING_PROTOBUF, msg.GetDescriptor()->full_name() + "/",
                        &buffer, size);
}
            
void goby::core::ProtobufNode::subscribe(const std::string& identifier)
{
    ZeroMQNode::subscribe(MARSHALLING_PROTOBUF, identifier);
}


bool goby::core::StaticProtobufNode::__is_valid_filter(const google::protobuf::Descriptor* descriptor, const protobuf::Filter& filter)
{
    using namespace google::protobuf;
    // check filter for exclusions
    const FieldDescriptor* field_descriptor = descriptor->FindFieldByName(filter.key());
    if(!field_descriptor // make sure it exists
       || (field_descriptor->cpp_type() == FieldDescriptor::CPPTYPE_ENUM || // exclude enums
           field_descriptor->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) // exclude embedded
       || field_descriptor->is_repeated()) // no repeated fields for filter
    {
        glogger() << die << "bad filter: " << filter << "for message descriptor: " << descriptor->DebugString() << std::endl;
        return false;
    }
    return true;
}

void goby::core::StaticProtobufNode::protobuf_inbox(const std::string& protobuf_type_name,
                                                    const void* data,
                                                    int size)
{
    boost::unordered_map<std::string, boost::shared_ptr<SubscriptionBase> >::iterator it = subscriptions_.find(protobuf_type_name);
    
    if(it != subscriptions_.end())
        it->second->post(data, size);
    else
        throw(std::runtime_error("No subscription to protobuf type: " + protobuf_type_name));
}




void goby::core::DynamicProtobufNode::protobuf_inbox(const std::string& protobuf_type_name,
                                                     const void* data,
                                                     int size)
{
    boost::shared_ptr<google::protobuf::Message> msg =
        DynamicProtobufManager::new_protobuf_message(protobuf_type_name);
    msg->ParseFromArray(data, size);
    dynamic_protobuf_inbox(msg);
}


