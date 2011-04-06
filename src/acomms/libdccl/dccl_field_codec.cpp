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

goby::acomms::DCCLFieldCodec::MessagePart goby::acomms::DCCLFieldCodec::part_ =
    goby::acomms::DCCLFieldCodec::BODY;
boost::signal<void (unsigned size)> goby::acomms::DCCLFieldCodec::get_more_bits;
std::vector<const google::protobuf::FieldDescriptor*> goby::acomms::DCCLFieldCodec::MessageHandler::field_;
std::vector<const google::protobuf::Descriptor*> goby::acomms::DCCLFieldCodec::MessageHandler::desc_;

//
// DCCLFieldCodec public
//
            
void goby::acomms::DCCLFieldCodec::encode(Bitset* bits,
                                          const boost::any& field_value,
                                          MessagePart part)
{
    part_ = part;    

    // MessageHandler msg_handler;
    // try
    // {
    //     const google::protobuf::Message* msg = boost::any_cast<const google::protobuf::Message*>(field_value);
    //     msg_handler.push(msg->GetDescriptor());

    //     if(DCCLCodec::log_)
    //         *DCCLCodec::log_ << debug << "Starting encode for root message (in header = "
    //                          << std::boolalpha
    //                          << (part_ == HEAD) << "): "
    //                          << msg->GetDescriptor()->full_name() << std::endl;

    // }
    // catch(boost::bad_any_cast& e)
    // {
    //     if(DCCLCodec::log_)
    //         *DCCLCodec::log_ << warn << "Initial message must be custom message" << std::endl;
    // }
    encode(bits, field_value, 0);
}

void goby::acomms::DCCLFieldCodec::encode(Bitset* bits,
                                          const boost::any& field_value,
                                          const google::protobuf::FieldDescriptor* field)
{
    MessageHandler msg_handler(field);
    Bitset new_bits = _encode(field_value);
    __encode_prepend_bits(new_bits, bits);
}

void goby::acomms::DCCLFieldCodec::encode_repeated(Bitset* bits,
                                                   const std::vector<boost::any>& field_values,
                                                   const google::protobuf::FieldDescriptor* field)
{
    MessageHandler msg_handler(field);
    Bitset new_bits = _encode_repeated(field_values);
    __encode_prepend_bits(new_bits, bits);
}


            
unsigned goby::acomms::DCCLFieldCodec::size(const google::protobuf::Message& msg,
                                        MessagePart part)
{
    part_ = part;
    MessageHandler msg_handler;
    msg_handler.push(msg.GetDescriptor());
    
    return size(&msg, 0);
}

unsigned goby::acomms::DCCLFieldCodec::size(const boost::any& field_value,
                                        const google::protobuf::FieldDescriptor* field)
{
    MessageHandler msg_handler(field);
    return _size(field_value);
}

unsigned goby::acomms::DCCLFieldCodec::size_repeated(const std::vector<boost::any>& field_values,
                                                     const google::protobuf::FieldDescriptor* field)
{
    MessageHandler msg_handler(field);
    return _size_repeated(field_values);
}




void goby::acomms::DCCLFieldCodec::decode(Bitset* bits,
                                          boost::any* field_value,
                                          MessagePart part)
{
    part_ = part;

    // MessageHandler msg_handler;
    // try
    // {
    //     boost::shared_ptr<google::protobuf::Message> msg =
    //         boost::any_cast<boost::shared_ptr<google::protobuf::Message> >(*field_value);
    //     msg_handler.push(msg->GetDescriptor());

    //     if(DCCLCodec::log_)
    //         *DCCLCodec::log_ << debug << "Starting decode for root message (in header = "
    //                          << std::boolalpha
    //                          << (part_ == HEAD) << "): "
    //                          << msg->GetDescriptor()->full_name() << std::endl;
        
    // }
    // catch(boost::bad_any_cast& e)
    // {
    //     if(DCCLCodec::log_)
    //         *DCCLCodec::log_ << warn << "Initial message must be custom message" << std::endl;
    // }    

    //boost::any a(msg);
    decode(bits, field_value, 0);
    // if(!a.empty())
    //     msg->MergeFrom(*boost::any_cast<boost::shared_ptr<google::protobuf::Message> >(a));
}


void goby::acomms::DCCLFieldCodec::decode(Bitset* bits,
                                          boost::any* field_value,
                                          const google::protobuf::FieldDescriptor* field)
{
    MessageHandler msg_handler(field);
    
    if(!field_value)
        throw(DCCLException("Decode called with NULL boost::any"));
    else if(!bits)
        throw(DCCLException("Decode called with NULL Bitset"));    
    
    if(DCCLCodec::log_ && field)
        *DCCLCodec::log_ << debug << "Starting decode for field: " << field->DebugString();

    Bitset these_bits;
    BitsHandler bits_handler(&these_bits, bits);
    bits_handler.transfer_bits(min_size(field));
    
    
    if(DCCLCodec::log_)
        *DCCLCodec::log_ << "using these bits: " << these_bits << std::endl;
    
    *field_value = _decode(&these_bits);    
}

void goby::acomms::DCCLFieldCodec::decode_repeated(Bitset* bits,
                     std::vector<boost::any>* field_values,
                     const google::protobuf::FieldDescriptor* field)
{
    MessageHandler msg_handler(field);
    
    if(!field_values)
        throw(DCCLException("Decode called with NULL field_values"));
    else if(!bits)
        throw(DCCLException("Decode called with NULL Bitset"));    
    
    if(DCCLCodec::log_ && field)
        *DCCLCodec::log_ << debug << "Starting repeated decode for field: " << field->DebugString();
    
    Bitset these_bits;
    BitsHandler bits_handler(&these_bits, bits);
    bits_handler.transfer_bits(min_size(field));    
    
    if(DCCLCodec::log_)
        *DCCLCodec::log_ << "using these bits: " << these_bits << std::endl;
    
    *field_values = _decode_repeated(&these_bits);
}


unsigned goby::acomms::DCCLFieldCodec::max_size(const google::protobuf::Descriptor* desc,
                                                MessagePart part)
{
    part_ = part;

    MessageHandler msg_handler;
    if(desc)
        msg_handler.push(desc);
    else
        throw(DCCLException("Max Size called with NULL Descriptor"));
    
    return max_size(static_cast<google::protobuf::FieldDescriptor*>(0));
}

unsigned goby::acomms::DCCLFieldCodec::max_size(const google::protobuf::FieldDescriptor* field)
{
    MessageHandler msg_handler(field);
    
    if(this_field())
        return this_field()->is_repeated() ? _max_size_repeated() : _max_size();
    else
        return _max_size();
}


            
unsigned goby::acomms::DCCLFieldCodec::min_size(const google::protobuf::Descriptor* desc,
                                                MessagePart part)
{
    part_ = part;

    MessageHandler msg_handler;
    if(desc)
        msg_handler.push(desc);
    else
        throw(DCCLException("Min Size called with NULL Descriptor"));

    return min_size(static_cast<google::protobuf::FieldDescriptor*>(0));
}

unsigned goby::acomms::DCCLFieldCodec::min_size(const google::protobuf::FieldDescriptor* field)
    
{
    MessageHandler msg_handler(field);
    
    if(this_field())
        return this_field()->is_repeated() ? _min_size_repeated() : _min_size();
    else
        return _min_size();
}

            
void goby::acomms::DCCLFieldCodec::validate(const google::protobuf::Descriptor* desc,
                                            MessagePart part)
{
    part_ = part;

    MessageHandler msg_handler;
    if(desc)
        msg_handler.push(desc);
    else
        throw(DCCLException("Validate called with NULL Descriptor"));

    validate(static_cast<google::protobuf::FieldDescriptor*>(0));
}


void goby::acomms::DCCLFieldCodec::validate(const google::protobuf::FieldDescriptor* field)
{
    MessageHandler msg_handler(field);

    if(field && (field->options().GetExtension(dccl::in_head) && _variable_size()))
        throw(DCCLException("Variable size codec used in header - header fields must be encoded with fixed size codec."));
    
    _validate();
}
            
void goby::acomms::DCCLFieldCodec::info(const google::protobuf::Descriptor* desc, std::ostream* os, MessagePart part)
{
    part_ = part;

    MessageHandler msg_handler;
    if(desc)
        msg_handler.push(desc);
    else
        throw(DCCLException("info called with NULL Descriptor"));

    info(static_cast<google::protobuf::FieldDescriptor*>(0), os);
}


void goby::acomms::DCCLFieldCodec::info(const google::protobuf::FieldDescriptor* field,
                                        std::ostream* os)
{
    MessageHandler msg_handler(field);

    std::string indent = "  ";

    std::string s;

    if(this_field())
        s = this_field()->DebugString();
    else
        indent = "";

    std::string specific_info = _info();

    if(!specific_info.empty())
        s += boost::trim_copy(_info()) + "\n";

    if(_variable_size())
    {
        unsigned max_sz = max_size(field);
        unsigned min_sz = min_size(field);
        if(max_sz != min_sz)
        {
            s += ":: min size = " + goby::util::as<std::string>(min_sz) + " bit(s)\n"
                + ":: max size = " + goby::util::as<std::string>(max_sz) + " bit(s)";
        }
        else
        {
            s += ":: size = " + goby::util::as<std::string>(max_sz) + " bit(s)";
        }
    }
    else
    {
        unsigned sz = max_size(field);
        s += ":: size = " + goby::util::as<std::string>(sz) + " bit(s)";
    }
    
    
    boost::trim(s);
    boost::replace_all(s, "\n", "\n" + indent);
    s = indent + s;

    *os << s << "\n";
}


//
// DCCLFieldCodec protected
//

std::string goby::acomms::DCCLFieldCodec::_info()
{
    return std::string();
}


void goby::acomms::DCCLFieldCodec::BitsHandler::transfer_bits(unsigned size)
{
    // if(DCCLCodec::log_)
    // {
    //     *DCCLCodec::log_ << debug << "_get_bits from (" << in_pool_ << ") " << *in_pool_ << " to add to (" << out_pool_ << ") " << *out_pool_ << " number: " << size << std::endl;
    // }
    
    for(int i = 0, n = size; i < n; ++i)
        out_pool_->push_back((*in_pool_)[i]);

    *in_pool_ >>= size;
    in_pool_->resize(in_pool_->size()-size);
}


goby::acomms::Bitset
goby::acomms::DCCLFieldCodec::_encode_repeated(const std::vector<boost::any>& field_values)
{
    Bitset out_bits;
    // out_bits = [field_values[2]][field_values[1]][field_values[0]]
    for(unsigned i = 0, n = this_field()->options().GetExtension(dccl::max_repeat); i < n; ++i)
    {
        Bitset new_bits;
        if(i < field_values.size())
            new_bits = _encode(field_values[i]);
        else
            new_bits = _encode(boost::any());
        __encode_prepend_bits(new_bits, &out_bits);
    }    
    return out_bits;
    
}

std::vector<boost::any>
goby::acomms::DCCLFieldCodec::_decode_repeated(Bitset* repeated_bits)
{
    std::vector<boost::any> out_values;
    for(int i = 0, n = this_field()->options().GetExtension(dccl::max_repeat); i < n; ++i)
    {
        Bitset these_bits;

        BitsHandler bits_handler(&these_bits, repeated_bits);
        bits_handler.transfer_bits(_min_size());

        boost::any value = _decode(&these_bits);
        out_values.push_back(value);
    }
    return out_values;
}

unsigned goby::acomms::DCCLFieldCodec::_size_repeated(const std::vector<boost::any>& field_values)
{
    unsigned out = 0;
    for(unsigned i = 0, n = this_field()->options().GetExtension(dccl::max_repeat); i < n; ++i)
    {
        if(i < field_values.size())
            out += _size(field_values[i]);
        else
            out += _size(boost::any());
    }    
    return out;
}

unsigned goby::acomms::DCCLFieldCodec::_max_size_repeated()
{    
    if(!this_field()->options().HasExtension(dccl::max_repeat))
        throw(DCCLException("Missing dccl.max_repeat option on `repeated` field"));
    else
        return _max_size() * this_field()->options().GetExtension(dccl::max_repeat);
}

unsigned goby::acomms::DCCLFieldCodec::_min_size_repeated()
{    
    if(!this_field()->options().HasExtension(dccl::max_repeat))
        throw(DCCLException("Missing dccl.max_repeat option on `repeated` field"));
    else
        return _min_size() * this_field()->options().GetExtension(dccl::max_repeat);
}

//
// DCCLFieldCodec private
//


void goby::acomms::DCCLFieldCodec::__encode_prepend_bits(const Bitset& new_bits, Bitset* bits)
{
    if(DCCLCodec::log_)
        *DCCLCodec::log_ << "got these " << new_bits.size() << " bits: " << new_bits << std::endl;
    
    for(int i = 0, n = new_bits.size(); i < n; ++i)
        bits->push_back(new_bits[i]);
}



//
// DCCLFieldCodec::MessageHandler
//

void goby::acomms::DCCLFieldCodec::MessageHandler::push(const google::protobuf::Descriptor* desc)
 
{
    desc_.push_back(desc);

    ++descriptors_pushed_;
    
    // if(DCCLCodec::log_)
    //     *DCCLCodec::log_ << debug << "Added descriptor  " << desc->full_name() << std::endl;
}

void goby::acomms::DCCLFieldCodec::MessageHandler::push(const google::protobuf::FieldDescriptor* field)
{
    field_.push_back(field);
    ++fields_pushed_;
    
    // if(DCCLCodec::log_)
    //     *DCCLCodec::log_ << debug << "Added field  " << field->name() << std::endl;
}


void goby::acomms::DCCLFieldCodec::MessageHandler::__pop_desc()
{
    // if(DCCLCodec::log_)
    //     *DCCLCodec::log_ << debug << "Removed descriptor  " << desc_.back()->full_name() << std::endl;

    if(!desc_.empty())
        desc_.pop_back();
}

void goby::acomms::DCCLFieldCodec::MessageHandler::__pop_field()
{
    // if(DCCLCodec::log_)
    //     *DCCLCodec::log_ << debug << "Removed field  " << field_.back()->name() << std::endl;

    if(!field_.empty())
        field_.pop_back();
}

goby::acomms::DCCLFieldCodec::MessageHandler::MessageHandler(const google::protobuf::FieldDescriptor* field)
    : descriptors_pushed_(0),
      fields_pushed_(0)
{
    if(field)
    {
        if(field->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE)
            push(field->message_type());
        push(field);
    }
}

//
// DCCLFixedFieldCodec
//


unsigned goby::acomms::DCCLFixedFieldCodec::_size_repeated()
{    
    if(!this_field()->options().HasExtension(dccl::max_repeat))
        throw(DCCLException("Missing dccl.max_repeat option on `repeated` field"));
    else
        return _size() * this_field()->options().GetExtension(dccl::max_repeat);
}

