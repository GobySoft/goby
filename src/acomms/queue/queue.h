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

#include <boost/algorithm/string.hpp>
#include <boost/any.hpp>

#include "goby/util/time.h"
#include "goby/util/as.h"

#include "goby/acomms/protobuf/queue.pb.h"
#include "goby/common/acomms_option_extensions.pb.h"
#include "goby/acomms/acomms_helpers.h"

namespace goby
{
    namespace acomms
    {
        class QueueManager;        

        struct QueuedMessage
        {
            boost::shared_ptr<google::protobuf::Message> dccl_msg;
            protobuf::QueuedMessageMeta meta;
        };
        

        typedef std::list<QueuedMessage>::iterator messages_it;
        typedef std::multimap<unsigned, messages_it>::iterator waiting_for_ack_it;

        
        class Queue
        {
          public:
            Queue(const google::protobuf::Descriptor* desc = 0, QueueManager* parent = 0);

            bool push_message(const protobuf::QueuedMessageMeta& encoded_msg,
                              boost::shared_ptr<google::protobuf::Message> dccl_msg);

            goby::acomms::QueuedMessage give_data(unsigned frame);
            bool pop_message(unsigned frame);
            bool pop_message_ack(unsigned frame, boost::shared_ptr<google::protobuf::Message>& removed_msg);
            void stream_for_pop(const google::protobuf::Message& dccl_msg);
            
            std::vector<boost::shared_ptr<google::protobuf::Message> > expire();
            
          
            bool get_priority_values(double* priority,
                                     boost::posix_time::ptime* last_send_time,
                                     const protobuf::ModemTransmission& request_msg,
                                     const std::string& data);
        
            void clear_ack_queue()
            { waiting_for_ack_.clear(); }

            void flush();
        
            size_t size() const 
            { return messages_.size(); }
    

            boost::posix_time::ptime last_send_time() const
            { return last_send_time_; }

            boost::posix_time::ptime newest_msg_time() const
            {
                return size()
                    ? goby::util::as<boost::posix_time::ptime>(
                        messages_.back().meta.time())
                    : boost::posix_time::ptime();
            }
            
            void info(std::ostream* os) const;

            std::string name() const
            {
                return desc_->full_name();
            }
            
            
            QueueMessageOptions queue_message_options()
            {
                return desc_->options().GetExtension(goby::msg).queue();
            }
            
            const google::protobuf::Descriptor* descriptor() const {return desc_;}

            
          private:
            waiting_for_ack_it find_ack_value(messages_it it_to_find);
            messages_it next_message_it();    


          private:
            const google::protobuf::Descriptor* desc_;
            QueueManager* parent_;
            
            boost::posix_time::ptime last_send_time_;    

            
            std::list<QueuedMessage> messages_;
            
            // map frame number onto messages list iterator
            // can have multiples in the same frame now
            std::multimap<unsigned, messages_it> waiting_for_ack_;
    
        };
        std::ostream & operator<< (std::ostream & os, const Queue & oq);
    }

}
#endif
