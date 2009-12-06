// copyright 2009 t. schneider tes@mit.edu
// ocean engineering graudate student - mit / whoi joint program
// massachusetts institute of technology (mit)
// laboratory for autonomous marine sensing systems (lamss)
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

#include "queue_config.h"

std::ostream& queue::operator<< (std::ostream& out, const QueueConfig& q)
{
    out << "queue configuration:\n"
        << "\t" << "name: " << q.name() << "\n"
        << "\t" << "id: " << q.id() << "\n"
        << "\t" << "type: " << q.type() << "\n"
        << std::boolalpha << "\t" << "ack: " << q.ack() << "\n"
        << "\t" << "blackout_time: " << q.blackout_time() << "\n"
        << "\t" << "max_queue: " << q.max_queue() << "\n"
        << "\t" << "newest_first: " << q.newest_first() << "\n"
        << "\t" << "priority_base: " << q.priority_base() << "\n"
        << "\t" << "priority_time_const: " << q.priority_time_const();
    
    return out;
}

