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


#ifndef Queue20080605H
#define Queue20080605H

#include <iostream>
#include <vector>
#include <deque>
#include <sstream>
#include <bitset>
#include <list>
#include <string>
#include <map>
#include <ctime>

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

#include "acomms/modem_message.h"

#include "queue_config.h"

typedef std::list<modem::Message>::iterator messages_it;
typedef std::multimap<unsigned, messages_it>::iterator waiting_for_ack_it;

namespace queue
{    
    class Queue
    {
      public:
        Queue(const QueueConfig cfg = QueueConfig(),
              std::ostream* os = 0,
              const unsigned& modem_id = 0);

        bool push_message(modem::Message& new_message);
        modem::Message give_data(unsigned frame);
        bool pop_message(unsigned frame);    
        bool pop_message_ack(unsigned frame, modem::Message& msg);
        void stream_for_pop(const std::string& snip);

        std::vector<modem::Message> expire();
        
        bool priority_values(double& priority,
                             double& last_send_time,
                             modem::Message& message);
        
        unsigned give_dest();

        void clear_ack_queue()
        { waiting_for_ack_.clear(); }

        void flush();
        
        size_t size() const 
        { return messages_.size(); }
    
        bool on_demand() const
        { return on_demand_; }

        double last_send_time() const
        { return last_send_time_; }

        double newest_msg_time() const
        { return size() ? messages_.back().t() : acomms_util::NOT_A_TIME; }
        
        void set_on_demand(bool b)
        { on_demand_ = b; }

        void set_on_demand(const std::string& s)
        { set_on_demand(tes_util::string2bool(s)); }

        const QueueConfig cfg() const
        { return cfg_; }
        
        std::string summary() const;
        
      private:
        waiting_for_ack_it find_ack_value(messages_it it_to_find);
        messages_it next_message_it();    
    
      private:
        const QueueConfig cfg_;
        
        bool on_demand_;
    
        double last_send_time_;
    
        const unsigned& modem_id_;
    
        std::ostream* os_;
    
        std::list<modem::Message> messages_;

        // map frame number onto messages list iterator
        // can have multiples in the same frame now
        std::multimap<unsigned, messages_it> waiting_for_ack_;
    
    };
    std::ostream & operator<< (std::ostream & os, const Queue & oq);
}


#endif
