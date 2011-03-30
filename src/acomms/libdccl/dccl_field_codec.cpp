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
#include "dccl.h"

bool goby::acomms::DCCLFieldCodec::in_header_ = false;
boost::signal<void (unsigned size)> goby::acomms::DCCLFieldCodec::get_more_bits;

std::vector<const google::protobuf::Message*> goby::acomms::DCCLFieldCodec::MessageHandler::msg_const_;
std::vector<google::protobuf::Message*> goby::acomms::DCCLFieldCodec::MessageHandler::msg_;

void goby::acomms::DCCLFieldCodec::encode(Bitset* bits,
                                          const boost::any& field_value,
                                          const google::protobuf::FieldDescriptor* field)
{
    field_ = field;

    MessageHandler msg_handler(field);
    if(msg_handler.first())
    {
        const google::protobuf::Message* msg = boost::any_cast<const google::protobuf::Message*>(field_value);
        if(msg)
            msg_handler.push(msg);
        else
            throw(DCCLException("Encode called with NULL Message"));

        
        
        if(DCCLCodec::log_)
            *DCCLCodec::log_ << debug << "Starting encode for root message (in header = " << std::boolalpha
                             << in_header() << "): "
                             << msg->GetDescriptor()->full_name() << std::endl;
    }

        
    if(DCCLCodec::log_ && field)
        *DCCLCodec::log_ << debug << "Starting encode for field: " << field->DebugString();
    
    Bitset new_bits = _encode(field_value);

    if(DCCLCodec::log_)
        *DCCLCodec::log_ << "got these bits: " << new_bits << std::endl;
    
    _prepend_bits(new_bits, bits);
}

void goby::acomms::DCCLFieldCodec::encode(Bitset* bits,
                                          const std::vector<boost::any>& field_values,
                                          const google::protobuf::FieldDescriptor* field)
{
    field_ = field;
    
    MessageHandler msg_handler(field);
    if(msg_handler.first())
    {
        throw(DCCLException("Encode called first with std::vector<boost::any>, must call with single boost::any containing a valid const google::protobuf::Message* to encode"));
    }


    
    Bitset new_bits = _encode_repeated(field_values);
    _prepend_bits(new_bits, bits);

}


unsigned goby::acomms::DCCLFieldCodec::max_size(const google::protobuf::Message* msg,
                                                const google::protobuf::FieldDescriptor* field)
{
    field_ = field;
    
    MessageHandler msg_handler(field);
    if(msg_handler.first())
    {
        if(msg)
            msg_handler.push(msg);
        else
            throw(DCCLException("Max Size called with NULL Message"));

    }
    
    if(field_)
        return field_->is_repeated() ? _max_size_repeated() : _max_size();
    else
        return _max_size();
}

unsigned goby::acomms::DCCLFieldCodec::min_size(const google::protobuf::Message* msg,
                                                const google::protobuf::FieldDescriptor* field)
{
    field_ = field;
    
    MessageHandler msg_handler(field);
    if(msg_handler.first())
    {
        if(msg)
            msg_handler.push(msg);
        else
            throw(DCCLException("Min Size called with NULL Message"));

    }
    
    if(field_)
        return field_->is_repeated() ? _min_size_repeated() : _min_size();
    else
        return _min_size();
}



void goby::acomms::DCCLFieldCodec::validate(const google::protobuf::Message* msg,
                                            const google::protobuf::FieldDescriptor* field)
{
    field_ = field;
    
    MessageHandler msg_handler(field);
    if(msg_handler.first())
    {
        if(msg)
            msg_handler.push(msg);
        else
            throw(DCCLException("Validate called with NULL Message"));
    }

    if(in_header() && _variable_size())
        throw(DCCLException("Variable size codec used in header - header fields must be encoded with fixed size codec."));

    
    
    
    _validate();
}

void goby::acomms::DCCLFieldCodec::info(const google::protobuf::Message* msg,
                                        const google::protobuf::FieldDescriptor* field,
                                        std::ostream* os)
{
    field_ = field;

    MessageHandler msg_handler(field);
    std::string indent = "  ";
    if(msg_handler.first())
    {
        if(msg)
            msg_handler.push(msg);
        else
            throw(DCCLException("info called with NULL Message"));

        indent = "";
    }



    std::string s;

    if(field_)
        s = field_->DebugString();

    s += _info();
    
    unsigned max_size = _max_size();
    unsigned bytes_max_size = ceil_bits2bytes(max_size);   

    s += ":: max size = " + goby::util::as<std::string>(max_size) + " bit(s) ["
        + goby::util::as<std::string>(bytes_max_size) + " byte(s)]";

    
    boost::trim(s);
    boost::replace_all(s, "\n", "\n" + indent);
    s = indent + s;

    *os << s << "\n";
}


std::string goby::acomms::DCCLFieldCodec::_info()
{
    return std::string();
}



void goby::acomms::DCCLFieldCodec::_prepend_bits(const Bitset& new_bits, Bitset* bits)
{    
    for(int i = 0, n = new_bits.size(); i < n; ++i)
        bits->push_back(new_bits[i]);
}

void goby::acomms::DCCLFieldCodec::decode(Bitset* bits,
                                          boost::any* field_value,
                                          const google::protobuf::FieldDescriptor* field)
{
    field_ = field;
    
    if(!field_value)
        throw(DCCLException("Decode called with NULL boost::any"));
    else if(!bits)
        throw(DCCLException("Decode called with NULL Bitset"));
    
    MessageHandler msg_handler(field);
    if(msg_handler.first())
    {
        google::protobuf::Message* msg = boost::any_cast<google::protobuf::Message*>(*field_value);
        if(msg)
            msg_handler.push(msg);
        else
            throw(DCCLException("Decode called with NULL Message"));

        if(DCCLCodec::log_)
            *DCCLCodec::log_ << debug << "Starting decode for root message (in header = " << std::boolalpha
                             << in_header() << "): "
                             << msg->GetDescriptor()->full_name() << std::endl;
    }

    
    
    if(DCCLCodec::log_ && field)
        *DCCLCodec::log_ << debug << "Starting decode for field: " << field->DebugString();

    if(DCCLCodec::log_)
    {
        *DCCLCodec::log_ << "bits (" << bits << ") " << *bits <<  std::endl;
        *DCCLCodec::log_ << "min size: " << _min_size() << std::endl;
    }
    
    
    Bitset these_bits;
    _get_bits(&these_bits, bits, _min_size());
    boost::signals::scoped_connection c = get_more_bits.connect(boost::bind(&DCCLFieldCodec::_get_bits, this,
                                                                       &these_bits, bits, _1), boost::signals::at_back);
    if(DCCLCodec::log_)
        *DCCLCodec::log_ << "making connection from bits: " << bits << " to these bits: " << &these_bits << std::endl;

    
    
    if(DCCLCodec::log_)
        *DCCLCodec::log_ << "using these bits: " << these_bits << std::endl;
    
    *field_value = _decode(&these_bits);
}

void goby::acomms::DCCLFieldCodec::decode(Bitset* bits,
                                          std::vector<boost::any>* field_values,
                                          const google::protobuf::FieldDescriptor* field)
{
    field_ = field;

    if(!field_values)
        throw(DCCLException("Decode called with NULL std::vector<boost::any>"));
    else if(!bits)
        throw(DCCLException("Decode called with NULL Bitset"));
    
    MessageHandler msg_handler(field);
    if(msg_handler.first())
    {
        throw(DCCLException("Decode called first with std::vector<boost::any>, must call with single boost::any* containing a valid google::protobuf::Message* to store result"));
    }

    
    Bitset these_bits;
    _get_bits(&these_bits, bits,  _max_size_repeated());


    *field_values = _decode_repeated(&these_bits);
}

void goby::acomms::DCCLFieldCodec::_get_bits(Bitset* these_bits, Bitset* bits, unsigned size)
{
    if(DCCLCodec::log_)
    {
        if(field_)
            *DCCLCodec::log_ << debug << "field is " << field_->DebugString() << std::endl;
        else
            *DCCLCodec::log_ << debug << "field is root message" << std::endl;
        *DCCLCodec::log_ << debug << "_get_bits from (" << bits << ") " << *bits << " to add to (" << these_bits << ") " << *these_bits << std::endl;
    }
    // 
    
    for(int i = 0, n = size; i < n; ++i)
        these_bits->push_back((*bits)[i]);

    *bits >>= size;
    bits->resize(bits->size()-size);
}


goby::acomms::Bitset
goby::acomms::DCCLFieldCodec::_encode_repeated(const std::vector<boost::any>& field_values)
{
    Bitset out_bits;
    // out_bits = [field_values[2]][field_values[1]][field_values[0]]
    BOOST_FOREACH(const boost::any& value, field_values)
        encode(&out_bits, value, field_);
    return out_bits;
    
}


std::vector<boost::any>
goby::acomms::DCCLFieldCodec::_decode_repeated(Bitset* repeated_bits)
{
    std::vector<boost::any> out_values;    
    for(int i = 0, n = field_->options().GetExtension(dccl::max_repeat); i < n; ++i)
    {
        boost::any value;
        decode(repeated_bits, &value, field_);
        out_values.push_back(value);
    }
    return out_values;
}

unsigned goby::acomms::DCCLFieldCodec::_max_size_repeated()
{    
    if(!field_->options().HasExtension(dccl::max_repeat))
        throw(DCCLException("Missing dccl.max_repeat option on `repeated` field"));
    else
        return _max_size() * field_->options().GetExtension(dccl::max_repeat);
}

unsigned goby::acomms::DCCLFieldCodec::_min_size_repeated()
{    
    if(!field_->options().HasExtension(dccl::max_repeat))
        throw(DCCLException("Missing dccl.max_repeat option on `repeated` field"));
    else
        return _min_size() * field_->options().GetExtension(dccl::max_repeat);
}




void goby::acomms::DCCLFieldCodec::MessageHandler::push(const google::protobuf::Message* msg)

{
    msg_const_.push_back(msg);

    ++messages_pushed_;
    
    if(DCCLCodec::log_)
        *DCCLCodec::log_ << debug << "Added message  " << msg->GetDescriptor()->full_name() << std::endl;
}
            
void goby::acomms::DCCLFieldCodec::MessageHandler::__pop()
{
    if(DCCLCodec::log_)
        *DCCLCodec::log_ << debug << "Removed message  " << msg_const_.back()->GetDescriptor()->full_name() << std::endl;

    if(!msg_.empty())
        msg_.pop_back();
    if(!msg_const_.empty())
        msg_const_.pop_back();

}

goby::acomms::DCCLFieldCodec::MessageHandler::MessageHandler(const google::protobuf::FieldDescriptor* field)
    : messages_pushed_(0)
{
    
    if(field && field->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE)
    {
        if(mutable_this_message())
            push(mutable_this_message()->GetReflection()->MutableMessage(mutable_this_message(), field));
        else
            push(&(this_message()->GetReflection()->GetMessage(*this_message(), field)));
    }
}
