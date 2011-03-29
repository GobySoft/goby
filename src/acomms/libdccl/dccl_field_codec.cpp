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

google::protobuf::Message* goby::acomms::DCCLFieldCodec::root_msg_ = 0;
google::protobuf::Message* goby::acomms::DCCLFieldCodec::this_msg_ = 0;
const google::protobuf::Message* goby::acomms::DCCLFieldCodec::root_msg_const_ = 0;
const google::protobuf::Message* goby::acomms::DCCLFieldCodec::this_msg_const_ = 0;
bool goby::acomms::DCCLFieldCodec::in_header_ = false;
int goby::acomms::DCCLFieldCodec::Counter::i_ = 0;

void goby::acomms::DCCLFieldCodec::encode(Bitset* bits,
                                          const boost::any& field_value,
                                          const google::protobuf::FieldDescriptor* field)
{
    Counter c;
    if(c.first())
    {
        const google::protobuf::Message* msg = boost::any_cast<const google::protobuf::Message*>(field_value);
        if(msg)
            __set_root_message(msg);
        else
            throw(DCCLException("Encode called with NULL Message"));
    }
    else
    {
        __try_set_this(field);
    }
    
    Bitset new_bits = _encode(field_value);
    __encode_common(&new_bits, bits, size());
}

void goby::acomms::DCCLFieldCodec::encode(Bitset* bits,
                                          const std::vector<boost::any>& field_values,
                                          const google::protobuf::FieldDescriptor* field)
{
    Counter c;
    if(c.first())
    {
        throw(DCCLException("Encode called first with std::vector<boost::any>, must call with single boost::any containing a valid const google::protobuf::Message* to encode"));
    }
    else
    {
        __try_set_this(field);
    }
    
    Bitset new_bits = _encode_repeated(field_values);
    __encode_common(&new_bits, bits, size());
}


unsigned goby::acomms::DCCLFieldCodec::size(const google::protobuf::Message* msg,
                                            const google::protobuf::FieldDescriptor* field)
{
    Counter c;
    if(c.first())
    {
        if(msg)
        {
            __set_root_message(msg);
        }
        else
        {
            throw(DCCLException("Size called with NULL Message"));
        }
    }
    else
    {
        __try_set_this(field);
    }
    
    if(this_field())
        return this_field()->is_repeated() ? _size_repeated() : _size();
    else
        return _size();
}


void goby::acomms::DCCLFieldCodec::validate(const google::protobuf::Message* msg,
                                            const google::protobuf::FieldDescriptor* field)
{
    Counter c;
    if(c.first())
    {
        if(msg)
            __set_root_message(msg);
        else
            throw(DCCLException("Validate called with NULL Message"));
    }
    else
    {
        __try_set_this(field);
    }
    
    _validate();
}



void goby::acomms::DCCLFieldCodec::__encode_common(Bitset* new_bits, Bitset* bits, unsigned size)
{
    new_bits->resize(size);
    
    for(int i = 0, n = size; i < n; ++i)
        bits->push_back((*new_bits)[i]);
}

void goby::acomms::DCCLFieldCodec::decode(Bitset* bits,
                                          boost::any* field_value,
                                          const google::protobuf::FieldDescriptor* field)
{
    if(!field_value)
        throw(DCCLException("Decode called with NULL boost::any"));
    else if(!bits)
        throw(DCCLException("Decode called with NULL Bitset"));

    Counter c;
    if(c.first())
    {
        google::protobuf::Message* msg = boost::any_cast<google::protobuf::Message*>(*field_value);
        if(msg)
            __set_root_message(msg);
        else
            throw(DCCLException("Decode called with NULL Message"));
    }
    else
    {
        __try_set_this(field);
    }
    
    Bitset these_bits;
    __decode_common(&these_bits, bits, size());
    *field_value = _decode(&these_bits);
}

void goby::acomms::DCCLFieldCodec::decode(Bitset* bits,
                                          std::vector<boost::any>* field_values,
                                          const google::protobuf::FieldDescriptor* field)
{
    if(!field_values)
        throw(DCCLException("Decode called with NULL std::vector<boost::any>"));
    else if(!bits)
        throw(DCCLException("Decode called with NULL Bitset"));
    
    Counter c;
    if(c.first())
    {
        throw(DCCLException("Decode called first with std::vector<boost::any>, must call with single boost::any* containing a valid google::protobuf::Message* to store result"));
    }
    else
    {
        __try_set_this(field);
    }

    Bitset these_bits;
    __decode_common(&these_bits, bits, _size_repeated());
    *field_values = _decode_repeated(&these_bits);
}

void goby::acomms::DCCLFieldCodec::__decode_common(Bitset* these_bits, Bitset* bits, unsigned size)
{
    for(int i = bits->size()-size, n = bits->size(); i < n; ++i)
        these_bits->push_back((*bits)[i]);
    bits->resize(bits->size()-size);
}


goby::acomms::Bitset
goby::acomms::DCCLFieldCodec::_encode_repeated(const std::vector<boost::any>& field_values)
{
    Bitset out_bits;
    // out_bits = [field_values[2]][field_values[1]][field_values[0]]
    BOOST_FOREACH(const boost::any& value, field_values)
        encode(&out_bits, value);
    return out_bits;
    
}


std::vector<boost::any>
goby::acomms::DCCLFieldCodec::_decode_repeated(Bitset* repeated_bits)
{
    std::vector<boost::any> out_values;    
    for(int i = 0, n = this_field()->options().GetExtension(dccl::max_repeat); i < n; ++i)
    {
        boost::any value;
        decode(repeated_bits, &value);
        out_values.push_back(value);
    }
    return out_values;
}

unsigned goby::acomms::DCCLFieldCodec::_size_repeated()
{    
    if(!this_field()->options().HasExtension(dccl::max_repeat))
        throw(DCCLException("Missing dccl.max_repeat option on `repeated` field"));
    else
        return _size() * this_field()->options().GetExtension(dccl::max_repeat);
}



void goby::acomms::DCCLFieldCodec::__try_set_this(const google::protobuf::FieldDescriptor* field)
{
    if(!field || field == this_field_)
        return;

    this_field_ = field;
    
    if(field->cpp_type() ==
       google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE)
    {
        if(this_msg_)
            this_msg_ = this_msg_->GetReflection()->MutableMessage(this_msg_, field);
        if(this_msg_const_)        
            this_msg_const_ = &this_msg_const_->GetReflection()->GetMessage(*this_msg_const_, field);
    } 
}  
