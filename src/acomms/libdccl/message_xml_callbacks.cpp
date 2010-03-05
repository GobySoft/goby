// copyright 2008, 2009 t. schneider tes@mit.edu
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

#include "message_xml_callbacks.h"

using namespace xercesc;

    
// this is called when the parser encounters the start tag, e.g. <message>
void dccl::MessageContentHandler::startElement( 
    const XMLCh *const uri,        // namespace URI
    const XMLCh *const localname,  // tagname w/ out NS prefix
    const XMLCh *const qname,      // tagname + NS pefix
    const Attributes &attrs )      // elements's attributes
{
    const XMLCh* val;

    // tag parameters
    static XercesString algorithm = fromNative("algorithm");
    static XercesString mandatory_content = fromNative("mandatory_content");
    static XercesString key = fromNative("key");
    static XercesString stype = fromNative("type");
    
    switch(tags_map_[toNative(localname)])
    {
        default:
            break;

        case tag_message:
            // start new message
            in_message = true;
            messages.push_back(Message());
            break;

        case tag_layout:
            in_layout = true;
            break;

        case tag_publish:
            in_publish = true;
            messages.back().add_publish();
            break;

        case tag_hex:
        case tag_int:
        case tag_string:
        case tag_float:
        case tag_bool:
        case tag_static:
        case tag_enum:
            in_message_var = true;
            messages.back().add_message_var(toNative(localname));
            type = localname;
            if((val = attrs.getValue(algorithm.c_str())) != 0)
                messages.back().last_message_var().set_algorithms(tes_util::explode(toNative(val),',',true));
            break;

        case tag_delta_encode:
            messages.back().set_delta_encode(true);
            break;
            
        case tag_trigger_var:
            if((val = attrs.getValue(mandatory_content.c_str())) != 0)
                messages.back().set_trigger_mandatory(toNative(val));
            break;

        case tag_all:
            messages.back().last_publish().set_use_all_names(true);
            break;

        case tag_destination_var:
            if((val = attrs.getValue(key.c_str())) != 0)
                messages.back().set_dest_key(toNative(val));
            break;

        case tag_moos_var:
            if(in_message_var && !in_publish && ((val = attrs.getValue(key.c_str())) != 0))
                messages.back().last_message_var().set_source_key(toNative(val));
            else if(in_publish && ((val = attrs.getValue(stype.c_str())) != 0))
            {
                if(toNative(val) == "double")
                    messages.back().last_publish().set_type(cpp_double);
                else if(toNative(val) == "string")
                    messages.back().last_publish().set_type(cpp_string);
                else if(toNative(val) == "long")
                    messages.back().last_publish().set_type(cpp_long);
                else if(toNative(val) == "bool")
                    messages.back().last_publish().set_type(cpp_bool);
            }
            break;

        case tag_message_var:
            if(in_publish)
            {
                if((val = attrs.getValue(algorithm.c_str())) != 0)
                    messages.back().last_publish().add_algorithms(tes_util::explode(toNative(val),',',true));
                else
                    messages.back().last_publish().add_algorithms(std::vector<std::string>());
            }
            break;
    }

    current_text.clear(); // starting a new tag, clear any old CDATA
}

void dccl::MessageContentHandler::endElement(          
    const XMLCh *const uri,        // namespace URI
    const XMLCh *const localname,  // tagname w/ out NS prefix
    const XMLCh *const qname )     // tagname + NS pefix
{        
    std::string trimmed_data = boost::trim_copy(toNative(current_text));

    switch(tags_map_[toNative(localname)])
    {
        case tag_message:
            messages.back().preprocess(); // do any message tasks that require all data
            in_message = false;
            break;
            
        case tag_layout:
            in_layout = false;
            break;
                
        case tag_publish:
            in_publish = false;
            break;                

        case tag_destination_var:
            messages.back().set_dest_var(trimmed_data);
            break;

        case tag_hex:
        case tag_int:
        case tag_bool:
        case tag_enum:
        case tag_float:
        case tag_static:
        case tag_string:
            in_message_var = false;        
            type.clear();
            break;

        case tag_expected_delta:
            messages.back().last_message_var().set_expected_delta(trimmed_data);
            break;
            
        case tag_format:
            messages.back().last_publish().set_format(trimmed_data);
            break;
            
        case tag_id:
            messages.back().set_id(trimmed_data);
            break;
            
        case tag_incoming_hex_moos_var:
            messages.back().set_in_var(trimmed_data);
            break;
                
        case tag_max_length:
            messages.back().last_message_var().set_max_length(trimmed_data);
            break;
            

            
        case tag_max:
            messages.back().last_message_var().set_max(trimmed_data);
            break;
            
        case tag_message_var:
            messages.back().last_publish().add_name(trimmed_data);  
            break;            

        case tag_min:
            messages.back().last_message_var().set_min(trimmed_data);
            break;
            
        case tag_moos_var:
            if(in_message_var)
                messages.back().last_message_var().set_source_var(trimmed_data);
            else if(in_publish)
                messages.back().last_publish().set_var(trimmed_data);
            break;

        case tag_src_var:
            messages.back().last_message_var().set_source_var(trimmed_data);
            break;

        case tag_publish_var:
            messages.back().last_publish().set_var(trimmed_data);
            break;
            
        case tag_name:
            if(in_message && !in_message_var && !in_publish)
                messages.back().set_name(trimmed_data);
            else if(in_message_var && !in_publish)
                messages.back().last_message_var().set_name(trimmed_data);
            break;
            
            
        case tag_outgoing_hex_moos_var:
            messages.back().set_out_var(trimmed_data);
            break;
            
        case tag_precision:
            messages.back().last_message_var().set_precision(trimmed_data);
            break;
            
            
        case tag_size:
            messages.back().set_size(trimmed_data);
            break;

        case tag_trigger_var:
            messages.back().set_trigger_var(trimmed_data);        
            break;
            
        case tag_trigger_time:
            messages.back().set_trigger_time(trimmed_data);
            break;
        
        case tag_trigger:
            messages.back().set_trigger(trimmed_data);
            break;
            
        case tag_value:
            if(tags_map_[toNative(type)] == tag_static)
                messages.back().last_message_var().set_static_val(trimmed_data);
            else if(tags_map_[toNative(type)] == tag_enum)
                messages.back().last_message_var().add_enum(trimmed_data);
            break;

        default:
            break;
            
    }
}
