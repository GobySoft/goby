// t. schneider tes@mit.edu 3.16.09
// ocean engineering graduate student - mit / whoi joint program
// massachusetts institute of technology (mit)
// laboratory for autonomous marine sensing systems (lamss)
// 
// this is message_queuing.cpp, part of libdccl
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

#include "message_queuing.h"

std::ostream& dccl::operator<< (std::ostream& out, const Queuing& q)
{
    out << std::boolalpha << "ack: " << q.ack() << " | "
        << "blackout_time: " << q.blackout_time() << " | "
        << "max_queue: " << q.max_queue() << " | "
        << "newest_first: " << q.newest_first() << " | "
        << "priority_base: " << q.priority_base() << " | "
        << "priority_time_const: " << q.priority_time_const();
    
    return out;
}

