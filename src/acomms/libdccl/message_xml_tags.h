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

#ifndef MessageXMLTags20091211H
#define MessageXMLTags20091211H

#include "acomms/xml/xerces_strings.h"

#include <boost/assign.hpp>

namespace dccl
{    
    enum Tags { tag_not_defined,
                tag_all,
                tag_bool,
                tag_delta_encode,
                tag_destination_var,
                tag_enum,
                tag_expected_delta,
                tag_float,
                tag_format,
                tag_hex,
                tag_id,
                tag_incoming_hex_moos_var,
                tag_int,
                tag_key,
                tag_layout,
                tag_max_length,
                tag_max,
                tag_message_var,
                tag_message,
                tag_min,
                tag_moos_var,
                tag_name,
                tag_outgoing_hex_moos_var,
                tag_precision, 
                tag_publish,
                tag_publish_var,
                tag_size,
                tag_source,
                tag_src_var,
                tag_static,
                tag_string,
                tag_trigger_var,
                tag_trigger_time,
                tag_trigger,
                tag_value
    };

    
    static void initialize_tags(std::map<std::string, Tags>& tags_map)
    {
        boost::assign::insert(tags_map)
            ("all",tag_all)
            ("bool",tag_bool)
            ("delta_encode", tag_delta_encode)
            ("destination_moos_var",tag_destination_var)
            ("destination_var",tag_destination_var)
            ("enum",tag_enum)
            ("expected_delta",tag_expected_delta)
            ("float",tag_float)
            ("format",tag_format)
            ("hex", tag_hex)
            ("id",tag_id)
            ("incoming_hex_moos_var",tag_incoming_hex_moos_var)
            ("int",tag_int)
            ("key",tag_key)
            ("layout",tag_layout)
            ("num_bytes",tag_max_length)
            ("max_length",tag_max_length)
            ("max",tag_max)
            ("message_var",tag_message_var)
            ("message",tag_message)
            ("min",tag_min)
            ("moos_var",tag_moos_var)
            ("name",tag_name)
            ("outgoing_hex_moos_var",tag_outgoing_hex_moos_var)
            ("precision", tag_precision) 
            ("publish",tag_publish)
            ("publish_var",tag_publish_var)
            ("size",tag_size)
            ("source",tag_source)
            ("src_var",tag_src_var)
            ("static",tag_static)
            ("string",tag_string)
            ("trigger_moos_var",tag_trigger_var)
            ("trigger_var",tag_trigger_var)
            ("trigger_time",tag_trigger_time)
            ("trigger",tag_trigger)
            ("value",tag_value);    
    }
}


#endif
