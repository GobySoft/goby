// t. schneider tes@mit.edu 10.18.09
// ocean engineering graduate student - mit / whoi joint program
// massachusetts institute of technology (mit)
// laboratory for autonomous marine sensing systems (lamss)
//
// this is message_xml_tags.h, part of libdccl 
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

#ifndef MessageXMLTagsH
#define MessageXMLTagsH

#include "xerces_strings.h"

#include <boost/assign.hpp>

namespace dccl
{    
    enum Tags { tag_not_defined,
                tag_ack,
                tag_all,
                tag_blackout_time,
                tag_bool,
                tag_destination_moos_var,
                tag_enum,
                tag_float,
                tag_format,
                tag_id,
                tag_incoming_hex_moos_var,
                tag_int,
                tag_key,
                tag_layout,
                tag_max_length,
                tag_max_queue,
                tag_max,
                tag_message_var,
                tag_message,
                tag_min,
                tag_moos_var,
                tag_name,
                tag_newest_first,
                tag_outgoing_hex_moos_var,
                tag_precision, 
                tag_priority_base,
                tag_priority_time_const,
                tag_publish,
                tag_size,
                tag_source,
                tag_static,
                tag_string,
                tag_trigger_moos_var,
                tag_trigger_time,
                tag_trigger,
                tag_value
    };

    
    static void initialize_tags(std::map<std::string, Tags>& tags_map)
    {
        boost::assign::insert(tags_map)
            ("ack",tag_ack)
            ("all",tag_all)
            ("blackout_time",tag_blackout_time)
            ("bool",tag_bool)
            ("destination_moos_var",tag_destination_moos_var)
            ("enum",tag_enum)
            ("float",tag_float)
            ("format",tag_format)
            ("id",tag_id)
            ("incoming_hex_moos_var",tag_incoming_hex_moos_var)
            ("int",tag_int)
            ("key",tag_key)
            ("layout",tag_layout)
            ("max_length",tag_max_length)
            ("max_queue",tag_max_queue)
            ("max",tag_max)
            ("message_var",tag_message_var)
            ("message",tag_message)
            ("min",tag_min)
            ("moos_var",tag_moos_var)
            ("name",tag_name)
            ("newest_first",tag_newest_first)
            ("outgoing_hex_moos_var",tag_outgoing_hex_moos_var)
            ("precision", tag_precision) 
            ("priority_base",tag_priority_base)
            ("priority_time_const",tag_priority_time_const)
            ("publish",tag_publish)
            ("size",tag_size)
            ("source",tag_source)
            ("static",tag_static)
            ("string",tag_string)
            ("trigger_moos_var",tag_trigger_moos_var)
            ("trigger_time",tag_trigger_time)
            ("trigger",tag_trigger)
            ("value",tag_value);    
    }
}


#endif
