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

using goby::util::goby_time;

goby::acomms::Queue::Queue(const QueueConfig cfg /* = 0 */,
                    std::ostream* os /* = 0 */,
                    const unsigned& modem_id /* = 0 */)
    : cfg_(cfg),
      on_demand_(false),
      last_send_time_(goby_time()),
      modem_id_(modem_id),
      os_(os)
{}


// add a new message
bool goby::acomms::Queue::push_message(ModemMessage& new_message)
{
    if(new_message.empty())
    {
        if(os_) *os_ << group("q_out") << warn
                     << "empty message attempted to be pushed to queue "
                     << cfg_.name() << std::endl;
        return false;
    }
    
    new_message.set_ack(new_message.ack_set() ? new_message.ack() : cfg_.ack());

    // BROADCAST cannot ACK messages
    if(new_message.dest() == acomms::BROADCAST_ID) new_message.set_ack(false);
    
    // needed for CCL messages
    new_message.set_src(modem_id_);
        
    messages_.push_back(new_message);

    
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
        
        if(os_) *os_ << group("pop") << "queue exceeded for " << cfg_.name() <<
                    ". removing: " << it_to_erase->snip() << std::endl;

        messages_.erase(it_to_erase);
    }
    
    if(os_) *os_ << group("push") << "pushing" << " to send stack "
                 << cfg_.name() << " (qsize " << size() <<  "/"
                 << cfg_.max_queue() << "): " << new_message.snip() << std::endl;
    
    return true;     
}

messages_it goby::acomms::Queue::next_message_it()
{
    messages_it it_to_give =
        cfg_.newest_first() ? messages_.end() : messages_.begin();
    if(it_to_give == messages_.end()) --it_to_give; // want "back" iterator not "end"    
    
    // find a value that isn't already waiting to be acknowledged
    while(find_ack_value(it_to_give) != waiting_for_ack_.end())
        cfg_.newest_first() ? --it_to_give : ++it_to_give;

    return it_to_give;
}

goby::acomms::ModemMessage goby::acomms::Queue::give_data(unsigned frame)
{
    messages_it it_to_give = next_message_it();
    
    if(it_to_give->ack()) waiting_for_ack_.insert(std::pair<unsigned, messages_it>(frame, it_to_give));
    
    return *it_to_give;
}


// gives priority values. returns false if in blackout interval or if no data or if messages of wrong size, true if not in blackout
bool goby::acomms::Queue::priority_values(double& priority,
                                          boost::posix_time::ptime& last_send_time,
                                          ModemMessage& message)
{
    priority = util::time_duration2double((goby_time()-last_send_time_))/cfg_.ttl()*cfg_.value_base();
    
    last_send_time = last_send_time_;

    // no messages left to send
    if(messages_.size() <= waiting_for_ack_.size()) return false;
    
    messages_it next_msg_it = next_message_it();

    // for followup user-frames, destination must be either zero (broadcast)
    // or the same as the first user-frame
    if((message.dest_set() && next_msg_it->dest() != acomms::BROADCAST_ID && message.dest() != next_msg_it->dest()))
    {
        if(os_) *os_<< group("priority") << "\t" <<  cfg_.name() << " next message has wrong destination  (must be BROADCAST (0) or same as first user-frame)" << std::endl;
        return false; 
    }
    else if((message.ack_set() && message.ack() != next_msg_it->ack()))
    {
        if(os_) *os_<< group("priority") << "\t" <<  cfg_.name() << " next message requires ACK and the packet does not" << std::endl;
        return false; 
    }
    else if(next_msg_it->size() > message.size())
    {
        if(os_) *os_<< group("priority") << "\t" << cfg_.name() << " next message is too large {" << next_msg_it->size() << "}" << std::endl;
        return false;
    }
    else if (last_send_time_ + boost::posix_time::seconds(cfg_.blackout_time()) > goby_time())
    {
        if(os_) *os_<< group("priority") << "\t" << cfg_.name() << " is in blackout" << std::endl;
        return false;
    }

    return true;
}

bool goby::acomms::Queue::pop_message(unsigned frame)
{   
    if (cfg_.newest_first() && !messages_.back().ack())
    {
        stream_for_pop(messages_.back().snip());
        messages_.pop_back();
    }
    else if(!cfg_.newest_first() && !messages_.front().ack())
    {
        stream_for_pop(messages_.front().snip());
        messages_.pop_front();
    }
    else
    {
        return false;
    }
    
    // only set last_send_time on a successful message (after ack, if one is desired)
    last_send_time_ = goby_time();

    return true;
}

bool goby::acomms::Queue::pop_message_ack(unsigned frame, ModemMessage& msg)
{
    // pop message from the ack stack
    if(waiting_for_ack_.count(frame))
    {
        // remove a messages in this frame that needs ack
        waiting_for_ack_it it = waiting_for_ack_.find(frame);
        msg = *(it->second);

        stream_for_pop(msg.snip());

        // remove the message
        messages_.erase(it->second);
        // clear the acknowledgement map entry for this message
        waiting_for_ack_.erase(it);
        
    }
    else
    {
        return false;
    }
    
    // only set last_send_time on a successful message (after ack, if one is desired)
    last_send_time_ = goby_time();
    
    return true;    
}

void goby::acomms::Queue::stream_for_pop(const std::string& snip)
{
    if(os_) *os_ << group("pop") <<  "popping" << " from send stack "
                 << cfg_.name() << " (qsize " << size()-1
                 <<  "/" << cfg_.max_queue() << "): "  << snip << std::endl;
}

std::vector<goby::acomms::ModemMessage> goby::acomms::Queue::expire()
{
    std::vector<ModemMessage> expired_msgs;
    
    while(!messages_.empty())
    {
        if((messages_.front().time() + boost::posix_time::seconds(cfg_.ttl())) < goby_time())
        {
            expired_msgs.push_back(messages_.front());
            if(os_) *os_ << group("pop") <<  "expiring" << " from send stack "
                         << cfg_.name() << " (qsize " << size()-1
                         <<  "/" << cfg_.max_queue() << "): "  << messages_.front().snip() << std::endl;
            messages_.pop_front();
        }
        else
        {
            return expired_msgs;
        } 
    }

    return expired_msgs;
}

unsigned goby::acomms::Queue::give_dest()
{
    return cfg_.newest_first() ? messages_.back().dest() : messages_.front().dest();
}


waiting_for_ack_it goby::acomms::Queue::find_ack_value(messages_it it_to_find)
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
    if(os_) *os_ << group("pop") << "flushing stack " << cfg_.name() << " (qsize 0)" << std::endl;
    messages_.clear();
}        


std::ostream& goby::acomms::operator<< (std::ostream& os, const goby::acomms::Queue& oq)
{
    os << oq.summary();
    return os;
}

