// t. schneider tes@mit.edu 3.16.09
// ocean engineering graduate student - mit / whoi joint program
// massachusetts institute of technology (mit)
// laboratory for autonomous marine sensing systems (lamss)
// 
// this is message_queuing.h, part of libdccl
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

#ifndef MESSAGE_QUEUING_H
#define MESSAGE_QUEUING_H

#include <ostream>

namespace dccl
{
    
    // defines parameters for queuing a message
    class Queuing
    {
      public:
        Queuing(): ack_(false),
            blackout_time_(0),
            max_queue_(0),
            newest_first_(false),
            priority_base_(1),
            priority_time_const_(120) { }

        void set_ack(bool ack) {ack_=ack;}
        void set_blackout_time(int blackout_time) {blackout_time_ = blackout_time;}
        void set_max_queue(int max_queue) {max_queue_ = max_queue;}
        void set_newest_first(bool newest_first) {newest_first_ = newest_first;}
        void set_priority_base(double priority_base) {priority_base_ = priority_base;}
        void set_priority_time_const(double priority_time_const) {priority_time_const_=priority_time_const;}

        bool ack() const {return ack_;}
        int blackout_time() const {return blackout_time_;} 
        int max_queue() const {return max_queue_;} 
        bool newest_first() const {return newest_first_;}
        double priority_base() const {return priority_base_;} 
        double priority_time_const() const {return priority_time_const_;}
        
        
        private:
        
        bool ack_;
        int blackout_time_;
        int max_queue_;
        bool newest_first_;
        double priority_base_;
        double priority_time_const_;
        
    };

    std::ostream& operator<< (std::ostream& out, const Queuing& q);
    
}

#endif
