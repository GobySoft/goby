// copyright 2009 t. schneider tes@mit.edu
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

#ifndef QueueXMLTagsH
#define QueueXMLTagsH

#include <boost/assign.hpp>

#include "acomms/xml/xerces_strings.h"

namespace queue
{    
    enum Tags { tag_not_defined,
                tag_id,
                tag_name,
                tag_message,
                tag_bool,
                tag_enum,
                tag_float,
                tag_int,
                tag_static,
                tag_string,
                tag_ack,
                tag_blackout_time,
                tag_max_queue,
                tag_newest_first,
                tag_priority_base,
                tag_priority_time_const
    };

    
    static void initialize_tags(std::map<std::string, Tags>& tags_map)
    {
        boost::assign::insert(tags_map)
            ("id",tag_id)
            ("name",tag_name)
            ("message",tag_message)
            ("bool",tag_bool)
            ("enum",tag_enum)
            ("float",tag_float)
            ("int",tag_int)
            ("static",tag_static)
            ("string",tag_string)
            ("ack",tag_ack)
            ("blackout_time",tag_blackout_time)
            ("max_queue",tag_max_queue)
            ("newest_first",tag_newest_first)
            ("priority_base",tag_priority_base)
            ("priority_time_const",tag_priority_time_const);
    }
}


#endif
