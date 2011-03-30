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

#include <sstream>

//
// DCCLDefaultBoolCodec
//

goby::acomms::Bitset goby::acomms::DCCLDefaultBoolCodec::_encode(const boost::any& field_value)
{
    if(field_value.empty())
        return Bitset(_max_size());
    
    try
    {
        bool b = boost::any_cast<bool>(field_value);
        return Bitset(_max_size(), (b ? 1 : 0) + 1);
    }
    catch(boost::bad_any_cast& e)
    {
        throw(DCCLException("Bad type given to encode"));
    }
}

boost::any goby::acomms::DCCLDefaultBoolCodec::_decode(Bitset* bits)
{
    unsigned long t = bits->to_ulong();
    if(t)
    {
        --t;
        return static_cast<bool>(t);
    }
    else
    {
        return boost::any();
    }
}


unsigned goby::acomms::DCCLDefaultBoolCodec::_max_size()
{    
    // true and false
    const unsigned BOOL_VALUES = 2;
    // if field unspecified
    const unsigned NULL_VALUE = 1;
    
    return std::ceil(std::log(BOOL_VALUES + NULL_VALUE)/std::log(2));
}

void goby::acomms::DCCLDefaultBoolCodec::_validate()
{ }

//
// DCCLDefaultStringCodec
//

goby::acomms::Bitset goby::acomms::DCCLDefaultStringCodec::_encode(const boost::any& field_value)
{
    try
    {
        if(field_value.empty())
            return Bitset(_min_size());

        std::string s = boost::any_cast<std::string>(field_value);

        Bitset value_bits;
        string2bitset(&value_bits, s);

        Bitset length_bits(_min_size(), s.length());

        // adds to MSBs
        for(int i = 0, n = value_bits.size(); i < n; ++i)
            length_bits.push_back(value_bits[i]);

        return length_bits;
    }
    catch(boost::bad_any_cast& e)
    {
        throw(DCCLException("Bad type given to encode, expecting std::string"));
    }
}

boost::any goby::acomms::DCCLDefaultStringCodec::_decode(Bitset* bits)
{
    unsigned value_length = bits->to_ulong();

    if(value_length)
    {
        
        unsigned header_length = _min_size();
    
        if(DCCLCodec::log_)
            *DCCLCodec::log_ << debug << "Length of string is = " << value_length << std::endl;

        if(DCCLCodec::log_)
            *DCCLCodec::log_ << debug << "bits before get_more_bits " << *bits << std::endl;    

        // grabs more bits to add to the MSBs of `bits`
        get_more_bits(value_length*BITS_IN_BYTE);

        if(DCCLCodec::log_)
            *DCCLCodec::log_ << debug << "bits after get_more_bits " << *bits << std::endl;    

        *bits >>= header_length;
        bits->resize(bits->size() - header_length);
    
        std::string value;
        bitset2string(*bits, &value);
        return value;
    }
    else
    {
        return boost::any();
    }
    
}

unsigned goby::acomms::DCCLDefaultStringCodec::_max_size()
{
    // string length + actual string
    return _min_size() + get(dccl::max_length) * BITS_IN_BYTE;
}

unsigned goby::acomms::DCCLDefaultStringCodec::_min_size()
{
    return std::ceil(std::log(MAX_STRING_LENGTH+1)/std::log(2));
}


void goby::acomms::DCCLDefaultStringCodec::_validate()
{
    require(dccl::max_length, "dccl.max_length");
    require(get(dccl::max_length) <= MAX_STRING_LENGTH,
            "dccl.max_length must be <= " + goby::util::as<std::string>(static_cast<int>(MAX_STRING_LENGTH)));
}

//
// DCCLDefaultEnumCodec
//

goby::acomms::Bitset goby::acomms::DCCLDefaultEnumCodec::_encode(const boost::any& field_value)
{
    try
    {

        boost::any field_value_int;
        double mx = 0, mn = 0;
        if(!field_value.empty())
        {
            const google::protobuf::EnumValueDescriptor* ev =
                boost::any_cast<const google::protobuf::EnumValueDescriptor*>(field_value);

            mx = max(ev->type());
            mn = min(ev->type());
            
            field_value_int = goby::util::as<int32>(ev->index());
        }
        
        return DCCLDefaultArithmeticFieldCodec<int32>::_encode(field_value_int, mx, mn);
    }
    catch(boost::bad_any_cast& e)
    {
        throw(DCCLException("Bad type given to encode, expecting const google::protobuf::EnumValueDescriptor*"));
    }
}

boost::any goby::acomms::DCCLDefaultEnumCodec::_decode(Bitset* bits)
{
    const google::protobuf::EnumDescriptor* e = field()->enum_type();
    boost::any index_any = DCCLDefaultArithmeticFieldCodec<int32>::_decode(bits, max(e), min(e));
    if(!index_any.empty())
    {
        int32 index = boost::any_cast<int32>(index_any);
        return e->value(index);
    }
    else
    {
        return boost::any();
    }
}


unsigned goby::acomms::DCCLDefaultEnumCodec::_max_size()
{
    const google::protobuf::EnumDescriptor* e = field()->enum_type();
    return DCCLDefaultArithmeticFieldCodec<int32>::_max_size(max(e), min(e));
}

void goby::acomms::DCCLDefaultEnumCodec::_validate()
{ }



//
// DCCLDefaultMessageCodec
//

goby::acomms::Bitset goby::acomms::DCCLDefaultMessageCodec::_encode(const boost::any& field_value)
{
    try
    {
        goby::acomms::Bitset bits;
        
        const google::protobuf::Message* msg = boost::any_cast<const google::protobuf::Message*>(field_value);

        if(DCCLCodec::log_)
            *DCCLCodec::log_ << debug << "Looking to encode Message " << msg->GetDescriptor()->full_name() << std::endl;

    
        const google::protobuf::Descriptor* desc = msg->GetDescriptor();
        const google::protobuf::Reflection* refl = msg->GetReflection();       
        for(int i = 0, n = desc->field_count(); i < n; ++i)
        {
            if(DCCLCodec::log_)
                *DCCLCodec::log_ << debug << "Looking to encode " << desc->field(i)->DebugString();

        
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
    catch(boost::bad_any_cast& e)
    {
        throw(DCCLException("Bad type given to encode, expecting const google::protobuf::Message*"));
    }
}

boost::any goby::acomms::DCCLDefaultMessageCodec::_decode(Bitset* bits)
{
    google::protobuf::Message* msg = DCCLFieldCodec::mutable_this_message();
    
    if(DCCLCodec::log_)
        *DCCLCodec::log_ << debug << "Looking to decode Message " << msg->GetDescriptor()->full_name() << std::endl;


    const google::protobuf::Descriptor* desc = msg->GetDescriptor();    
    for(int i = 0, n = desc->field_count(); i < n; ++i)
    {
        if(DCCLCodec::log_)
            *DCCLCodec::log_ << debug << "Looking to decode " << desc->field(i)->DebugString();

        
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
    return msg;
    
}


unsigned goby::acomms::DCCLDefaultMessageCodec::_max_size()
{
    unsigned sum = 0;

    if(DCCLCodec::log_)
        *DCCLCodec::log_ << debug << "Looking for max size of " << this_message()->GetDescriptor()->full_name() << std::endl;
    
    const google::protobuf::Descriptor* desc =
        DCCLFieldCodec::this_message()->GetDescriptor();
    for(int i = 0, n = desc->field_count(); i < n; ++i)
    {
        
        if(DCCLCodec::log_)
            *DCCLCodec::log_ << debug << "Looking to get max size of field " << desc->field(i)->DebugString();

        
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
            sum += DCCLCodec::codec_manager().find(field_desc->cpp_type(),field_codec)->
                max_size(DCCLFieldCodec::this_message(), field_desc);
        }
    }
    
    return sum;    
}

unsigned goby::acomms::DCCLDefaultMessageCodec::_min_size()
{
    unsigned sum = 0;

    if(DCCLCodec::log_)
        *DCCLCodec::log_ << debug << "Looking for min size of " << this_message()->GetDescriptor()->full_name() << std::endl;
    
    const google::protobuf::Descriptor* desc =
        DCCLFieldCodec::this_message()->GetDescriptor();
    for(int i = 0, n = desc->field_count(); i < n; ++i)
    {
        
        if(DCCLCodec::log_)
            *DCCLCodec::log_ << debug << "Looking to get min size of field " << desc->field(i)->DebugString();

        
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
            sum += DCCLCodec::codec_manager().find(field_desc->cpp_type(),field_codec)->
                min_size(DCCLFieldCodec::this_message(), field_desc);
        }
    }
    
    return sum;    
}

void goby::acomms::DCCLDefaultMessageCodec::_validate()
{
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
            DCCLCodec::codec_manager().find(field_desc->cpp_type(),field_codec)->
                validate(DCCLFieldCodec::this_message(), field_desc);
        }
    }    
}

std::string goby::acomms::DCCLDefaultMessageCodec::_info()
{
    std::stringstream ss;
    
    const google::protobuf::Descriptor* desc =
        DCCLFieldCodec::this_message()->GetDescriptor();

    
    ss << desc->full_name() << " {\n";
    
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
            DCCLCodec::codec_manager().find(field_desc->cpp_type(),field_codec)->
                info(DCCLFieldCodec::this_message(), field_desc, &ss);
        }
    }
    ss << "}\n";
    return ss.str();
}



//
// DCCLTimeCodec
//

// goby::acomms::Bitset goby::acomms::DCCLTimeCodec::_encode(const boost::any& field_value)
// {
//     if(field_value.empty())
//         return Bitset(_max_size());
                
//     try
//     {
//         // trim to time of day
//         boost::posix_time::time_duration time_of_day =
//             util::as<boost::posix_time::ptime>(
//                 boost::any_cast<std::string>(field_value)
//                 ).time_of_day();
                
//         return Bitset(_max_size(),
//                       static_cast<unsigned long>(
//                           util::unbiased_round(
//                               util::time_duration2double(time_of_day), 0)));
//     }
//     catch(boost::bad_any_cast& e)
//     {
//         throw(DCCLException("Bad type given to encode, expecting std::string"));
//     }
// }


// boost::any goby::acomms::DCCLTimeCodec::_decode(Bitset* bits)
// {
//     // add the date back in (assumes message sent the same day received)
//     unsigned long v = bits->to_ulong();

//     using namespace boost::posix_time;
//     using namespace boost::gregorian;
                
//     ptime now = util::goby_time();
//     date day_sent;
                
//     // this logic will break if there is a separation between message sending and
//     // message receipt of greater than 1/2 day (twelve hours)

                
//     return util::as<std::string>(ptime(day_sent,seconds(v)));
// }
            
