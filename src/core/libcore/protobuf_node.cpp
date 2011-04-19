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

using goby::util::glogger;
using goby::util::as;

void goby::core::ProtobufNode::inbox(MarshallingScheme marshalling_scheme,
                                     const std::string& identifier,
                                     const void* data,
                                     int size)
{
    if(static_cast<MarshallingScheme>(marshalling_scheme) == MARSHALLING_PROTOBUF)
        protobuf_inbox(identifier, data, size);
}


void goby::core::ProtobufNode::publish(const google::protobuf::Message& msg)
{
    int size = msg.ByteSize();
    char buffer[size];
    msg.SerializeToArray(&buffer, size);
    ZeroMQNode::publish(MARSHALLING_PROTOBUF, msg.GetDescriptor()->full_name(),
                        &buffer, size);
}
            
void goby::core::ProtobufNode::subscribe(const std::string& identifier)
{
    ZeroMQNode::subscribe(MARSHALLING_PROTOBUF, identifier);
}
