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
#include "goby/util/logger.h"

#include "queue.h"
#include "queue_manager.h"

using goby::util::goby_time;

goby::acomms::Queue::Queue(const protobuf::QueueConfig cfg /* = 0 */,
                           std::ostream* log /* = 0 */)
    : cfg_(cfg),
      last_send_time_(goby_time()),
      log_(log)
{}


bool goby::acomms::Queue::push_message(const google::protobuf::Message& dccl_msg)
{
    boost::shared_ptr<google::protobuf::Message> new_dccl_msg(dccl_msg.New());
    new_dccl_msg->CopyFrom(dccl_msg);

    protobuf::ModemDataTransmission data_msg;
    
    goby::acomms::DCCLCodec* codec = goby::acomms::DCCLCodec::get();
    data_msg.mutable_data()->resize(codec->size(&dccl_msg));
    
    push_message(data_msg, new_dccl_msg);
}



// add a new message
bool goby::acomms::Queue::push_message(const protobuf::ModemDataTransmission& encoded_msg, boost::shared_ptr<google::protobuf::Message> dccl_msg /* =  boost::shared_ptr<google::protobuf::Message> */)
{
    if(encoded_msg.data().empty())
    {
        if(log_) *log_ << group("q_out") << warn
                       << "empty message attempted to be pushed to queue "
                       << cfg_.name() << std::endl;
        return false;
    }
    else if(cfg_.key().type() == protobuf::QUEUE_CCL && encoded_msg.data()[0] != char(cfg_.key().id()))
    {
        if(log_) *log_ << group("q_out") << warn
                       << "CCL message attempted to be pushed to queue "
                       << cfg_.name() << " that doesn't have proper CCL byte. " 
                       << "expecting: " << cfg_.key().id() << ", given: " << int(encoded_msg.data()[0])
                       << std::endl;
        return false;
    }    
    
    messages_.push_back(QueuedMessage());
    messages_.back().encoded_msg.CopyFrom(encoded_msg);
    messages_.back().dccl_msg = dccl_msg;
    protobuf::ModemDataTransmission* new_data_msg = &messages_.back().encoded_msg;

    if(!new_data_msg->has_ack_requested())
        new_data_msg->set_ack_requested(cfg_.ack());
    
    // needed for CCL messages
    new_data_msg->mutable_base()->set_src(QueueManager::modem_id_);
    
    // pop messages off the stack if the queue is full
    if(cfg_.max_queue() && messages_.size() > cfg_.max_queue())
    {        
        messages_it it_to_erase =
            cfg_.newest_first() ? messages_.begin() : messages_.end();

        // want "back" iterator not "end"
        if(it_to_erase == messages_.end()) --it_to_erase;
        
        // if we were waiting for an ack for this, erase that too
        waiting_for_ack_it it = find_ack_value(it_to_erase);
        if(it != waiting_for_ack_.end()) waiting_for_ack_.erase(it);        
        
        if(log_) *log_ << group("pop") << "queue exceeded for " << cfg_.name() <<
                     ". removing: " << it_to_erase->encoded_msg << std::endl;

        messages_.erase(it_to_erase);
    }
    
    if(log_) *log_ << group("push") << "pushing" << " to send stack "
                   << cfg_.name() << " (qsize " << size() <<  "/"
                   << cfg_.max_queue() << "): " << encoded_msg << std::endl;
    
    return true;     
}

goby::acomms::messages_it goby::acomms::Queue::next_message_it()
{
    messages_it it_to_give =
        cfg_.newest_first() ? messages_.end() : messages_.begin();
    if(it_to_give == messages_.end()) --it_to_give; // want "back" iterator not "end"    
    
    // find a value that isn't already waiting to be acknowledged
    while(find_ack_value(it_to_give) != waiting_for_ack_.end())
        cfg_.newest_first() ? --it_to_give : ++it_to_give;

    return it_to_give;
}

goby::acomms::QueuedMessage goby::acomms::Queue::give_data(const protobuf::ModemDataRequest& request_msg)
{
    messages_it it_to_give = next_message_it();

    bool ack = it_to_give->encoded_msg.ack_requested();
    // broadcast cannot acknowledge
    if(it_to_give->encoded_msg.base().dest() == BROADCAST_ID && ack == true)
    {
        if(log_) *log_ << group("pop") << warn << "overriding ack request and setting ack = false because dest = BROADCAST (0) cannot acknowledge messages" << std::endl;
        ack = false;
    }

    it_to_give->encoded_msg.set_ack_requested(ack);

    if(ack)
        waiting_for_ack_.insert(std::pair<unsigned, messages_it>(request_msg.frame(),
                                                                 it_to_give));

    last_send_time_ = goby_time();
    
    
    return *it_to_give;
}


// gives priority values. returns false if in blackout interval or if no data or if messages of wrong size, true if not in blackout
bool goby::acomms::Queue::priority_values(double& priority,
                                          boost::posix_time::ptime& last_send_time,
                                          const protobuf::ModemDataRequest& request_msg,
                                          const protobuf::ModemDataTransmission& data_msg)
{
    priority = util::time_duration2double((goby_time()-last_send_time_))/cfg_.ttl()*cfg_.value_base();
    
    last_send_time = last_send_time_;

    // no messages left to send
    if(messages_.size() <= waiting_for_ack_.size()) return false;
    
    protobuf::ModemDataTransmission& next_msg = next_message_it()->encoded_msg;

    // for followup user-frames, destination must be either zero (broadcast)
    // or the same as the first user-frame

    if (last_send_time_ + boost::posix_time::seconds(cfg_.blackout_time()) > goby_time())
    {
        if(log_) *log_<< group("priority") << "\t" << cfg_.name() << " is in blackout" << std::endl;
        return false;
    }
    // wrong size
    else if(request_msg.has_max_bytes() &&
            (next_msg.data().size() > (request_msg.max_bytes() - data_msg.data().size())))
    {
        if(log_) *log_<< group("priority") << "\t" << cfg_.name() << " next message is too large {" << next_msg.data().size() << "}" << std::endl;
        return false;
    }
    // wrong destination
    else if((data_msg.base().has_dest()
             && data_msg.base().dest() != QUERY_DESTINATION_ID
             && next_msg.base().dest() != BROADCAST_ID
             && data_msg.base().dest() != next_msg.base().dest()))
    {
        if(log_) *log_<< group("priority") << "\t" <<  cfg_.name() << " next message has wrong destination  (must be BROADCAST (0) or same as first user-frame)" << std::endl;
        return false; 
    }
    // wrong ack value UNLESS message can be broadcast
    else if((data_msg.has_ack_requested() && !data_msg.ack_requested() &&
             next_msg.ack_requested() && data_msg.base().dest() != acomms::BROADCAST_ID))
    {
        if(log_) *log_<< group("priority") << "\t" <<  cfg_.name() << " next message requires ACK and the packet does not" << std::endl;
        return false; 
    }
    else // ok!
    {
        if(log_) *log_<< group("priority") << "\t" << cfg().name()
                      << " (" << next_msg.data().size()
                      << "B) has priority value"
                      << ": " << priority << std::endl;
        return true;
    }
    
}

bool goby::acomms::Queue::pop_message(unsigned frame)
{   
    if (cfg_.newest_first() && !messages_.back().encoded_msg.ack_requested())
    {
        stream_for_pop(*messages_.back().dccl_msg);
        messages_.pop_back();
    }
    else if(!cfg_.newest_first() && !messages_.front().encoded_msg.ack_requested())
    {
        stream_for_pop(*messages_.front().dccl_msg);
        messages_.pop_front();
    }
    else
    {
        return false;
    }    

    return true;
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
    if(log_) *log_ << group("pop") <<  "popping" << " from send stack "
                   << cfg_.name() << " (qsize " << size()-1
                   <<  "/" << cfg_.max_queue() << "): "  << dccl_msg << std::endl;
}

std::vector<boost::shared_ptr<google::protobuf::Message> > goby::acomms::Queue::expire()
{
    std::vector<boost::shared_ptr<google::protobuf::Message> > expired_msgs;
    
    while(!messages_.empty())
    {
        if((goby::util::as<boost::posix_time::ptime>(messages_.front().encoded_msg.base().time())
            + boost::posix_time::seconds(cfg_.ttl())) < goby_time())
        {
            expired_msgs.push_back(messages_.front().dccl_msg);
            if(log_) *log_ << group("pop") <<  "expiring" << " from send stack "
                           << cfg_.name() << " (qsize " << size()-1
                           <<  "/" << cfg_.max_queue() << "): "  << messages_.front().dccl_msg << std::endl;
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


std::string goby::acomms::Queue::summary() const 
{
    std::stringstream ss;
    ss << cfg_;
    return ss.str();
}


void goby::acomms::Queue::flush()
{
    if(log_) *log_ << group("pop") << "flushing stack " << cfg_.name() << " (qsize 0)" << std::endl;
    messages_.clear();
}        


std::ostream& goby::acomms::operator<< (std::ostream& os, const goby::acomms::Queue& oq)
{
    os << oq.summary();
    return os;
}

