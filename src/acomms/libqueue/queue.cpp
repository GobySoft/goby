// t. schneider tes@mit.edu 06.05.08
// ocean engineering graudate student - mit / whoi joint program
// massachusetts institute of technology (mit)
// laboratory for autonomous marine sensing systems (lamss)
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


// OutQueue - a class that handles all information pertaining to a given
// MOOS variable specified to send (send = ... configuration parameter)
// used in pAcommsHandler

// essentially each OutQueue maintains its own stack of messages to send
// and the configuration data pertaining to its priority in relation to other messages

#include "queue.h"

#include "tes_utils.h"
#include "term_color.h"
#include "acomms_constants.h"
#include "logger_manipulators.h"

queue::Queue::Queue(const QueueConfig cfg /* = 0 */,
                    std::ostream* os /* = 0 */,
                    const unsigned& modem_id /* = 0 */)
    : cfg_(cfg),
      on_demand_(false),
      last_send_time_(time(NULL)),
      modem_id_(modem_id),
      os_(os)
{}


// add a new message
bool queue::Queue::push_message(micromodem::Message& new_message)
{
    if(new_message.empty())
    {
        if(os_) *os_ << group("q_out") << warn
                     << "empty message attempted to be pushed to queue "
                     << cfg_.name() << std::endl;
        return false;
    }
            
    // most significant byte is CCL message type,
    // second most is split into two nibbles
    //     most significant nibble is remaining packets to receive from this id (modem and variable)
    //     least significant nibble is variableID
    
    if(cfg_.type() == queue_data)
    {
        std::string hex_variableID;
        tes_util::number2hex_string(hex_variableID, cfg_.id());

        new_message.set_data(acomms_util::DCCL_CCL_HEADER_STR +
                             hex_variableID +
                             new_message.data());
    }    
    new_message.set_src(modem_id_);
    new_message.set_ack(cfg_.ack());
        
    messages_.push_back(new_message);

    
    // push messages off the stack if the queue is full
    if(messages_.size() > cfg_.max_queue() && cfg_.max_queue() > 0)
    {        
        messages_it it_to_erase =
            cfg_.newest_first() ? messages_.begin() : messages_.end();

        // want "back" iterator not "end"
        if(it_to_erase == messages_.end()) --it_to_erase;
        
        // if we were waiting for an ack for this, erase that too
        waiting_for_ack_it it = find_ack_value(it_to_erase);
        if(it != waiting_for_ack_.end()) waiting_for_ack_.erase(it);        
        
        if(os_) *os_ << "queue exceeded for " << cfg_.name() <<
                    ". removing: " << it_to_erase->snip() << std::endl;

        messages_.erase(it_to_erase);
    }
    
    if(os_) *os_ << group("push") << "pushing" << " to send stack "
                 << cfg_.name() << " (qsize " << size() <<  "/"
                 << cfg_.max_queue() << "): " << new_message.snip() << std::endl;    
    
    return true;     
}

messages_it queue::Queue::next_message_it()
{
    messages_it it_to_give =
        cfg_.newest_first() ? messages_.end() : messages_.begin();
    if(it_to_give == messages_.end()) --it_to_give; // want "back" iterator not "end"    
    
    // find a value that isn't already waiting to be acknowledged
    while(find_ack_value(it_to_give) != waiting_for_ack_.end())
        cfg_.newest_first() ? --it_to_give : ++it_to_give;

    return it_to_give;
}

micromodem::Message queue::Queue::give_data(unsigned frame)
{
    messages_it it_to_give = next_message_it();
    
    if(cfg_.ack()) waiting_for_ack_.insert(std::pair<unsigned, messages_it>(frame, it_to_give));
    
    return *it_to_give;
}


// gives priority values. returns false if in blackout interval or if no data or if messages of wrong size, true if not in blackout
bool queue::Queue::priority_values(double* priority,
                                   double* last_send_time,
                                   micromodem::Message* message,
                                   std::string* error)
{
    double now = time(NULL);
    
    *priority =
        cfg_.priority_time_const() ? cfg_.priority_base() * exp((now-last_send_time_) / cfg_.priority_time_const()) : cfg_.priority_base();
    
    *last_send_time = last_send_time_;
    
    if(messages_.size() <= waiting_for_ack_.size())
    {
        *error = "no available messages";
        return false;
    }
        
    micromodem::Message next_message = *next_message_it();

    // for followup user-frames, destination must be either zero (broadcast)
    // or the same as the first user-frame
    if((message->dest_set() && next_message.dest() != acomms_util::BROADCAST_ID && message->dest() != next_message.dest())
       || (message->ack_set() && message->ack() != next_message.ack()))
    {
        *error = cfg_.name() + " next message has wrong destination  (must be BROADCAST (0) or same as first user-frame) or requires ACK and the packet does not";
        return false; 
    }
    else if(next_message.size() > message->size())
    {
        *error = cfg_.name() + " next message is too large {"+ boost::lexical_cast<std::string>(next_message.size()) +  "}";
        return false;
    }
    else if (last_send_time_ + cfg_.blackout_time() > time(NULL))
    {
        *error = cfg_.name() + " is in blackout";
        return false;
    }

    return true;
}

bool queue::Queue::pop_message(unsigned frame)
{
    if (cfg_.newest_first() && !cfg_.ack())
    {
        stream_for_pop(messages_.back().snip());
        messages_.pop_back();
    }
    else if(!cfg_.newest_first() && !cfg_.ack())
    {
        stream_for_pop(messages_.front().snip());
        messages_.pop_front();
    }
    else
    {
        return false;
    }
    
    // only set last_send_time on a successful message (after ack, if one is desired)
    last_send_time_ = time(NULL);

    return true;
}

bool queue::Queue::pop_message_ack(unsigned frame, micromodem::Message& msg)
{
    // pop message from the ack stack
    if(cfg_.ack() && frame && waiting_for_ack_.count(frame))
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
    last_send_time_ = time(NULL);
    
    return true;    
}

void queue::Queue::stream_for_pop(const std::string& snip)
{
    if(os_) *os_ << group("pop") <<  "popping" << " from send stack "
                 << cfg_.name() << " (qsize " << size()-1
                 <<  "/" << cfg_.max_queue() << "): "  << snip << std::endl;
}


unsigned queue::Queue::give_dest()
{
    return cfg_.newest_first() ? messages_.back().dest() : messages_.front().dest();
}


waiting_for_ack_it queue::Queue::find_ack_value(messages_it it_to_find)
{
    waiting_for_ack_it n = waiting_for_ack_.end();
    for(waiting_for_ack_it it = waiting_for_ack_.begin(); it != n; ++it)
    {
        if(it->second == it_to_find)
            return it;
    }
    return n;
}


std::string queue::Queue::summary() const 
{
    std::stringstream ss;
    ss << cfg_;
    return ss.str();
}


void queue::Queue::flush()
{
    if(os_) *os_ << group("pop") << "flushing stack " << cfg_.name() << " (qsize 0)" << std::endl;
    messages_.clear();
}        


std::ostream& queue::operator<< (std::ostream& os, const queue::Queue& oq)
{
    os << oq.summary();
    return os;
}

