// copyright 2008, 2009 t. schneider tes@mit.edu
//
// this file is part of the Queue Library (libqueue),
// the goby-acomms message queue manager. goby-acomms is a collection of 
// libraries for acoustic underwater networking
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

#include "goby/acomms/acomms_constants.h"
#include "goby/common/logger.h"

#include "queue.h"
#include "queue_manager.h"
#include "goby/common/protobuf/acomms_option_extensions.pb.h"

using goby::common::goby_time;

using namespace goby::common::logger;

goby::acomms::Queue::Queue(const google::protobuf::Descriptor* desc /*= 0*/, QueueManager* parent /*= 0*/)
    : desc_(desc),
      parent_(parent),
      last_send_time_(goby_time())
{

}


// add a new message
bool goby::acomms::Queue::push_message(const protobuf::QueuedMessageMeta& meta_msg,
                                       boost::shared_ptr<google::protobuf::Message> dccl_msg)
{
    if(meta_msg.non_repeated_size() == 0)
    {
        goby::glog.is(WARN) && glog << group(parent_->glog_out_group()) 
                                    << "empty message attempted to be pushed to queue "
                                    << name() << std::endl;
        return false;
    }
    
    messages_.push_back(QueuedMessage());
    messages_.back().meta = meta_msg;
    messages_.back().dccl_msg = dccl_msg;
    protobuf::QueuedMessageMeta* new_meta_msg = &messages_.back().meta;
    
    if(!new_meta_msg->has_ack_requested())
        new_meta_msg->set_ack_requested(queue_message_options().ack());
    
    // pop messages off the stack if the queue is full
    if(queue_message_options().max_queue() && messages_.size() > queue_message_options().max_queue())
    {        
        messages_it it_to_erase =
            queue_message_options().newest_first() ? messages_.begin() : messages_.end();

        // want "back" iterator not "end"
        if(it_to_erase == messages_.end()) --it_to_erase;
        
        // if we were waiting for an ack for this, erase that too
        waiting_for_ack_it it = find_ack_value(it_to_erase);
        if(it != waiting_for_ack_.end()) waiting_for_ack_.erase(it);        
        
        glog.is(DEBUG1) && glog << group(parent_->glog_pop_group()) << "queue exceeded for " << name() <<
            ". removing: " << it_to_erase->meta << std::endl;

        messages_.erase(it_to_erase);
    }
    
    glog.is(DEBUG1) && glog << group(parent_->glog_push_group()) << "pushing" << " to send stack "
                            << name() << " (qsize " << size() <<  "/"
                             << queue_message_options().max_queue() << "): ";
    
    glog.is(DEBUG1) && glog << *dccl_msg << std::endl;
    glog.is(DEBUG2) && glog << meta_msg << std::endl;    
    
    return true;     
}

goby::acomms::messages_it goby::acomms::Queue::next_message_it()
{
    messages_it it_to_give =
        queue_message_options().newest_first() ? messages_.end() : messages_.begin();
    if(it_to_give == messages_.end()) --it_to_give; // want "back" iterator not "end"    
    
    // find a value that isn't already waiting to be acknowledged
    while(find_ack_value(it_to_give) != waiting_for_ack_.end())
        queue_message_options().newest_first() ? --it_to_give : ++it_to_give;

    return it_to_give;
}

goby::acomms::QueuedMessage goby::acomms::Queue::give_data(unsigned frame)
{
    messages_it it_to_give = next_message_it();

    bool ack = it_to_give->meta.ack_requested();
    // broadcast cannot acknowledge
    if(it_to_give->meta.dest() == BROADCAST_ID && ack == true)
    {
        glog.is(DEBUG1) && glog << group(parent_->glog_pop_group()) << name() << ": overriding ack request and setting ack = false because dest = BROADCAST (0) cannot acknowledge messages" << std::endl;
        ack = false;
    }

    it_to_give->meta.set_ack_requested(ack);

    if(ack)
        waiting_for_ack_.insert(std::pair<unsigned, messages_it>(frame, it_to_give));

    last_send_time_ = goby_time();
    
    
    return *it_to_give;
}


// gives priority values. returns false if in blackout interval or if no data or if messages of wrong size, true if not in blackout
bool goby::acomms::Queue::get_priority_values(double* priority,
                                              boost::posix_time::ptime* last_send_time,
                                              const protobuf::ModemTransmission& request_msg,
                                              const std::string& data)
{
    *priority = common::time_duration2double((goby_time()-last_send_time_))/queue_message_options().ttl()*queue_message_options().value_base();

    *last_send_time = last_send_time_;

    // no messages left to send
    if(messages_.size() <= waiting_for_ack_.size())
        return false;
    
    protobuf::QueuedMessageMeta& next_msg = next_message_it()->meta;

    // for followup user-frames, destination must be either zero (broadcast)
    // or the same as the first user-frame

    if (last_send_time_ + boost::posix_time::seconds(queue_message_options().blackout_time()) > goby_time())
    {
        glog.is(DEBUG1) && glog << group(parent_->glog_priority_group()) << "\t" << name() << " is in blackout" << std::endl;
        return false;
    }
    // wrong size
    else if(request_msg.has_max_frame_bytes() &&
            (next_msg.non_repeated_size() > (request_msg.max_frame_bytes() - data.size())))
    {
        glog.is(DEBUG1) && glog << group(parent_->glog_priority_group()) << "\t" << name() << " next message is too large {" << next_msg.non_repeated_size() << "}" << std::endl;
        return false;
    }
    // wrong destination
    else if((request_msg.has_dest() &&
             !(request_msg.dest() == QUERY_DESTINATION_ID // can set to a real destination
               || next_msg.dest() == BROADCAST_ID // can switch to a real destination
               || request_msg.dest() == next_msg.dest()))) // same as real destination
    {
        glog.is(DEBUG1) && glog << group(parent_->glog_priority_group()) << "\t" <<  name() << " next message has wrong destination (must be BROADCAST (0) or same as first user-frame, is " << next_msg.dest() << ")" << std::endl;
        return false; 
    }
    // wrong ack value UNLESS message can be broadcast
    else if((request_msg.has_ack_requested() && !request_msg.ack_requested() &&
             next_msg.ack_requested() && request_msg.dest() != acomms::BROADCAST_ID))
    {
        glog.is(DEBUG1) && glog << group(parent_->glog_priority_group()) << "\t" <<  name() << " next message requires ACK and the packet does not" << std::endl;
        return false; 
    }
    else // ok!
    {
        glog.is(DEBUG1) && glog << group(parent_->glog_priority_group()) << "\t" << name()
                                << " (" << next_msg.non_repeated_size()
                                << "B) has priority value"
                                << ": " << *priority << std::endl;
        return true;
    }
    
}

bool goby::acomms::Queue::pop_message(unsigned frame)
{
    std::list<QueuedMessage>::iterator back_it = messages_.end();
    --back_it;  // gives us "back" iterator
    std::list<QueuedMessage>::iterator front_it = messages_.begin();
    
    // find the first message that isn't waiting for an ack
    std::list<QueuedMessage>::iterator it = queue_message_options().newest_first() ? back_it : front_it;

    while(true)
    {
        if(!it->meta.ack_requested())
        {
            stream_for_pop(*it->dccl_msg);
            messages_.erase(it);
            return true;
        }
        
        if(it == (queue_message_options().newest_first() ? front_it : back_it))
            return false;
        
        queue_message_options().newest_first() ? --it: ++it;
    }
    return false;
}

bool goby::acomms::Queue::pop_message_ack(unsigned frame, boost::shared_ptr<google::protobuf::Message>& removed_msg)
{
    // pop message from the ack stack
    if(waiting_for_ack_.count(frame))
    {
        // remove a messages in this frame that needs ack
        waiting_for_ack_it it = waiting_for_ack_.find(frame);
        removed_msg = (it->second)->dccl_msg;

        stream_for_pop(*removed_msg);

        // remove the message
        messages_.erase(it->second);
        // clear the acknowledgement map entry for this message
        waiting_for_ack_.erase(it);
    }
    else
    {
        return false;
    }
    
    return true;    
}

void goby::acomms::Queue::stream_for_pop(const google::protobuf::Message& dccl_msg)
{
    glog.is(DEBUG1) && glog  << group(parent_->glog_pop_group()) <<  "popping" << " from send stack "
                              << name() << " (qsize " << size()-1
                              <<  "/" << queue_message_options().max_queue() << "): "  << dccl_msg << std::endl;
}

std::vector<boost::shared_ptr<google::protobuf::Message> > goby::acomms::Queue::expire()
{
    std::vector<boost::shared_ptr<google::protobuf::Message> > expired_msgs;
    
    while(!messages_.empty())
    {
        if((goby::util::as<boost::posix_time::ptime>(messages_.front().meta.time())
            + boost::posix_time::seconds(queue_message_options().ttl())) < goby_time())
        {
            expired_msgs.push_back(messages_.front().dccl_msg);
            glog.is(DEBUG1) && glog  << group(parent_->glog_pop_group()) <<  "expiring" << " from send stack "
                                      << name() << " (qsize " << size()-1
                                      <<  "/" << queue_message_options().max_queue() << "): "  << messages_.front().dccl_msg << std::endl;
            // if we were waiting for an ack for this, erase that too
            waiting_for_ack_it it = find_ack_value(messages_.begin());
            if(it != waiting_for_ack_.end()) waiting_for_ack_.erase(it);
            
            messages_.pop_front();
        }
        else
        {
            return expired_msgs;
        } 
    }

    return expired_msgs;
}

goby::acomms::waiting_for_ack_it goby::acomms::Queue::find_ack_value(messages_it it_to_find)
{
    waiting_for_ack_it n = waiting_for_ack_.end();
    for(waiting_for_ack_it it = waiting_for_ack_.begin(); it != n; ++it)
    {
        if(it->second == it_to_find)
            return it;
    }
    return n;
}


void goby::acomms::Queue::info(std::ostream* os) const
{
    *os << "Queue [[" << name() << "]] contains " << messages_.size() << " message(s)." << "\n"
        << "Configured options: \n" << desc_->options().DebugString();
}


void goby::acomms::Queue::flush()
{
    glog.is(DEBUG1) && glog  << group(parent_->glog_pop_group()) << "flushing stack " << name() << " (qsize 0)" << std::endl;
    messages_.clear();
    waiting_for_ack_.clear();
}        


std::ostream& goby::acomms::operator<< (std::ostream& os, const goby::acomms::Queue& oq)
{
    oq.info(&os);
    return os;
}

