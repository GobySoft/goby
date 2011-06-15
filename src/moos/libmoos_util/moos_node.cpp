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

using goby::core::ZeroMQService;
using goby::glog;
using namespace goby::util::logger_lock;


goby::moos::MOOSNode::MOOSNode(ZeroMQService* service)
    : goby::core::NodeInterface<CMOOSMsg>(service),
      global_blackout_(boost::posix_time::not_a_date_time)
{
    zeromq_service()->connect_inbox_slot(&goby::moos::MOOSNode::inbox, this);

    boost::mutex::scoped_lock lock(glog.mutex());    
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
        boost::shared_ptr<CMOOSMsg> msg(new CMOOSMsg);
        std::string bytes(static_cast<const char*>(data), size);

        glog.is(debug2, lock) && 
            glog << group("in_hex") << goby::util::hex_encode(bytes) << std::endl << unlock;
        MOOSSerializer::parse(msg.get(), bytes);

        const std::string& key = msg->GetKey();
        newest_vars[key] = msg;
        
        boost::posix_time::ptime this_time = goby::util::unix_double2ptime(msg->GetTime());
        const boost::posix_time::ptime& last_post_time = last_posted_[key];
        // check blackout conditions
        if(last_post_time.is_not_a_date_time() || // first post
           (blackout_.count(key) && // no local blackout
            this_time - last_post_time > blackout_[key]) || // passes local blackout
           (global_blackout_.is_not_a_date_time() || // no global blackout
            this_time - last_post_time  > global_blackout_)) // passes global blackout
        {
            last_posted_[key] = this_time;
            moos_inbox(*msg);
        }
        else
        {
            glog.is(debug2, lock) && 
                glog << "Message is in blackout: this time:" << this_time << ", last time: " << last_post_time << ", global blackout: " << global_blackout_ << ", local blackout: " << blackout_[key] << std::endl << unlock;
        }
        
    }
    
}

void goby::moos::MOOSNode::send(const CMOOSMsg& msg, int socket_id)
{            

    std::string bytes;
    MOOSSerializer::serialize(msg, &bytes);

    glog.is(debug2, lock) &&
        glog << group("out_hex") << goby::util::hex_encode(bytes) << std::endl << unlock;

    zeromq_service()->send(goby::core::MARSHALLING_MOOS, "CMOOSMsg/" + msg.GetKey() + "/", &bytes[0], bytes.size(), socket_id);
}

void goby::moos::MOOSNode::subscribe(const std::string& full_or_partial_moos_name, int socket_id)
{
    std::string trimmed_name = boost::trim_copy(full_or_partial_moos_name);
    unsigned size = trimmed_name.size();
    if(!size)
    {
        glog.is(warn, lock) && glog << "Not subscribing for empty string!" << std::endl << unlock;
    }
    else if(trimmed_name[size-1] == '*')
    {
        zeromq_service()->subscribe(goby::core::MARSHALLING_MOOS, "CMOOSMsg/" + trimmed_name.substr(0, size-1), socket_id);
    }
    else
    {
        zeromq_service()->subscribe(goby::core::MARSHALLING_MOOS, "CMOOSMsg/" + trimmed_name + "/", socket_id);
    }
}

void goby::moos::MOOSNode::unsubscribe(const std::string& full_or_partial_moos_name, int socket_id)
{
    std::string trimmed_name = boost::trim_copy(full_or_partial_moos_name);
    unsigned size = trimmed_name.size();
    if(!size)
    {
        glog.is(warn, lock) && glog << warn << "Not unsubscribing for empty string!" << std::endl << unlock;
    }
    else if(trimmed_name[size-1] == '*')
    {
        zeromq_service()->unsubscribe(goby::core::MARSHALLING_MOOS, "CMOOSMsg/" + trimmed_name.substr(0, size-1), socket_id);
    }
    else
    {
        zeromq_service()->unsubscribe(goby::core::MARSHALLING_MOOS, "CMOOSMsg/" + trimmed_name + "/", socket_id);
    }
}




CMOOSMsg& goby::moos::MOOSNode::newest(const std::string& key)
{
    if(!newest_vars.count(key))
        newest_vars[key] = boost::shared_ptr<CMOOSMsg>(new CMOOSMsg);
    
    return *newest_vars[key];
}

void goby::moos::MOOSNode::set_global_blackout(boost::posix_time::time_duration duration)
{
    glog.is(debug2, lock) &&
        glog << "Global blackout set to " << duration << std::endl << unlock;
    global_blackout_ = duration;
}


void goby::moos::MOOSNode::set_blackout(const std::string& key,
                                        boost::posix_time::time_duration duration)            
{
    glog.is(debug2, lock) &&
        glog << "Blackout for key "  << key << " set to " << duration << std::endl << unlock;
    blackout_[key] = duration;
}
