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

//
// DCCLDefaultMessageCodec
//

            
goby::acomms::Bitset goby::acomms::DCCLDefaultMessageCodec::any_encode(const boost::any& wire_value)
{
    if(wire_value.empty())
        return Bitset(min_size());
    else
    {
        try
        {
            Bitset return_value;
       
            const google::protobuf::Message* msg = boost::any_cast<const google::protobuf::Message*>(wire_value);
       
            // 
            //     DCCLCommon::logger() << debug << "Looking to encode Message " << msg->GetDescriptor()->full_name() << std::endl;

   
            const google::protobuf::Descriptor* desc = msg->GetDescriptor();
            const google::protobuf::Reflection* refl = msg->GetReflection();       
            for(int i = 0, n = desc->field_count(); i < n; ++i)
            {
                // 
                //     DCCLCommon::logger() << debug << "Looking to encode " << desc->field(i)->DebugString();
       
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
                    for(int j = 0, m = refl->FieldSize(*msg, field_desc); j < m; ++j)
                        wire_values.push_back(helper->get_repeated_value(field_desc, *msg, j));
                   
                    codec->base_encode_repeated(&return_value, wire_values, field_desc);
                }
                else
                {
                    codec->base_encode(&return_value, helper->get_value(field_desc, *msg), field_desc);
                }
            }
            return return_value;
        }
        catch(boost::bad_any_cast& e)
        {
            throw(DCCLException("Bad type given to encode, expecting const google::protobuf::Message*"));
        }
    }    
}

unsigned goby::acomms::DCCLDefaultMessageCodec::any_size(const boost::any& field_value)
{
    
    if(field_value.empty())
        return min_size();
    else
    {
        try
        {
            unsigned return_value = 0;
            
            
            const google::protobuf::Message* msg = boost::any_cast<const google::protobuf::Message*>(field_value);
       
            // 
            //     DCCLCommon::logger() << debug << "Looking to encode Message " << msg->GetDescriptor()->full_name() << std::endl;

   
            const google::protobuf::Descriptor* desc = msg->GetDescriptor();
            const google::protobuf::Reflection* refl = msg->GetReflection();       
            for(int i = 0, n = desc->field_count(); i < n; ++i)
            {
                // 
                //     DCCLCommon::logger() << debug << "Looking to encode " << desc->field(i)->DebugString();
       
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
                    for(int j = 0, m = refl->FieldSize(*msg, field_desc); j < m; ++j)
                        wire_values.push_back(helper->get_repeated_value(field_desc, *msg, j));
                    
                    codec->base_size_repeated(&return_value, wire_values, field_desc);
                }
                else
                {
                    codec->base_size(&return_value, helper->get_value(field_desc, *msg), field_desc);
                }
            }
            return return_value;
        }
        catch(boost::bad_any_cast& e)
        {
            throw(DCCLException("Bad type given to encode, expecting const google::protobuf::Message*"));
        }
    }
    
}

void goby::acomms::DCCLDefaultMessageCodec::any_run_hooks(const boost::any& field_value)
{
    if(!field_value.empty())
    {
        try
        {
            const google::protobuf::Message* msg = boost::any_cast<const google::protobuf::Message*>(field_value);
       
             
            DCCLCommon::logger() << debug1 << "Looking to run hooks on Message " << msg->GetDescriptor()->full_name() << std::endl;

   
            const google::protobuf::Descriptor* desc = msg->GetDescriptor();
//            const google::protobuf::Reflection* refl = msg->GetReflection();       
            for(int i = 0, n = desc->field_count(); i < n; ++i)
            {
                // 
                DCCLCommon::logger() << debug1 << "Looking to run hooks " << desc->field(i)->DebugString();
       
                const google::protobuf::FieldDescriptor* field_desc = desc->field(i);

                if(!check_field(field_desc))
                    continue;
                
                std::string field_codec = find_codec(field_desc);
           
                boost::shared_ptr<DCCLFieldCodecBase> codec =
                    DCCLFieldCodecManager::find(field_desc, field_codec);
                boost::shared_ptr<FromProtoCppTypeBase> helper =
                    DCCLTypeHelper::find(field_desc);

                if(!field_desc->is_repeated())
                    codec->base_run_hooks(helper->get_value(field_desc, *msg), field_desc);
            }
        }
        catch(boost::bad_any_cast& e)
        {
            throw(DCCLException("Bad type given to encode, expecting const google::protobuf::Message*"));
        }
    }
}


boost::any goby::acomms::DCCLDefaultMessageCodec::any_decode(Bitset* bits)
{
    boost::shared_ptr<google::protobuf::Message> msg(
        DCCLCommon::message_factory().GetPrototype(DCCLFieldCodecBase::this_descriptor())->New());
    
   
    //DCCLCommon::logger() << debug << "Looking to decode Message " << msg->GetDescriptor()->full_name() << std::endl;


    const google::protobuf::Descriptor* desc = msg->GetDescriptor();

    for(int i = 0, n = desc->field_count(); i < n; ++i)
    {
//        
//            DCCLCommon::logger() << debug << "Looking to decode " << desc->field(i)->DebugString();
        
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
    unsigned sum = 0;

//    
//        DCCLCommon::logger() << debug << "Looking for max size of " << this_descriptor()->full_name() << std::endl;
    
    const google::protobuf::Descriptor* desc =
        DCCLFieldCodecBase::this_descriptor();
    for(int i = 0, n = desc->field_count(); i < n; ++i)
    {
        
        // 
        //     DCCLCommon::logger() << debug << "Looking to get max size of field " << desc->field(i)->DebugString();

        
        const google::protobuf::FieldDescriptor* field_desc = desc->field(i);

        if(!check_field(field_desc))
            continue;

        std::string field_codec = find_codec(field_desc);

        sum += DCCLFieldCodecManager::find(field_desc,field_codec)->
            base_max_size(field_desc);
    }
    
    return sum;    
}

unsigned goby::acomms::DCCLDefaultMessageCodec::min_size()
{
    unsigned sum = 0;

    // 
    //     DCCLCommon::logger() << debug << "Looking for min size of " << this_descriptor()->full_name() << std::endl;
    
    const google::protobuf::Descriptor* desc =
        DCCLFieldCodecBase::this_descriptor();
    for(int i = 0, n = desc->field_count(); i < n; ++i)
    {
        
        // 
        //     DCCLCommon::logger() << debug << "Looking to get min size of field " << desc->field(i)->DebugString();
        const google::protobuf::FieldDescriptor* field_desc = desc->field(i);

        if(!check_field(field_desc))
            continue;
        
        std::string field_codec = find_codec(field_desc);

        sum += DCCLFieldCodecManager::find(field_desc,field_codec)->
            base_min_size(field_desc);
    }
    
    return sum;
}

void goby::acomms::DCCLDefaultMessageCodec::validate()
{
    const google::protobuf::Descriptor* desc =
        DCCLFieldCodecBase::this_descriptor();
    for(int i = 0, n = desc->field_count(); i < n; ++i)
    {
        const google::protobuf::FieldDescriptor* field_desc = desc->field(i);

        if(!check_field(field_desc))
            continue;

        
        std::string field_codec = find_codec(field_desc);

        DCCLCommon::logger() << "field is " << field_desc->DebugString();
        DCCLCommon::logger() << "codec is " << field_codec << "\n" << std::endl;
        
        DCCLFieldCodecManager::find(field_desc,field_codec)->
            base_validate(field_desc);
    }    
}

std::string goby::acomms::DCCLDefaultMessageCodec::info()
{
    std::stringstream ss;
    
    const google::protobuf::Descriptor* desc =
        DCCLFieldCodecBase::this_descriptor();

    
    ss << desc->full_name() << " {\n";
    
    for(int i = 0, n = desc->field_count(); i < n; ++i)
    {
        const google::protobuf::FieldDescriptor* field_desc = desc->field(i);

        if(!check_field(field_desc))
            continue;

        std::string field_codec = find_codec(field_desc);

        DCCLFieldCodecManager::find(field_desc,field_codec)->
            base_info(field_desc, &ss);
    }
    ss << "}\n";
    return ss.str();
}

bool goby::acomms::DCCLDefaultMessageCodec::check_field(const google::protobuf::FieldDescriptor* field)
{
    if(!field)
        return true;
    else if(field->options().GetExtension(dccl::omit) // omit
            || (field->cpp_type() != google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE // for non message fields, skip if header / body mismatch
                && ((part() == HEAD && !field->options().GetExtension(dccl::in_head))
                    || (part() == BODY && field->options().GetExtension(dccl::in_head)))))
        return false;
    else
        return true;
}

std::string goby::acomms::DCCLDefaultMessageCodec::find_codec(const google::protobuf::FieldDescriptor* field)
{
    if(field->options().HasExtension(dccl::codec))
        return field->options().GetExtension(dccl::codec);
    else if(field->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE)
        return field->message_type()->options().GetExtension(dccl::message_codec);
    else
        return field->options().GetExtension(dccl::codec);
}


