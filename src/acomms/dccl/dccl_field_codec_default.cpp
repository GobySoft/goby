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
// DCCLDefaultIdentifierCodec
//

goby::acomms::Bitset goby::acomms::DCCLDefaultIdentifierCodec::encode()
{
    return encode(0);
}

goby::acomms::Bitset goby::acomms::DCCLDefaultIdentifierCodec::encode(const uint32& id)
{
    if(id <= ONE_BYTE_MAX_ID)
    {
        return(goby::acomms::Bitset(size(id), id));
    }                
    else
    {
        goby::acomms::Bitset return_bits(size(id), id);
        // set MSB to indicate long header form
        return_bits.set(return_bits.size()-1);
        return return_bits;
    }
}

goby::uint32 goby::acomms::DCCLDefaultIdentifierCodec::decode(const Bitset& bits)
{
    if(bits.test(bits.size()-1))
    {
        // long header
        // grabs more bits to add to the MSBs of `bits`
        get_more_bits((LONG_FORM_ID_BYTES - SHORT_FORM_ID_BYTES)*BITS_IN_BYTE);
        
        return bits.to_ulong() - (1 << (bits.size()-1));
    }
    else
    {
        // short header
        return bits.to_ulong();
    }
}

unsigned goby::acomms::DCCLDefaultIdentifierCodec::size()
{
    return size(0);
}

unsigned goby::acomms::DCCLDefaultIdentifierCodec::size(const uint32& id)
{
    if(id < 0 || id > TWO_BYTE_MAX_ID)
        throw(DCCLException("dccl.id provided (" + goby::util::as<std::string>(id) + ") is less than 0 or exceeds maximum: " + goby::util::as<std::string>(int(TWO_BYTE_MAX_ID))));
    
    return (id <= ONE_BYTE_MAX_ID) ?
        SHORT_FORM_ID_BYTES*BITS_IN_BYTE :
        LONG_FORM_ID_BYTES*BITS_IN_BYTE;

}

unsigned goby::acomms::DCCLDefaultIdentifierCodec::max_size()
{
    return LONG_FORM_ID_BYTES * BITS_IN_BYTE;
}

unsigned goby::acomms::DCCLDefaultIdentifierCodec::min_size()
{
    return SHORT_FORM_ID_BYTES * BITS_IN_BYTE;
}


        

//
// DCCLDefaultBoolCodec
//

goby::acomms::Bitset goby::acomms::DCCLDefaultBoolCodec::encode()
{
    return Bitset(size());
}

goby::acomms::Bitset goby::acomms::DCCLDefaultBoolCodec::encode(const bool& wire_value)
{
    return Bitset(size(), this_field()->is_required() ? wire_value : wire_value + 1);
}

bool goby::acomms::DCCLDefaultBoolCodec::decode(const Bitset& bits)
{
    unsigned long t = bits.to_ulong();
    if(this_field()->is_required())
    {
        return t;
    }
    else if(t)
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
    const unsigned NULL_VALUE = this_field()->is_required() ? 0 : 1;
    
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
    if(s.size() > dccl_field_options().max_length())
    {
        goby::glog.is(warn) && goby::glog << "String " << s <<  " exceeds `dccl.max_length`, truncating" << std::endl;
        s.resize(dccl_field_options().max_length()); 
    }
        
            
    Bitset value_bits;
    string2bitset(&value_bits, s);
        
    Bitset length_bits(min_size(), s.length());

    // adds to MSBs
    for(int i = 0, n = value_bits.size(); i < n; ++i)
        length_bits.push_back(value_bits[i]);

    return length_bits;
}

std::string goby::acomms::DCCLDefaultStringCodec::decode(const Bitset& bits)
{
    unsigned value_length = bits.to_ulong();
    
    if(value_length)
    {
        
        unsigned header_length = min_size();
        
        goby::glog.is(debug2) && goby::glog << "Length of string is = " << value_length << std::endl;

        
        goby::glog.is(debug2) && goby::glog << "bits before get_more_bits " << bits << std::endl;    

        // grabs more bits to add to the MSBs of `bits`
        get_more_bits(value_length*BITS_IN_BYTE);

        
        goby::glog.is(debug2) && goby::glog << "bits after get_more_bits " << bits << std::endl;    
        Bitset string_body_bits = bits;
        string_body_bits >>= header_length;
        string_body_bits.resize(bits.size() - header_length);
    
        std::string value;
        bitset2string(string_body_bits, &value);
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
    return min_size() + dccl_field_options().max_length() * BITS_IN_BYTE;
}

unsigned goby::acomms::DCCLDefaultStringCodec::min_size()
{
    return std::ceil(std::log(MAX_STRING_LENGTH+1)/std::log(2));
}


void goby::acomms::DCCLDefaultStringCodec::validate()
{
    require(dccl_field_options().has_max_length(), "missing (goby.field).dccl.max_length");
    require(dccl_field_options().max_length() <= MAX_STRING_LENGTH,
            "(goby.field).dccl.max_length must be <= " + goby::util::as<std::string>(static_cast<int>(MAX_STRING_LENGTH)));
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

    if(!this_field()->is_required())
        bits.set(0, true); // presence bit
        
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


std::string goby::acomms::DCCLDefaultBytesCodec::decode(const Bitset& bits)
{
    bool present = (this_field()->is_required()) ? true : bits.to_ulong();
    if(present)
    {
        // grabs more bits to add to the MSBs of `bits`
        get_more_bits(max_size()- min_size());

        Bitset bytes_body_bits = bits;
        bytes_body_bits >>= min_size();
        bytes_body_bits.resize(bits.size() - min_size());
        
        std::string value;
        bitset2string(bytes_body_bits, &value);
        return value;
    }
    else
    {
        throw(DCCLNullValueException());
    }
    
}

unsigned goby::acomms::DCCLDefaultBytesCodec::max_size()
{
    return min_size() + dccl_field_options().max_length() * BITS_IN_BYTE;
}

unsigned goby::acomms::DCCLDefaultBytesCodec::min_size()
{
    if(this_field()->is_required())
        return 0;
    else
        return 1; // presence bit
}

void goby::acomms::DCCLDefaultBytesCodec::validate()
{
    require(dccl_field_options().has_max_length(), "missing (goby.field).dccl.max_length");
}

//
// DCCLDefaultEnumCodec
//
goby::int32 goby::acomms::DCCLDefaultEnumCodec::pre_encode(const google::protobuf::EnumValueDescriptor* const& field_value)
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
// DCCLModemIdConverterCodec
//

boost::bimap<std::string, goby::int32> goby::acomms::DCCLModemIdConverterCodec::platform2modem_id_;

goby::int32 goby::acomms::DCCLModemIdConverterCodec::pre_encode(const std::string& field_value)
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


