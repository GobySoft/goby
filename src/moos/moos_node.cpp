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

#include "goby/common/logger.h"

#include "goby/util/binary.h"

using goby::common::ZeroMQService;
using goby::glog;
using namespace goby::common::logger_lock;
using namespace goby::common::logger;


goby::moos::MOOSNode::MOOSNode(ZeroMQService* service)
    : goby::common::NodeInterface<CMOOSMsg>(service)
{
    boost::mutex::scoped_lock lock(glog.mutex());    
    glog.add_group("in_hex", common::Colors::green, "Goby MOOS (hex) - Incoming");
    glog.add_group("out_hex", common::Colors::magenta, "Goby MOOS (hex) - Outgoing");

}


void goby::moos::MOOSNode::inbox(common::MarshallingScheme marshalling_scheme,
                                 const std::string& identifier,
                                 const void* data,
                                 int size,
                                 int socket_id)
{

    glog.is(DEBUG2, lock) && 
        glog << group("in_hex") << "Received marshalling scheme: " << marshalling_scheme << std::endl << unlock;
    
    if(marshalling_scheme == goby::common::MARSHALLING_MOOS)
    {
        boost::shared_ptr<CMOOSMsg> msg(new CMOOSMsg);
        std::string bytes(static_cast<const char*>(data), size);

        glog.is(DEBUG2, lock) && 
            glog << group("in_hex") << goby::util::hex_encode(bytes) << std::endl << unlock;
        MOOSSerializer::parse(msg.get(), bytes);

        const std::string& key = msg->GetKey();
        newest_vars[key] = msg;
        moos_inbox(*msg);
    }
}

void goby::moos::MOOSNode::send(const CMOOSMsg& msg, int socket_id)
{            

    std::string bytes;
    MOOSSerializer::serialize(msg, &bytes);

    glog.is(DEBUG1, lock) &&
        glog << "Sent: " << "CMOOSMsg/" << msg.GetKey() << "/"  << std::endl << unlock;


    glog.is(DEBUG2, lock) &&
        glog << group("out_hex") << goby::util::hex_encode(bytes) << std::endl << unlock;

    zeromq_service()->send(goby::common::MARSHALLING_MOOS, "CMOOSMsg/" + msg.GetKey() + "/", &bytes[0], bytes.size(), socket_id);
}

void goby::moos::MOOSNode::subscribe(const std::string& full_or_partial_moos_name, int socket_id)
{
    std::string trimmed_name = boost::trim_copy(full_or_partial_moos_name);
    unsigned size = trimmed_name.size();
    if(!size)
    {
        zeromq_service()->subscribe(goby::common::MARSHALLING_MOOS, "CMOOSMsg/", socket_id);
    }
    else if(trimmed_name[size-1] == '*')
    {
        zeromq_service()->subscribe(goby::common::MARSHALLING_MOOS, "CMOOSMsg/" + trimmed_name.substr(0, size-1), socket_id);
    }
    else
    {
        zeromq_service()->subscribe(goby::common::MARSHALLING_MOOS, "CMOOSMsg/" + trimmed_name + "/", socket_id);
    }
}

void goby::moos::MOOSNode::unsubscribe(const std::string& full_or_partial_moos_name, int socket_id)
{
    std::string trimmed_name = boost::trim_copy(full_or_partial_moos_name);
    unsigned size = trimmed_name.size();
    if(!size)
    {
        glog.is(WARN, lock) && glog << warn << "Not unsubscribing for empty string!" << std::endl << unlock;
    }
    else if(trimmed_name[size-1] == '*')
    {
        zeromq_service()->unsubscribe(goby::common::MARSHALLING_MOOS, "CMOOSMsg/" + trimmed_name.substr(0, size-1), socket_id);
    }
    else
    {
        zeromq_service()->unsubscribe(goby::common::MARSHALLING_MOOS, "CMOOSMsg/" + trimmed_name + "/", socket_id);
    }
}




CMOOSMsg& goby::moos::MOOSNode::newest(const std::string& key)
{
    if(!newest_vars.count(key))
        newest_vars[key] = boost::shared_ptr<CMOOSMsg>(new CMOOSMsg);
    
    return *newest_vars[key];
}

