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

#include "modem_message.h"
#include "flex_cout.h"
#include "queue_config.h"

typedef std::list<micromodem::Message>::iterator messages_it;
typedef std::multimap<unsigned, messages_it>::iterator waiting_for_ack_it;

namespace queue
{    
    class Queue
    {
      public:
        Queue(const QueueConfig cfg = QueueConfig(),
              FlexCout* tout = 0,
              const unsigned& modem_id = 0);

        bool push_message(micromodem::Message& new_message);
        micromodem::Message give_data(unsigned frame);
        bool pop_message(unsigned frame);    
        bool pop_message_ack(unsigned frame, micromodem::Message& msg);
        void pop_tout(const std::string& snip);
        
        
            
        bool priority_values(double* priority, double* last_send_time, micromodem::Message* message, std::string* error);
        unsigned give_dest();

        void clear_ack_queue() { waiting_for_ack_.clear(); }
        void flush();
        
        size_t size() { return messages_.size(); }
    
        bool on_demand() const        { return on_demand_; }
        double last_send_time() const { return last_send_time_; }

        void set_on_demand(bool b)       { on_demand_ = b; }
        void set_on_demand(const std::string& s)     { set_on_demand(tes_util::string2bool(s)); }

        const QueueConfig cfg() const { return cfg_; }
        
        std::string summary() const;
        
      private:
        waiting_for_ack_it find_ack_value(messages_it it_to_find);
        messages_it next_message_it();    
    
      private:
        const QueueConfig cfg_;
        
        bool on_demand_;
    
        double last_send_time_;
    
        const unsigned& modem_id_;
    
        FlexCout* tout_;
    
        std::list<micromodem::Message> messages_;

        // map frame number onto messages list iterator
        // can have multiples in the same frame now
        std::multimap<unsigned, messages_it> waiting_for_ack_;
    
    };
    std::ostream & operator<< (std::ostream & os, const Queue & oq);
}


#endif
