// copyright 2009 t. schneider tes@mit.edu
// 
// this file is part of the Dynamic Compact Control Language (DCCL),
// the goby-acomms codec. goby-acomms is a collection of libraries 
// for acoustic underwater networking
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

#ifndef MESSAGE_QUEUING20091211H
#define MESSAGE_QUEUING20091211H

#include <ostream>

#include <boost/lexical_cast.hpp>

#include "goby/util/string.h"
namespace goby
{
namespace queue
{
    /// specifies the type of the Queue
    enum QueueType 
    {
        queue_notype, /*!< no type (queue is useless)   */
        queue_dccl, /*!< queue holds DCCL messages   */
        queue_ccl, /*!< queue holds CCL messages   */
        queue_data /*!< queue holds miscellaneous data  */
    };

    /// human readable output of the QueueType
    inline std::ostream& operator<< (std::ostream& out, const QueueType& qt)
    {
        switch(qt)
        {
            default:
            case queue_notype: return out << "notype";
            case queue_dccl:   return out << "dccl";
            case queue_ccl:    return out << "ccl";
            case queue_data:   return out << "data";
        }
    }

    
    /// defines parameters for configuring a message %queue
    class QueueConfig
    {
      public:
      QueueConfig()
          : ack_(true),
            blackout_time_(0),
            max_queue_(0),
            newest_first_(true),
            value_base_(1),
            ttl_(1800),
            type_(queue_notype),
            id_(0),
            name_("")
            { }

        /// sets whether the %queue should require an acknowledgement of all data
        void set_ack(bool ack)
        { ack_=ack; }
        /// sets how long (in seconds) between sending the last message from this %queue should this %queue be ignored (considered "in blackout")
        void set_blackout_time(unsigned blackout_time)
        { blackout_time_ = blackout_time; }
        /// sets how many messages fit in this %queue before discarding the oldest (newest_first = true) or the newest (newest_first = false)
        void set_max_queue(unsigned max_queue)
        { max_queue_ = max_queue; }
        /// sets whether the newest messages are sent first (FILO %queue) or not (FIFO %queue)
        void set_newest_first(bool newest_first)
        { newest_first_ = newest_first; }
        /// sets the base value
        void set_value_base(double value_base)
        { value_base_ = value_base; }
        /// sets the time to live
        void set_ttl(unsigned ttl)
        { ttl_=ttl; }
        /// sets the type of the %queue
        void set_type(QueueType t)
        { type_ = t; }
        /// sets the id of the %queue. This id is the DCCL id (<id/>) for %queues of type queue_dccl. On the other hand, this id is the CCL id (first byte in decimal) for %queues of type queue_ccl. The type and id together form a unique key (a QueueKey).
        void set_id(unsigned id)
        { id_ = id; }
        /// sets the name of the %queue. This is used informational and can be omitted if desired.
        void set_name(const std::string& name)
        { name_ = name; }
        
        void set_ack(const std::string& s)
        { set_ack(str::string2bool(s)); }
        void set_blackout_time(const std::string& s)
        { set_blackout_time(boost::lexical_cast<unsigned>(s)); }
        void set_max_queue(const std::string& s)
        { set_max_queue(boost::lexical_cast<unsigned>(s)); }
        void set_newest_first(const std::string& s)
        { set_newest_first(str::string2bool(s)); }
        void set_value_base(const std::string& s)
        { set_value_base(boost::lexical_cast<double>(s)); }            
        void set_ttl(const std::string& s)
        { set_ttl(boost::lexical_cast<unsigned>(s)); }
        void set_type(const std::string& s)
        {
            if(str::stricmp("queue_dccl", s))
                set_type(queue_dccl);
            else if(str::stricmp("queue_ccl", s))
                set_type(queue_ccl);
            else if(str::stricmp("queue_data", s))
                set_type(queue_data);
            else
                set_type(queue_notype);
        }        
        void set_id(const std::string& s)
        { set_id(boost::lexical_cast<unsigned>(s)); }
        
        /// \return current acknowledgement setting    
        bool ack() const {return ack_;}
        /// \return current blackout time settings (seconds)
        unsigned blackout_time() const {return blackout_time_;} 
        /// \return current %queue maximum setting (# of messages)
        unsigned max_queue() const {return max_queue_;} 
        /// \return whether new messages are sent first (true) or not (false)
        bool newest_first() const {return newest_first_;}
        /// \return the type of the %queue.
        QueueType type() const { return type_; }
        /// \return the id of the %queue.
        unsigned id() const { return id_; }
        /// \return the name of the %queue.
        std::string name() const { return name_; }
        /// \return the base value of messages in this queue
        double value_base() const {return value_base_;} 
        /// \return the time to live of messages
        unsigned ttl() const {return ttl_;}
        
        private:
        
        bool ack_;
        unsigned blackout_time_;
        unsigned max_queue_;
        bool newest_first_;
        double value_base_;
        unsigned ttl_;

        QueueType type_;

        // meaning of id_ depends on type_
        // - queue_dccl: DCCL ID (<id/>)
        // - queue_ccl: CCL ID
        // - queue_data: data ID (0-63 for data CCL)
        unsigned id_;
        
        std::string name_;
    };

    std::ostream& operator<< (std::ostream& out, const QueueConfig& q);
    
}
}
#endif
