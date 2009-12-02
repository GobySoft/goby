// t. schneider tes@mit.edu 06.05.08
// ocean engineering graudate student - mit / whoi joint program
// massachusetts institute of technology (mit)
// laboratory for autonomous marine sensing systems (lamss)
// 
// this is out_queue.cpp, part of pAcommsHandler
//
// see the readme file within this directory for information
// pertaining to usage and purpose of this script.
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

#include "out_queue.h"

#include "tes_utils.h" 

#include "term_color.h"
#include "binary.h"
#include "acomms_constants.h"

using namespace termcolor;

OutQueue::OutQueue(FlexCout * ptout,
                   unsigned modem_id,
                   bool is_ccl):name_(""),
                                var_id_(""),        
                                ack_(false),
                                blackout_time_(0),
                                max_queue_(0),
                                newest_first_(false),
                                priority_(0),
                                priority_T_(120),
                                is_dccl_(false),
                                on_demand_(false),
                                last_send_time_(time(NULL)),
                                modem_id_(modem_id),
                                is_ccl_(is_ccl),
                                ptout_(ptout)
{ }


// add a new message
bool OutQueue::push_message(micromodem::Message* new_message)
{
    // most significant byte is CCL message type,
    // second most is split into two nibbles
    //     most significant nibble is remaining packets to receive from this id (modem and variable)
    //     least significant nibble is variableID

    if(!is_ccl_ && !is_dccl_)
    {
        std::string hex_variableID;
        acomms_util::number2hex_string(hex_variableID, var_id_as_num());

        new_message->set_data(acomms_util::DCCL_CCL_HEADER_STR +
                              hex_variableID +
                              new_message->data());
    }    
    new_message->set_src(modem_id_);
    new_message->set_ack(ack_);
        
    messages_.push_back(*new_message);

    
    // push messages off the stack if the queue is full
    if(messages_.size() > max_queue_ && max_queue_ > 0)
    {        
        messages_it it_to_erase =
            newest_first_ ? messages_.begin() : messages_.end();

        // want "back" iterator not "end"
        if(it_to_erase == messages_.end()) --it_to_erase;
        
        // if we were waiting for an ack for this, erase that too
        waiting_for_ack_it it = find_ack_value(it_to_erase);
        if(it != waiting_for_ack_.end()) waiting_for_ack_.erase(it);        
        
        *ptout_ << "queue exceeded for " << name_ <<
            ". removing: " << it_to_erase->snip() << std::endl;

        messages_.erase(it_to_erase);
    }
    
    *ptout_ << group("push") << lt_cyan
            << "pushing" << nocolor << " to send stack: "
            << name_ << ": " << new_message->snip() << std::endl;    
    
    return true;    
}

messages_it OutQueue::next_message_it()
{
    messages_it it_to_give =
        newest_first_ ? messages_.end() : messages_.begin();
    if(it_to_give == messages_.end()) --it_to_give; // want "back" iterator not "end"    
    
    // find a value that isn't already waiting to be acknowledged
    while(find_ack_value(it_to_give) != waiting_for_ack_.end())
        newest_first_ ? --it_to_give : ++it_to_give;

    return it_to_give;
}

micromodem::Message OutQueue::give_data(unsigned frame)
{
    messages_it it_to_give = next_message_it();
    
    if(ack_) waiting_for_ack_.insert(std::pair<unsigned, messages_it>(frame, it_to_give));
    
    return *it_to_give;
}


// gives priority values. returns false if in blackout interval or if no data or if messages of wrong size, true if not in blackout
bool OutQueue::priority_values(double* priority,
                               double* last_send_time,
                               micromodem::Message* message,
                               std::string* error)
{
    double now = time(NULL);
    
    *priority =
        priority_T_ ? priority_ * exp((now-last_send_time_) / priority_T_) : priority_;
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
        *error = name() + " next message has wrong destination  (must be BROADCAST (0) or same as first user-frame) or requires ACK and the packet does not";
        return false; 
    }
    else if(next_message.size() > message->size())
    {
        *error = name() + " next message is too large {"+ boost::lexical_cast<std::string>(next_message.size()) +  "}";
        return false;
    }
    else if (last_send_time_ + blackout_time_ > time(NULL))
    {
        *error = name() + " is in blackout";
        return false;
    }

    return true;
}

bool OutQueue::pop_message(unsigned frame = 0)
{
    if (newest_first_ && !ack_)
    {
        *ptout_ << group("pop") << lt_green << "popping" << nocolor
                << " from send stack: " << name_ << ": "
                << messages_.back().snip() << std::endl;
        messages_.pop_back();
    }
    else if(!newest_first_ && !ack_)
    {
        *ptout_ << group("pop") << lt_green << "popping" << nocolor
                << " from send stack: " << name_ << ": "
                << messages_.front().snip() << std::endl;
        messages_.pop_front();
    }
    // pop message from the ack stack
    else if(ack_ && frame && waiting_for_ack_.count(frame))
    {
        // remove all messages in this frame that need ack
        waiting_for_ack_it it = waiting_for_ack_.find(frame);
        micromodem::Message& message_to_remove = *(it->second);

        *ptout_ << group("pop") << lt_green << "popping" << nocolor
                << " from send stack: " << name_ << ": "
                << message_to_remove.snip() << std::endl;

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

unsigned OutQueue::give_dest()
{
    return newest_first_ ? messages_.back().dest() : messages_.front().dest();
}


waiting_for_ack_it OutQueue::find_ack_value(messages_it it_to_find)
{
    waiting_for_ack_it n = waiting_for_ack_.end();
    for(waiting_for_ack_it it = waiting_for_ack_.begin(); it != n; ++it)
    {
        if(it->second == it_to_find)
            return it;
    }
    return n;
}



std::ostream & operator<< (std::ostream & os, const OutQueue & oq)
{
    os << std::boolalpha;
    
    if(oq.is_ccl())
        os << "setting up CCL message from: " << oq.name() << "\n";
    else
        os << "mapping send variable id: " << oq.var_id() << " to " << oq.name() << "\n";

    os << "\t configuration parameters:" << "\n" 
       << "\t is ccl: " << oq.is_ccl() << "\n"
       << "\t on demand encoding: " << oq.on_demand() << "\n"
       << "\t ack: " << oq.ack() << "\n"
       << "\t blackout time: " << oq.blackout_time() << "\n"
       << "\t max queue: " << oq.max_queue() << "\n"
       << "\t newest first: " << oq.newest_first() << "\n"
       << "\t priority: " << oq.priority() << "\n"
       << "\t priority time constant: " << oq.priority_T() << "\n"
       << "\t messages already contain MOOS CCL header (0x20) and message ID: " << oq.is_dccl();

    return os;
}

