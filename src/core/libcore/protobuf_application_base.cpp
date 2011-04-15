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

#include "protobuf_application_base.h"

#include "goby/util/logger.h"

using goby::util::glogger;
using goby::util::as;


goby::core::ProtobufApplicationBase::ProtobufApplicationBase(google::protobuf::Message* cfg /*= 0*/)
    : ZeroMQNode(&glogger()),
      MinimalApplicationBase(cfg)
{
    std::string multicast_connection = "epgm://" + base_cfg().ethernet_address() + ";" +
        base_cfg().multicast_address() + ":" + as<std::string>(base_cfg().ethernet_port());

    start_sockets(multicast_connection);
}

goby::core::ProtobufApplicationBase::~ProtobufApplicationBase()
{
}


void goby::core::ProtobufApplicationBase::inbox(MarshallingScheme marshalling_scheme,
                                                const std::string& identifier,
                                                const void* data,
                                                int size)
{
    if(static_cast<MarshallingScheme>(marshalling_scheme) != MARSHALLING_PROTOBUF)
    {
        glogger() << warn
                  << "Unknown Marshalling Scheme; should be MARSHALLING_PROTOBUF, got: "
                  << marshalling_scheme << std::endl;
    }
    else
    {
        inbox(identifier, data, size);
    }
}
