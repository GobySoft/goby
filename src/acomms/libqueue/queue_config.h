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

#include "util/tes_utils.h"

namespace queue
{
    enum QueueType 
    {
        queue_notype,
        queue_dccl,
        queue_ccl,
        queue_data
    };

    inline std::ostream& operator<< (std::ostream& out, const QueueType& qt)
    {
        switch(qt)
        {
            case queue_notype: return out << "notype";
            case queue_dccl:   return out << "dccl";
            case queue_ccl:    return out << "ccl";
            case queue_data:   return out << "data";
        }
    }

    
// defines parameters for queuing a message
    class QueueConfig
    {
      public:
      QueueConfig()
          : ack_(false),
            blackout_time_(0),
            max_queue_(0),
            newest_first_(false),
            priority_base_(1),
            priority_time_const_(120),
            type_(queue_notype),
            id_(0),
            name_("")
            { }

        void set_ack(bool ack)
        { ack_=ack; }
        void set_blackout_time(unsigned blackout_time)
        { blackout_time_ = blackout_time; }
        void set_max_queue(unsigned max_queue)
        { max_queue_ = max_queue; }
        void set_newest_first(bool newest_first)
        { newest_first_ = newest_first; }
        void set_priority_base(double priority_base)
        { priority_base_ = priority_base; }
        void set_priority_time_const(double priority_time_const)
        { priority_time_const_=priority_time_const; }
        void set_type(QueueType t)
        { type_ = t; }
        void set_id(unsigned id)
        { id_ = id; }
        void set_name(const std::string& name)
        { name_ = name; }

        void set_ack(const std::string& s)
        { set_ack(tes_util::string2bool(s)); }
        void set_blackout_time(const std::string& s)
        { set_blackout_time(boost::lexical_cast<unsigned>(s)); }
        void set_max_queue(const std::string& s)
        { set_max_queue(boost::lexical_cast<unsigned>(s)); }
        void set_newest_first(const std::string& s)
        { set_newest_first(tes_util::string2bool(s)); }
        void set_priority_base(const std::string& s)
        { set_priority_base(boost::lexical_cast<double>(s)); }            
        void set_priority_time_const(const std::string& s)
        { set_priority_time_const(boost::lexical_cast<double>(s)); }
        void set_type(const std::string& s)
        {
            if(tes_util::stricmp("queue_dccl", s))
                set_type(queue_dccl);
            else if(tes_util::stricmp("queue_ccl", s))
                set_type(queue_ccl);
            else if(tes_util::stricmp("queue_data", s))
                set_type(queue_data);
            else
                set_type(queue_notype);
        }
        
        void set_id(const std::string& s)
        { set_id(boost::lexical_cast<unsigned>(s)); }

            
        bool ack() const {return ack_;}
        unsigned blackout_time() const {return blackout_time_;} 
        unsigned max_queue() const {return max_queue_;} 
        bool newest_first() const {return newest_first_;}
        double priority_base() const {return priority_base_;} 
        double priority_time_const() const {return priority_time_const_;}
        QueueType type() const { return type_; }
        unsigned id() const { return id_; }
        std::string name() const { return name_; }
        
        
        private:
        
        bool ack_;
        unsigned blackout_time_;
        unsigned max_queue_;
        bool newest_first_;
        double priority_base_;
        double priority_time_const_;

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

#endif
