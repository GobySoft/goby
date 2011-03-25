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

#include <boost/foreach.hpp>

#include "dccl_field_codec.h"
#include "dccl_exception.h"
#include "goby/protobuf/dccl_option_extensions.pb.h"

const google::protobuf::Message* goby::acomms::DCCLFieldCodec::msg_ = 0;
bool goby::acomms::DCCLFieldCodec::in_header_ = false;

void goby::acomms::DCCLFieldCodec::encode(Bitset* bits,
                                          const boost::any& field_value,
                                          const google::protobuf::FieldDescriptor* field)
{
    Bitset new_bits = _encode(field_value, field);
    encode_common(&new_bits, bits, size(field));
}

void goby::acomms::DCCLFieldCodec::encode(Bitset* bits,
                                          const std::vector<boost::any>& field_values,
                                          const google::protobuf::FieldDescriptor* field)
{
    Bitset new_bits = repeated_encode(field_values, field);
    encode_common(&new_bits, bits, repeated_size(field));
}

void goby::acomms::DCCLFieldCodec::encode_common(Bitset* new_bits, Bitset* bits, unsigned size)
{
    new_bits->resize(size);
    
    for(int i = 0, n = size; i < n; ++i)
        bits->push_back((*new_bits)[i]);
}

void goby::acomms::DCCLFieldCodec::decode(Bitset* bits,
                                          boost::any* field_value,
                                          const google::protobuf::FieldDescriptor* field)
{
    Bitset these_bits;
    decode_common(&these_bits, bits, size(field));
    *field_value = _decode(these_bits, field);
}

void goby::acomms::DCCLFieldCodec::decode(Bitset* bits,
                                          std::vector<boost::any>* field_values,
                                          const google::protobuf::FieldDescriptor* field)
{
    Bitset these_bits;
    decode_common(&these_bits, bits, repeated_size(field));
    *field_values = repeated_decode(&these_bits, field);
}

void goby::acomms::DCCLFieldCodec::decode_common(Bitset* these_bits, Bitset* bits, unsigned size)
{
    for(int i = bits->size()-size, n = bits->size(); i < n; ++i)
        these_bits->push_back((*bits)[i]);
    bits->resize(bits->size()-size);
}


goby::acomms::Bitset
goby::acomms::DCCLFieldCodec::repeated_encode(const std::vector<boost::any>& field_values,
                                              const google::protobuf::FieldDescriptor* field)
{
    Bitset out_bits;
    // out_bits = [field_values[2]][field_values[1]][field_values[0]]
    BOOST_FOREACH(const boost::any& value, field_values)
        encode(&out_bits, value, field);
    return out_bits;
    
}


std::vector<boost::any>
goby::acomms::DCCLFieldCodec::repeated_decode(Bitset* repeated_bits,
                                              const google::protobuf::FieldDescriptor* field)
{
    std::vector<boost::any> out_values;    
    for(int i = 0, n = field->options().GetExtension(dccl::max_repeat); i < n; ++i)
    {
        boost::any value;
        decode(repeated_bits, &value, field);
        out_values.push_back(value);
    }
    return out_values;
}

unsigned goby::acomms::DCCLFieldCodec::size(const google::protobuf::FieldDescriptor* field)
{
    return field->is_repeated() ? repeated_size(field) : _size(field);
}

unsigned goby::acomms::DCCLFieldCodec::repeated_size(const google::protobuf::FieldDescriptor* field)
{
    if(!field->options().HasExtension(dccl::max_repeat))
        throw(DCCLException("Missing dccl.max_repeat option on `repeated` field"));
    else
        return _size(field) * field->options().GetExtension(dccl::max_repeat);
}



