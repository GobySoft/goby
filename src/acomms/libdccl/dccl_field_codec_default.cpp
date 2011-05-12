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

goby::acomms::Bitset goby::acomms::DCCLDefaultBoolCodec::encode()
{
    return Bitset(size());
}

goby::acomms::Bitset goby::acomms::DCCLDefaultBoolCodec::encode(const bool& wire_value)
{
    return Bitset(size(), (wire_value ? 1 : 0) + 1);
}

bool goby::acomms::DCCLDefaultBoolCodec::decode(Bitset* bits)
{
    unsigned long t = bits->to_ulong();
    if(t)
    {
        --t;
        return t;
    }
    else
    {
        throw(DCCLNullValueException());
    }
}


unsigned goby::acomms::DCCLDefaultBoolCodec::size()
{    
    // true and false
    const unsigned BOOL_VALUES = 2;
    // if field unspecified
    const unsigned NULL_VALUE = 1;
    
    return std::ceil(std::log(BOOL_VALUES + NULL_VALUE)/std::log(2));
}

void goby::acomms::DCCLDefaultBoolCodec::validate()
{ }

//
// DCCLDefaultStringCodec
//

goby::acomms::Bitset goby::acomms::DCCLDefaultStringCodec::encode()
{
    return Bitset(min_size());
}

goby::acomms::Bitset goby::acomms::DCCLDefaultStringCodec::encode(const std::string& wire_value)
{
    std::string s = wire_value;
    if(s.size() > get(dccl::max_length))
    {
        goby::glog.is(warn) && goby::glog << "String " << s <<  " exceeds `dccl.max_length`, truncating" << std::endl;
        s.resize(get(dccl::max_length)); 
    }
        
            
    Bitset value_bits;
    string2bitset(&value_bits, s);
        
    Bitset length_bits(min_size(), s.length());

    // adds to MSBs
    for(int i = 0, n = value_bits.size(); i < n; ++i)
        length_bits.push_back(value_bits[i]);

    return length_bits;
}

std::string goby::acomms::DCCLDefaultStringCodec::decode(Bitset* bits)
{
    unsigned value_length = bits->to_ulong();

    if(value_length)
    {
        
        unsigned header_length = min_size();
        
        goby::glog.is(debug2) && goby::glog << "Length of string is = " << value_length << std::endl;

        
        goby::glog.is(debug2) && goby::glog << "bits before get_more_bits " << *bits << std::endl;    

        // grabs more bits to add to the MSBs of `bits`
        get_more_bits(value_length*BITS_IN_BYTE);

        
        goby::glog.is(debug2) && goby::glog << "bits after get_more_bits " << *bits << std::endl;    

        *bits >>= header_length;
        bits->resize(bits->size() - header_length);
    
        std::string value;
        bitset2string(*bits, &value);
        return value;
    }
    else
    {
        throw(DCCLNullValueException());
    }
    
}

unsigned goby::acomms::DCCLDefaultStringCodec::size()
{
    return min_size();
}

unsigned goby::acomms::DCCLDefaultStringCodec::size(const std::string& field_value)
{
    return std::min(min_size() + static_cast<unsigned>(field_value.length()*BITS_IN_BYTE), max_size());
}


unsigned goby::acomms::DCCLDefaultStringCodec::max_size()
{
    // string length + actual string
    return min_size() + get(dccl::max_length) * BITS_IN_BYTE;
}

unsigned goby::acomms::DCCLDefaultStringCodec::min_size()
{
    return std::ceil(std::log(MAX_STRING_LENGTH+1)/std::log(2));
}


void goby::acomms::DCCLDefaultStringCodec::validate()
{
    require(dccl::max_length, "dccl.max_length");
    require(get(dccl::max_length) <= MAX_STRING_LENGTH,
            "dccl.max_length must be <= " + goby::util::as<std::string>(static_cast<int>(MAX_STRING_LENGTH)));
}

//
// DCCLDefaultBytesCodec
//
goby::acomms::Bitset goby::acomms::DCCLDefaultBytesCodec::encode()
{
    return Bitset(min_size(), 0);
}


goby::acomms::Bitset goby::acomms::DCCLDefaultBytesCodec::encode(const std::string& wire_value)
{
    Bitset bits;
    string2bitset(&bits, wire_value);
    bits.resize(max_size());
    bits <<= min_size();
    bits.set(0, true);
        
    return bits;
}

unsigned goby::acomms::DCCLDefaultBytesCodec::size()
{
    return min_size();    
}


unsigned goby::acomms::DCCLDefaultBytesCodec::size(const std::string& field_value)
{
    return max_size();
}


std::string goby::acomms::DCCLDefaultBytesCodec::decode(Bitset* bits)
{
    bool presence_flag = bits->to_ulong();
    if(presence_flag)
    {
        // grabs more bits to add to the MSBs of `bits`
        get_more_bits(max_size()- min_size());

        *bits >>= min_size();
        bits->resize(bits->size() - min_size());
        
        std::string value;
        bitset2string(*bits, &value);
        return value;
    }
    else
    {
        throw(DCCLNullValueException());
    }
    
}

unsigned goby::acomms::DCCLDefaultBytesCodec::max_size()
{
    return min_size() + get(dccl::max_length) * BITS_IN_BYTE;
}

unsigned goby::acomms::DCCLDefaultBytesCodec::min_size()
{
    // presence bit
    return 1;
}

void goby::acomms::DCCLDefaultBytesCodec::validate()
{
    require(dccl::max_length, "dccl.max_length");
}

//
// DCCLDefaultEnumCodec
//
goby::acomms::int32 goby::acomms::DCCLDefaultEnumCodec::pre_encode(const google::protobuf::EnumValueDescriptor* const& field_value)
{
    return field_value->index();
}

const google::protobuf::EnumValueDescriptor* goby::acomms::DCCLDefaultEnumCodec::post_decode(const int32& wire_value)
{
    const google::protobuf::EnumDescriptor* e = this_field()->enum_type();
    const google::protobuf::EnumValueDescriptor* return_value = e->value(wire_value);

    if(return_value)
        return return_value;
    else
        throw(DCCLNullValueException());
}




//
// DCCLTimeCodec
//
goby::acomms::int32 goby::acomms::DCCLTimeCodec::pre_encode(const std::string& field_value)
{
    return util::as<boost::posix_time::ptime>(field_value).time_of_day().total_seconds();
}


std::string goby::acomms::DCCLTimeCodec::post_decode(const int32& wire_value)
{
    using namespace boost::posix_time;
    using namespace boost::gregorian;
        
    ptime now = util::goby_time();
    date day_sent;
    // if message is from part of the day removed from us by 12 hours, we assume it
    // was sent yesterday
    if(abs(now.time_of_day().total_seconds() - double(wire_value)) > hours(12).total_seconds())
        day_sent = now.date() - days(1);
    else // otherwise figure it was sent today
        day_sent = now.date();
                
    // this logic will break if there is a separation between message sending and
    // message receipt of greater than 1/2 day (twelve hours)               
    return util::as<std::string>(ptime(day_sent,seconds(wire_value)));
}
//
// DCCLModemIdConverterCodec
//

boost::bimap<std::string, goby::acomms::int32> goby::acomms::DCCLModemIdConverterCodec::platform2modem_id_;

goby::acomms::int32 goby::acomms::DCCLModemIdConverterCodec::pre_encode(const std::string& field_value)
{
    int32 v = BROADCAST_ID;
    if(platform2modem_id_.left.count(field_value))
        v = platform2modem_id_.left.at(field_value);
    
    return v;
}
            
std::string goby::acomms::DCCLModemIdConverterCodec::post_decode(const int32& wire_value)
{
    if(wire_value == BROADCAST_ID)
        return "broadcast";
    else if(platform2modem_id_.right.count(wire_value))
        return platform2modem_id_.right.at(wire_value);
    else
        throw DCCLNullValueException();
}            


