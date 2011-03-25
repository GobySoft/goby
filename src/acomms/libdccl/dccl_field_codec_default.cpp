// copyright 2009-2011 t. schneider tes@mit.edu
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

#include "dccl_field_codec_default.h"



goby::acomms::Bitset goby::acomms::DCCLDefaultMessageCodec::_encode(const boost::any& field_value)
{
    goby::acomms::Bitset bits;
        
    const google::protobuf::Message* msg = DCCLFieldCodec::this_message();
    
    const google::protobuf::Descriptor* desc = msg->GetDescriptor();
    const google::protobuf::Reflection* refl = msg->GetReflection();       
    for(int i = 0, n = desc->field_count(); i < n; ++i)
    {
        const google::protobuf::FieldDescriptor* field_desc = desc->field(i);
        std::string field_codec = field_desc->options().GetExtension(dccl::codec);

        if(field_desc->options().GetExtension(dccl::omit)
           || (in_header() && !field_desc->options().GetExtension(dccl::in_head))
           || (!in_header() && field_desc->options().GetExtension(dccl::in_head)))
        {
            continue;
        }
        else
        {
            boost::shared_ptr<DCCLFieldCodec> codec =
                DCCLCodec::codec_manager().find(field_desc->cpp_type(), field_codec);
            boost::shared_ptr<FromProtoCppTypeBase> helper =
                DCCLCodec::cpptype_helper().find(field_desc->cpp_type());

            if(field_desc->is_repeated())
            {
                
                    
                std::vector<boost::any> field_values;
                for(int j = 0, m = refl->FieldSize(*msg, field_desc); j < m; ++j)
                {
                    field_values.push_back(helper->get_repeated_value(field_desc,
                                                                      *msg, j));
                }
                
                codec->encode(&bits, field_values, field_desc);
            }
            else
            {
                codec->encode(&bits, helper->get_value(field_desc, *msg), field_desc);
            }
        }
    }
    return bits;
}

boost::any goby::acomms::DCCLDefaultMessageCodec::_decode(Bitset* bits)
{
    google::protobuf::Message* msg = DCCLFieldCodec::mutable_this_message();
    
    const google::protobuf::Descriptor* desc = msg->GetDescriptor();
    const google::protobuf::Reflection* refl = msg->GetReflection();
    
    for(int i = desc->field_count()-1, n = 0; i >= n; --i)
    {
        const google::protobuf::FieldDescriptor* field_desc = desc->field(i);
        std::string field_codec = field_desc->options().GetExtension(dccl::codec);
            
        if(field_desc->options().GetExtension(dccl::omit)
           || (in_header() && !field_desc->options().GetExtension(dccl::in_head))
           || (!in_header() && field_desc->options().GetExtension(dccl::in_head)))
        {
            continue;
        }
        else
        {
            boost::shared_ptr<DCCLFieldCodec> codec =
                DCCLCodec::codec_manager().find(field_desc->cpp_type(), field_codec);
            boost::shared_ptr<FromProtoCppTypeBase> helper =
                DCCLCodec::cpptype_helper().find(field_desc->cpp_type());

            if(field_desc->is_repeated())
            {   
                std::vector<boost::any> field_values;
                codec->decode(bits, &field_values, field_desc);
                for(int j = field_values.size()-1, m = 0; j >= m; --j)
                    helper->add_value(field_desc, msg, field_values[j]);
            }
            else
            {
                boost::any field_value;            
                codec->decode(bits, &field_value, field_desc);
                helper->set_value(field_desc, msg, field_value);
            } 
        }    
    }
}


unsigned goby::acomms::DCCLDefaultMessageCodec::_size()
{
    unsigned sum = 0;
    
    const google::protobuf::Descriptor* desc =
        DCCLFieldCodec::this_message()->GetDescriptor();
    for(int i = 0, n = desc->field_count(); i < n; ++i)
    {
        const google::protobuf::FieldDescriptor* field_desc = desc->field(i);
        std::string field_codec = field_desc->options().GetExtension(dccl::codec);

        if(field_desc->options().GetExtension(dccl::omit)
           || (in_header() && !field_desc->options().GetExtension(dccl::in_head))
           || (!in_header() && field_desc->options().GetExtension(dccl::in_head)))
        {
            continue;
        }
        else
        {
            sum += DCCLCodec::codec_manager().find(
                field_desc->cpp_type(),field_codec)->size(this_message(), field_desc);
        }
    }
    
    return sum;    
}
void goby::acomms::DCCLDefaultMessageCodec::_validate()
{
}  



goby::acomms::Bitset goby::acomms::DCCLTimeCodec::_encode(const boost::any& field_value)
{
    if(field_value.empty())
        return Bitset(_size());
                
    try
    {
        // trim to time of day
        boost::posix_time::time_duration time_of_day =
            util::as<boost::posix_time::ptime>(
                boost::any_cast<std::string>(field_value)
                ).time_of_day();
                
        return Bitset(_size(),
                      static_cast<unsigned long>(
                          util::unbiased_round(
                              util::time_duration2double(time_of_day), 0)));
    }
    catch(boost::bad_any_cast& e)
    {
        throw(DCCLException("Bad type given to encode"));
    }
}
            
boost::any goby::acomms::DCCLTimeCodec::_decode(Bitset* bits)
{
    // add the date back in (assumes message sent the same day received)
    unsigned long v = bits->to_ulong();

    using namespace boost::posix_time;
    using namespace boost::gregorian;
                
    ptime now = util::goby_time();
    date day_sent;
                
    // this logic will break if there is a separation between message sending and
    // message receipt of greater than 1/2 day (twelve hours)
            
    // if message is from part of the day removed from us by 12 hours, we assume it
    // was sent yesterday
    if(abs(now.time_of_day().total_seconds() - v) > hours(12).total_seconds())
        day_sent = now.date() - days(1);
    else // otherwise figure it was sent today
        day_sent = now.date();
                
    return util::as<std::string>(ptime(day_sent,seconds(v)));
}
            
