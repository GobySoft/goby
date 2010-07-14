// copyright 2009 t. schneider tes@mit.edu 
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

#include "queue_xml_callbacks.h"

using namespace xercesc;
using namespace goby::xml;

// this is called when the parser encounters the start tag, e.g. <message>
void goby::queue::QueueContentHandler::startElement( 
    const XMLCh *const uri,        // namespace URI
    const XMLCh *const localname,  // tagname w/ out NS prefix
    const XMLCh *const qname,      // tagname + NS pefix
    const Attributes &attrs )      // elements's attributes
{
    //const XMLCh* val;
    Tag current_tag = tags_map_[toNative(localname)];
    parents_.insert(current_tag);
    
    switch(current_tag)
    {
        default:
            break;            
            
        case tag_message:
            q_.push_back(QueueConfig());
            q_.back().set_type(queue_dccl);
            break;
    }
    
    current_text.clear();
}

void goby::queue::QueueContentHandler::endElement(          
    const XMLCh *const uri,        // namespace URI
    const XMLCh *const localname,  // tagname w/ out NS prefix
    const XMLCh *const qname )     // tagname + NS pefix
{        
    std::string trimmed_data = boost::trim_copy(toNative(current_text));

    Tag current_tag = tags_map_[toNative(localname)];
    parents_.erase(current_tag);
    
    switch(current_tag)
    {            
        default:
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
        case tag_priority_time_const:
            // deprecated
            break;

        case tag_name:
            if(!in_message_var() && !in_header_var())
                q_.back().set_name(trimmed_data);
            break;
            

        case tag_ttl:
            q_.back().set_ttl(trimmed_data);            
            break;


        case tag_value_base:
            q_.back().set_value_base(trimmed_data);
            break;

    }
}
