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

// xml code based initially on work in C++ Cookbook by D. Ryan Stephens, Christopher Diggins, Jonathan Turkanis, and Jeff Cogswell. Copyright 2006 O'Reilly Media, INc., 0-596-00761-2. 

#include "queue_xml_callbacks.h"

using namespace xercesc;
    
// this is called when the parser encounters the start tag, e.g. <message>
void queue::QueueContentHandler::startElement( 
    const XMLCh *const uri,        // namespace URI
    const XMLCh *const localname,  // tagname w/ out NS prefix
    const XMLCh *const qname,      // tagname + NS pefix
    const Attributes &attrs )      // elements's attributes
{
    //const XMLCh* val;
    
    switch(tags_map_[toNative(localname)])
    {
        default:
            break;

        case tag_int:
        case tag_string:
        case tag_float:
        case tag_bool:
        case tag_static:
        case tag_enum:
            in_message_var_ = true;
            break;
            
            
        case tag_message:
            // start new message
            in_message_ = true;
            q_.push_back(QueueConfig());
            q_.back().set_type(queue_dccl);
            break;


    }
    
    current_text.clear();
}

void queue::QueueContentHandler::endElement(          
    const XMLCh *const uri,        // namespace URI
    const XMLCh *const localname,  // tagname w/ out NS prefix
    const XMLCh *const qname )     // tagname + NS pefix
{        
    std::string trimmed_data = boost::trim_copy(toNative(current_text));

    switch(tags_map_[toNative(localname)])
    {
        case tag_bool:
        case tag_enum:
        case tag_float:
        case tag_int:
        case tag_static:
        case tag_string:
            in_message_var_ = false;        
            break;

        case tag_message:
            in_message_ = false;
            break;
            
        case tag_ack:
            q_.back().set_ack(trimmed_data);
            break;            

        case tag_blackout_time:
            q_.back().set_blackout_time(trimmed_data);
            break;

        case tag_max_queue:
            q_.back().set_max_queue(trimmed_data);
            break;
            
        case tag_newest_first:
            q_.back().set_newest_first(trimmed_data);
            break;        

        case tag_id:
            q_.back().set_id(trimmed_data);
            break;
            
        case tag_priority_base:
            q_.back().set_priority_base(trimmed_data);
            break;
            
        case tag_priority_time_const:
            q_.back().set_priority_time_const(trimmed_data);
            break;

        case tag_name:
            if(in_message_ && !in_message_var_)
                q_.back().set_name(trimmed_data);
            break;
            
        default:
            break;
            
    }
}
