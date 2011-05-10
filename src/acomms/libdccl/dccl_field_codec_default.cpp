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

goby::acomms::Bitset goby::acomms::DCCLDefaultBoolCodec::any_encode(const boost::any& wire_value)
{
    if(wire_value.empty())
        return Bitset(size());
    
    try
    {
        bool b = boost::any_cast<bool>(wire_value);
        return Bitset(size(), (b ? 1 : 0) + 1);
    }
    catch(boost::bad_any_cast& e)
    {
        throw(DCCLException("Bad type given to encode"));
    }
}

boost::any goby::acomms::DCCLDefaultBoolCodec::any_decode(Bitset* bits)
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

goby::acomms::Bitset goby::acomms::DCCLDefaultStringCodec::any_encode(const boost::any& wire_value)
{
    try
    {
        if(wire_value.empty())
            return Bitset(min_size());

        std::string s = boost::any_cast<std::string>(wire_value);
        if(s.size() > get(dccl::max_length))
        {
            DCCLCommon::logger() << warn << "String " << s <<  " exceeds `dccl.max_length`, truncating" << std::endl;
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
    catch(boost::bad_any_cast& e)
    {
        throw(DCCLException("Bad type given to encode, expecting std::string"));
    }
}

boost::any goby::acomms::DCCLDefaultStringCodec::any_decode(Bitset* bits)
{
    unsigned value_length = bits->to_ulong();

    if(value_length)
    {
        
        unsigned header_length = min_size();
    
        
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

unsigned goby::acomms::DCCLDefaultStringCodec::any_size(const boost::any& field_value)
{
    try
    {
        if(field_value.empty())
            return min_size();
        
        std::string s = boost::any_cast<std::string>(field_value);
        return std::min(min_size() + static_cast<unsigned>(s.length()*BITS_IN_BYTE), max_size());
    }
    catch(boost::bad_any_cast& e)
    {
        throw(DCCLException("Bad type given to encode, expecting std::string"));
    }
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

goby::acomms::Bitset goby::acomms::DCCLDefaultBytesCodec::any_encode(const boost::any& wire_value)
{
    try
    {
        if(wire_value.empty())
            return Bitset(min_size(), 0);

        std::string s = boost::any_cast<std::string>(wire_value);

        Bitset bits;
        string2bitset(&bits, s);
        bits.resize(max_size());
        bits <<= min_size();
        bits.set(0, true);
        
        return bits;
    }
    catch(boost::bad_any_cast& e)
    {
        throw(DCCLException("Bad type given to encode, expecting std::string"));
    }
}

unsigned goby::acomms::DCCLDefaultBytesCodec::any_size(const boost::any& field_value)
{
    if(field_value.empty())
        return min_size();
    else
        return max_size();
}


boost::any goby::acomms::DCCLDefaultBytesCodec::any_decode(Bitset* bits)
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
        return boost::any();
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

boost::any goby::acomms::DCCLDefaultEnumCodec::any_pre_encode(const boost::any& field_value)
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

boost::any goby::acomms::DCCLDefaultEnumCodec::any_post_decode(const boost::any& wire_value)
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
// DCCLTimeCodec
//

boost::any goby::acomms::DCCLTimeCodec::any_pre_encode(const boost::any& field_value)
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

boost::any goby::acomms::DCCLTimeCodec::any_post_decode(const boost::any& wire_value)
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
boost::any goby::acomms::DCCLModemIdConverterCodec::any_pre_encode(const boost::any& field_value)
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
            
boost::any goby::acomms::DCCLModemIdConverterCodec::any_post_decode(const boost::any& wire_value)
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


