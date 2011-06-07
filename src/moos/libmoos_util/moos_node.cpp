// copyright 2011 t. schneider tes@mit.edu
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

#include "moos_node.h"

#include "goby/util/logger.h"

#include "goby/util/binary.h"

using goby::core::ZeroMQNode;
using goby::glog;

goby::moos::MOOSNode::MOOSNode(ZeroMQNode* service)
    : goby::core::NodeInterface<CMOOSMsg>(service)
{
    zeromq_service()->connect_inbox_slot(&goby::moos::MOOSNode::inbox, this);
        
    glog.add_group("in_hex", util::Colors::green, "Goby MOOS (hex) - Incoming");
    glog.add_group("out_hex", util::Colors::magenta, "Goby MOOS (hex) - Outgoing");

}


void goby::moos::MOOSNode::inbox(core::MarshallingScheme marshalling_scheme,
                                 const std::string& identifier,
                                 const void* data,
                                 int size,
                                 int socket_id)
{
    if(marshalling_scheme == goby::core::MARSHALLING_MOOS)
    {
        CMOOSMsg msg;
        std::string bytes(static_cast<const char*>(data), size);

        glog.is(debug2) && 
            glog << group("in_hex") << goby::util::hex_encode(bytes) << std::endl;
        MOOSSerializer::parse(&msg, bytes);
        moos_inbox(msg);
    }
    
}

void goby::moos::MOOSNode::send(CMOOSMsg& msg, int socket_id)
{            

    std::string bytes;
    MOOSSerializer::serialize(msg, &bytes);

    glog.is(debug2) &&
        glog << group("out_hex") << goby::util::hex_encode(bytes) << std::endl;

    zeromq_service()->send(goby::core::MARSHALLING_MOOS, "CMOOSMsg/" + msg.GetKey() + "/", &bytes[0], bytes.size(), socket_id);
}

void goby::moos::MOOSNode::subscribe(const std::string& full_or_partial_moos_name, int socket_id)
{
    zeromq_service()->subscribe(goby::core::MARSHALLING_MOOS, full_or_partial_moos_name, socket_id);
}

