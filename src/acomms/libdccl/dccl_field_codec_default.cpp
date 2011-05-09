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


#include <sstream>
#include <algorithm>

#include "dccl_field_codec_default.h"
#include "dccl_type_helper.h"

//
// DCCLDefaultBoolCodec
//

goby::acomms::Bitset goby::acomms::DCCLDefaultBoolCodec::_encode(const boost::any& wire_value)
{
    if(wire_value.empty())
        return Bitset(_size());
    
    try
    {
        bool b = boost::any_cast<bool>(wire_value);
        return Bitset(_size(), (b ? 1 : 0) + 1);
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


unsigned goby::acomms::DCCLDefaultBoolCodec::_size()
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

goby::acomms::Bitset goby::acomms::DCCLDefaultStringCodec::_encode(const boost::any& wire_value)
{
    try
    {
        if(wire_value.empty())
            return Bitset(_min_size());

        std::string s = boost::any_cast<std::string>(wire_value);
        if(s.size() > get(dccl::max_length))
        {
            DCCLCommon::logger() << warn << "String " << s <<  " exceeds `dccl.max_length`, truncating" << std::endl;
            s.resize(get(dccl::max_length)); 
        }
        
            
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
    
        
        DCCLCommon::logger() << debug1 << "Length of string is = " << value_length << std::endl;

        
        DCCLCommon::logger() << debug1 << "bits before get_more_bits " << *bits << std::endl;    

        // grabs more bits to add to the MSBs of `bits`
        get_more_bits(value_length*BITS_IN_BYTE);

        
        DCCLCommon::logger() << debug1 << "bits after get_more_bits " << *bits << std::endl;    

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

unsigned goby::acomms::DCCLDefaultStringCodec::_size(const boost::any& field_value)
{
    try
    {
        if(field_value.empty())
            return _min_size();
        
        std::string s = boost::any_cast<std::string>(field_value);
        return std::min(_min_size() + static_cast<unsigned>(s.length()*BITS_IN_BYTE), _max_size());
    }
    catch(boost::bad_any_cast& e)
    {
        throw(DCCLException("Bad type given to encode, expecting std::string"));
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
// DCCLDefaultBytesCodec
//

goby::acomms::Bitset goby::acomms::DCCLDefaultBytesCodec::_encode(const boost::any& wire_value)
{
    try
    {
        if(wire_value.empty())
            return Bitset(_min_size(), 0);

        std::string s = boost::any_cast<std::string>(wire_value);

        Bitset bits;
        string2bitset(&bits, s);
        bits.resize(_max_size());
        bits <<= _min_size();
        bits.set(0, true);
        
        return bits;
    }
    catch(boost::bad_any_cast& e)
    {
        throw(DCCLException("Bad type given to encode, expecting std::string"));
    }
}

unsigned goby::acomms::DCCLDefaultBytesCodec::_size(const boost::any& field_value)
{
    if(field_value.empty())
        return _min_size();
    else
        return _max_size();
}


boost::any goby::acomms::DCCLDefaultBytesCodec::_decode(Bitset* bits)
{
    bool presence_flag = bits->to_ulong();
    if(presence_flag)
    {
        // grabs more bits to add to the MSBs of `bits`
        get_more_bits(_max_size()-_min_size());

        *bits >>= _min_size();
        bits->resize(bits->size() - _min_size());
        
        std::string value;
        bitset2string(*bits, &value);
        return value;
    }
    else
    {
        return boost::any();
    }
    
}

unsigned goby::acomms::DCCLDefaultBytesCodec::_max_size()
{
    return _min_size() + get(dccl::max_length) * BITS_IN_BYTE;
}

unsigned goby::acomms::DCCLDefaultBytesCodec::_min_size()
{
    // presence bit
    return 1;
}

void goby::acomms::DCCLDefaultBytesCodec::_validate()
{
    require(dccl::max_length, "dccl.max_length");
}

//
// DCCLDefaultEnumCodec
//

boost::any goby::acomms::DCCLDefaultEnumCodec::pre_encode(const boost::any& field_value)
{
    try
    {
        if(!field_value.empty())
        {
            const google::protobuf::EnumValueDescriptor* ev =
                boost::any_cast<const google::protobuf::EnumValueDescriptor*>(field_value);

            return goby::util::as<int32>(ev->index());
        }        
        else
        {
            return boost::any();
        }
    }
    catch(boost::bad_any_cast& e)
    {
        throw(DCCLException("Bad type given to encode, expecting const google::protobuf::EnumValueDescriptor*"));
    }
}

boost::any goby::acomms::DCCLDefaultEnumCodec::post_decode(const boost::any& wire_value)
{
    const google::protobuf::EnumDescriptor* e = this_field()->enum_type();
    if(!wire_value.empty())
    {
        int32 index = boost::any_cast<int32>(wire_value);
        return e->value(index);
    }
    else
    {
        return boost::any();
    }
}



//
// DCCLDefaultMessageCodec
//

            
goby::acomms::Bitset goby::acomms::DCCLDefaultMessageCodec::_encode(const boost::any& wire_value)
{
    if(wire_value.empty())
        return Bitset(_min_size());
    else
        return traverse_const<goby::acomms::Bitset>(wire_value,
                                                    &DCCLFieldCodec::encode,
                                                    &DCCLFieldCodec::encode_repeated);    
}

unsigned goby::acomms::DCCLDefaultMessageCodec::_size(const boost::any& field_value)
{
    
    if(field_value.empty())
        return _min_size();
    else
        return traverse_const<unsigned>(field_value,
                                        &DCCLFieldCodec::size,
                                        &DCCLFieldCodec::size_repeated);    
}

boost::any goby::acomms::DCCLDefaultMessageCodec::_decode(Bitset* bits)
{
    boost::shared_ptr<google::protobuf::Message> msg(
        DCCLCommon::message_factory().GetPrototype(DCCLFieldCodec::this_descriptor())->New());
    
//    
//        DCCLCommon::logger() << debug << "Looking to decode Message " << msg->GetDescriptor()->full_name() << std::endl;


    const google::protobuf::Descriptor* desc = msg->GetDescriptor();

    for(int i = 0, n = desc->field_count(); i < n; ++i)
    {
//        
//            DCCLCommon::logger() << debug << "Looking to decode " << desc->field(i)->DebugString();
        
        const google::protobuf::FieldDescriptor* field_desc = desc->field(i);

        if(!__check_field(field_desc))
            continue;

        std::string field_codec = __find_codec(field_desc);
        
        boost::shared_ptr<DCCLFieldCodec> codec =
            DCCLFieldCodecManager::find(field_desc, field_codec);
        boost::shared_ptr<FromProtoCppTypeBase> helper =
            DCCLTypeHelper::find(field_desc);

        if(field_desc->is_repeated())
        {   
            std::vector<boost::any> wire_values;
            codec->decode_repeated(bits, &wire_values, field_desc);
            for(int j = 0, m = wire_values.size(); j < m; ++j)
                helper->add_value(field_desc, msg.get(), wire_values[j]);
        }
        else
        {
            boost::any wire_value;
            codec->decode(bits, &wire_value, field_desc);
            helper->set_value(field_desc, msg.get(), wire_value);
        } 
    }

    std::vector< const google::protobuf::FieldDescriptor* > set_fields;
    msg->GetReflection()->ListFields(*msg, &set_fields);
    return set_fields.empty() ? boost::any() : msg;    
}


unsigned goby::acomms::DCCLDefaultMessageCodec::_max_size()
{
    unsigned sum = 0;

//    
//        DCCLCommon::logger() << debug << "Looking for max size of " << this_descriptor()->full_name() << std::endl;
    
    const google::protobuf::Descriptor* desc =
        DCCLFieldCodec::this_descriptor();
    for(int i = 0, n = desc->field_count(); i < n; ++i)
    {
        
        // 
        //     DCCLCommon::logger() << debug << "Looking to get max size of field " << desc->field(i)->DebugString();

        
        const google::protobuf::FieldDescriptor* field_desc = desc->field(i);

        if(!__check_field(field_desc))
            continue;

        std::string field_codec = __find_codec(field_desc);

        sum += DCCLFieldCodecManager::find(field_desc,field_codec)->
            max_size(field_desc);
    }
    
    return sum;    
}

unsigned goby::acomms::DCCLDefaultMessageCodec::_min_size()
{
    unsigned sum = 0;

    // 
    //     DCCLCommon::logger() << debug << "Looking for min size of " << this_descriptor()->full_name() << std::endl;
    
    const google::protobuf::Descriptor* desc =
        DCCLFieldCodec::this_descriptor();
    for(int i = 0, n = desc->field_count(); i < n; ++i)
    {
        
        // 
        //     DCCLCommon::logger() << debug << "Looking to get min size of field " << desc->field(i)->DebugString();
        const google::protobuf::FieldDescriptor* field_desc = desc->field(i);

        if(!__check_field(field_desc))
            continue;
        
        std::string field_codec = __find_codec(field_desc);

        sum += DCCLFieldCodecManager::find(field_desc,field_codec)->
            min_size(field_desc);
    }
    
    return sum;
}

void goby::acomms::DCCLDefaultMessageCodec::_validate()
{
    const google::protobuf::Descriptor* desc =
        DCCLFieldCodec::this_descriptor();
    for(int i = 0, n = desc->field_count(); i < n; ++i)
    {
        const google::protobuf::FieldDescriptor* field_desc = desc->field(i);

        if(!__check_field(field_desc))
            continue;

        
        std::string field_codec = __find_codec(field_desc);

        DCCLCommon::logger() << "field is " << field_desc->DebugString();
        DCCLCommon::logger() << "codec is " << field_codec << "\n" << std::endl;
        
        DCCLFieldCodecManager::find(field_desc,field_codec)->
            validate(field_desc);
    }    
}

std::string goby::acomms::DCCLDefaultMessageCodec::_info()
{
    std::stringstream ss;
    
    const google::protobuf::Descriptor* desc =
        DCCLFieldCodec::this_descriptor();

    
    ss << desc->full_name() << " {\n";
    
    for(int i = 0, n = desc->field_count(); i < n; ++i)
    {
        const google::protobuf::FieldDescriptor* field_desc = desc->field(i);

        if(!__check_field(field_desc))
            continue;

        std::string field_codec = __find_codec(field_desc);

        DCCLFieldCodecManager::find(field_desc,field_codec)->
            info(field_desc, &ss);
    }
    ss << "}\n";
    return ss.str();
}

bool goby::acomms::DCCLDefaultMessageCodec::__check_field(const google::protobuf::FieldDescriptor* field)
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

std::string goby::acomms::DCCLDefaultMessageCodec::__find_codec(const google::protobuf::FieldDescriptor* field)
{
    if(field->options().HasExtension(dccl::codec))
        return field->options().GetExtension(dccl::codec);
    else if(field->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE)
        return field->message_type()->options().GetExtension(dccl::message_codec);
    else
        return field->options().GetExtension(dccl::codec);
}




//
// DCCLTimeCodec
//

boost::any goby::acomms::DCCLTimeCodec::pre_encode(const boost::any& field_value)
{
    try
    {
        // trim to time of day
        return util::as<int32>(util::as<boost::posix_time::ptime>(
                                   boost::any_cast<std::string>(field_value)
                                   ).time_of_day().total_seconds());
    }
    catch(boost::bad_any_cast& e)
    {
        throw(DCCLException("Bad type given to pre-encode, expecting std::string"));
    }
}

boost::any goby::acomms::DCCLTimeCodec::post_decode(const boost::any& wire_value)
{
    if(!wire_value.empty())
    {
        // add the date back in (assumes message sent the same day received)
        int32 v = boost::any_cast<int32>(wire_value);

        using namespace boost::posix_time;
        using namespace boost::gregorian;
                
        ptime now = util::goby_time();
        date day_sent;
        // if message is from part of the day removed from us by 12 hours, we assume it
        // was sent yesterday
        if(abs(now.time_of_day().total_seconds() - double(v)) > hours(12).total_seconds())
            day_sent = now.date() - days(1);
        else // otherwise figure it was sent today
            day_sent = now.date();
                
        // this logic will break if there is a separation between message sending and
        // message receipt of greater than 1/2 day (twelve hours)               
        return util::as<std::string>(ptime(day_sent,seconds(v)));
    }
    else
    {
        return boost::any();
    }
}
//
// DCCLModemIdConverterCodec
//

boost::bimap<std::string, goby::acomms::int32> goby::acomms::DCCLModemIdConverterCodec::platform2modem_id_;
boost::any goby::acomms::DCCLModemIdConverterCodec::pre_encode(const boost::any& field_value)
{
    try
    {
        std::string s = boost::any_cast<std::string>(field_value);

        int32 v = BROADCAST_ID;
        if(platform2modem_id_.left.count(s))
            v = platform2modem_id_.left.at(s);
        
        return v;
    }
    catch(boost::bad_any_cast& e)
    {
        throw(DCCLException("Bad type given to encode, expecting std::string"));
    }    
}
            
boost::any goby::acomms::DCCLModemIdConverterCodec::post_decode(const boost::any& wire_value)
{
    if(!wire_value.empty())
    {
        int32 v = boost::any_cast<int32>(wire_value);

        if(v == BROADCAST_ID)
            return "broadcast";
        else if(platform2modem_id_.right.count(v))
            return platform2modem_id_.right.at(v);
        else
            return boost::any();
    }
    else
    {
        return boost::any();
    }
}            


