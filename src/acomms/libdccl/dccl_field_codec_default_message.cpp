// copyright 2011 t. schneider tes@mit.edu
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

#include "dccl_field_codec_default_message.h"
#include "goby/protobuf/dynamic_protobuf_manager.h"

using goby::glog;

//
// DCCLDefaultMessageCodec
//

goby::acomms::Bitset goby::acomms::DCCLDefaultMessageCodec::any_encode(const boost::any& wire_value)
{
  if(wire_value.empty())
      return Bitset(min_size());
  else
      return traverse_const_message<Encoder, Bitset>(wire_value);
}
  

 
unsigned goby::acomms::DCCLDefaultMessageCodec::any_size(const boost::any& field_value)
{
    if(field_value.empty())
        return min_size();
    else
        return traverse_const_message<Size, unsigned>(field_value);
}

void goby::acomms::DCCLDefaultMessageCodec::any_run_hooks(const boost::any& field_value)
{
    if(!field_value.empty())
        traverse_const_message<RunHooks, bool>(field_value);
}


boost::any goby::acomms::DCCLDefaultMessageCodec::any_decode(Bitset* bits)
{
    boost::shared_ptr<google::protobuf::Message> msg =
        goby::protobuf::DynamicProtobufManager::new_protobuf_message(DCCLFieldCodecBase::this_descriptor());
    
   
    //glog.is(debug1) && glog  << debug << "Looking to decode Message " << msg->GetDescriptor()->full_name() << std::endl;


    const google::protobuf::Descriptor* desc = msg->GetDescriptor();

    for(int i = 0, n = desc->field_count(); i < n; ++i)
    {
//        
//            glog.is(debug1) && glog  << debug << "Looking to decode " << desc->field(i)->DebugString();
        
        const google::protobuf::FieldDescriptor* field_desc = desc->field(i);

        if(!check_field(field_desc))
            continue;

        std::string field_codec = find_codec(field_desc);
        
        boost::shared_ptr<DCCLFieldCodecBase> codec =
            DCCLFieldCodecManager::find(field_desc, field_codec);
        boost::shared_ptr<FromProtoCppTypeBase> helper =
            DCCLTypeHelper::find(field_desc);

        if(field_desc->is_repeated())
        {   
            std::vector<boost::any> wire_values;
            codec->base_decode_repeated(bits, &wire_values, field_desc);
            for(int j = 0, m = wire_values.size(); j < m; ++j)
                helper->add_value(field_desc, msg.get(), wire_values[j]);
        }
        else
        {
            boost::any wire_value;
            codec->base_decode(bits, &wire_value, field_desc);
            helper->set_value(field_desc, msg.get(), wire_value);
        } 
    }

     std::vector< const google::protobuf::FieldDescriptor* > set_fields;
     msg->GetReflection()->ListFields(*msg, &set_fields);
     return set_fields.empty() ? boost::any() : msg;    
}


unsigned goby::acomms::DCCLDefaultMessageCodec::max_size()
{
    unsigned u = 0;
    traverse_descriptor<MaxSize>(&u);
    return u;
}

unsigned goby::acomms::DCCLDefaultMessageCodec::min_size()
{
    unsigned u = 0;
    traverse_descriptor<MinSize>(&u);
    return u;
}


void goby::acomms::DCCLDefaultMessageCodec::validate()
{
    bool b = false;
    traverse_descriptor<Validate>(&b);
}

std::string goby::acomms::DCCLDefaultMessageCodec::info()
{
    std::stringstream ss;
    traverse_descriptor<Info>(&ss);
    return ss.str();
}

bool goby::acomms::DCCLDefaultMessageCodec::check_field(const google::protobuf::FieldDescriptor* field)
{
    if(!field)
        return true;
    else
    {
        DCCLFieldOptions dccl_field_options = field->options().GetExtension(goby::field).dccl();
        if(dccl_field_options.omit() // omit
            || (field->cpp_type() != google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE // for non message fields, skip if header / body mismatch
                && ((part() == HEAD && !dccl_field_options.in_head())
                    || (part() == BODY && dccl_field_options.in_head()))))
            return false;
        else
            return true;
    }
    
}

std::string goby::acomms::DCCLDefaultMessageCodec::find_codec(const google::protobuf::FieldDescriptor* field)
{
    DCCLFieldOptions dccl_field_options = field->options().GetExtension(goby::field).dccl();
    
    if(dccl_field_options.has_codec())
        return dccl_field_options.codec();
    else if(field->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE)
        return field->message_type()->options().GetExtension(goby::msg).dccl().codec();
    else
        return dccl_field_options.codec();
}


