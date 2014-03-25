// Copyright 2009-2014 Toby Schneider (https://launchpad.net/~tes)
//                     GobySoft, LLC (2013-)
//                     Massachusetts Institute of Technology (2007-2014)
//                     Goby Developers Team (https://launchpad.net/~goby-dev)
// 
//
// This file is part of the Goby Underwater Autonomy Project Libraries
// ("The Goby Libraries").
//
// The Goby Libraries are free software: you can redistribute them and/or modify
// them under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 2.1 of the License, or
// (at your option) any later version.
//
// The Goby Libraries are distributed in the hope that they will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with Goby.  If not, see <http://www.gnu.org/licenses/>.



#include <boost/foreach.hpp>

#include "dccl_field_codec.h"
#include "dccl_exception.h"
#include "dccl.h"

goby::acomms::MessageHandler::MessagePart goby::acomms::DCCLFieldCodecBase::part_ =
    goby::acomms::MessageHandler::BODY;

const google::protobuf::Message* goby::acomms::DCCLFieldCodecBase::root_message_ = 0;


using goby::glog;
using namespace goby::common::logger;

//
// DCCLFieldCodecBase public
//
goby::acomms::DCCLFieldCodecBase::DCCLFieldCodecBase() { }
            
void goby::acomms::DCCLFieldCodecBase::base_encode(Bitset* bits,
                                          const google::protobuf::Message& field_value,
                                          MessageHandler::MessagePart part)
{
    part_ = part;    
    root_message_ = &field_value;

    // we pass this through the FromProtoCppTypeBase to do dynamic_cast (RTII) for
    // custom message codecs so that these codecs can be written in the derived class (not google::protobuf::Message)
    field_encode(bits,
                 DCCLTypeHelper::find(field_value.GetDescriptor())->get_value(field_value),
                 0);
}

void goby::acomms::DCCLFieldCodecBase::field_encode(Bitset* bits,
                                          const boost::any& field_value,
                                          const google::protobuf::FieldDescriptor* field)
{
    MessageHandler msg_handler(field);

    if(field)
        glog.is(DEBUG2) && glog << group(DCCLCodec::glog_encode_group()) <<  "Starting encode for field: " << field->DebugString() << std::flush;

    boost::any wire_value;
    field_pre_encode(&wire_value, field_value);
    
    Bitset new_bits;
    any_encode(&new_bits, wire_value);
    bits->append(new_bits);
}

void goby::acomms::DCCLFieldCodecBase::field_encode_repeated(Bitset* bits,
                                                   const std::vector<boost::any>& field_values,
                                                   const google::protobuf::FieldDescriptor* field)
{
    MessageHandler msg_handler(field);

    std::vector<boost::any> wire_values;
    field_pre_encode_repeated(&wire_values, field_values);
    
    Bitset new_bits;
    any_encode_repeated(&new_bits, wire_values);
    bits->append(new_bits);
}

            
void goby::acomms::DCCLFieldCodecBase::base_size(unsigned* bit_size,
                                        const google::protobuf::Message& msg,
                                        MessageHandler::MessagePart part)
{
    *bit_size = 0;

    root_message_ = &msg;
    part_ = part;
    field_size(bit_size, &msg, 0);
}

void goby::acomms::DCCLFieldCodecBase::field_size(unsigned* bit_size,
                                        const boost::any& field_value,
                                        const google::protobuf::FieldDescriptor* field)
{
    MessageHandler msg_handler(field);

    boost::any wire_value;
    field_pre_encode(&wire_value, field_value);

    *bit_size += any_size(wire_value);
}


void goby::acomms::DCCLFieldCodecBase::field_size_repeated(unsigned* bit_size,
                                                  const std::vector<boost::any>& field_values,
                                                  const google::protobuf::FieldDescriptor* field)
{
    MessageHandler msg_handler(field);

    std::vector<boost::any> wire_values;
    field_pre_encode_repeated(&wire_values, field_values);

    *bit_size += any_size_repeated(wire_values);
}




void goby::acomms::DCCLFieldCodecBase::base_decode(Bitset* bits,
                                                   google::protobuf::Message* field_value,
                                                   MessageHandler::MessagePart part)
{
    part_ = part;
    root_message_ = field_value;
    boost::any value(field_value);
    field_decode(bits, &value, 0);
}


void goby::acomms::DCCLFieldCodecBase::field_decode(Bitset* bits,
                                                   boost::any* field_value,
                                                   const google::protobuf::FieldDescriptor* field)
{
    MessageHandler msg_handler(field);
    
    if(!field_value)
        throw(DCCLException("Decode called with NULL boost::any"));
    else if(!bits)
        throw(DCCLException("Decode called with NULL Bitset"));    
    
    if(field)
        glog.is(DEBUG2) && glog << group(DCCLCodec::glog_decode_group()) <<  "Starting decode for field: " << field->DebugString() << std::flush;
    
    glog.is(DEBUG3) && glog << group(DCCLCodec::glog_decode_group()) <<  "Message thus far is: " << root_message()->DebugString() << std::flush;
    
    Bitset these_bits(bits);

    unsigned bits_to_transfer = 0;
    field_min_size(&bits_to_transfer, field);
    these_bits.get_more_bits(bits_to_transfer);    
    
    glog.is(DEBUG2) && glog  << group(DCCLCodec::glog_decode_group()) << "... using these bits: " << these_bits << std::endl;

    boost::any wire_value = *field_value;
    
    any_decode(&these_bits, &wire_value);
    
    field_post_decode(wire_value, field_value);  
}

void goby::acomms::DCCLFieldCodecBase::field_decode_repeated(Bitset* bits,
                                                            std::vector<boost::any>* field_values,
                                                            const google::protobuf::FieldDescriptor* field)
{
    MessageHandler msg_handler(field);
    
    if(!field_values)
        throw(DCCLException("Decode called with NULL field_values"));
    else if(!bits)
        throw(DCCLException("Decode called with NULL Bitset"));    
    
    if(field)
        glog.is(DEBUG2) && glog  << group(DCCLCodec::glog_decode_group()) <<  "Starting repeated decode for field: " << field->DebugString();
    
    Bitset these_bits(bits);
    
    unsigned bits_to_transfer = 0;
    field_min_size(&bits_to_transfer, field);
    these_bits.get_more_bits(bits_to_transfer);
    
    glog.is(DEBUG2) && glog  << group(DCCLCodec::glog_decode_group()) << "using these " <<
        these_bits.size() << " bits: " << these_bits << std::endl;

    std::vector<boost::any> wire_values = *field_values;
    any_decode_repeated(&these_bits, &wire_values);
    
    field_post_decode_repeated(wire_values, field_values);
}


void goby::acomms::DCCLFieldCodecBase::base_max_size(unsigned* bit_size,
                                                         const google::protobuf::Descriptor* desc,
                                                         MessageHandler::MessagePart part)
{
    *bit_size = 0;

    part_ = part;

    MessageHandler msg_handler;
    if(desc)
        msg_handler.push(desc);
    else
        throw(DCCLException("Max Size called with NULL Descriptor"));
    
    field_max_size(bit_size, static_cast<google::protobuf::FieldDescriptor*>(0));
}

void goby::acomms::DCCLFieldCodecBase::field_max_size(unsigned* bit_size,
                                                         const google::protobuf::FieldDescriptor* field)
{
    MessageHandler msg_handler(field);
    
    if(this_field())
        *bit_size += this_field()->is_repeated() ? max_size_repeated() : max_size();
    else
        *bit_size += max_size();
}


            
void goby::acomms::DCCLFieldCodecBase::base_min_size(unsigned* bit_size,
                                                     const google::protobuf::Descriptor* desc,
                                                     MessageHandler::MessagePart part)
{
    *bit_size = 0;

    part_ = part;

    MessageHandler msg_handler;
    if(desc)
        msg_handler.push(desc);
    else
        throw(DCCLException("Min Size called with NULL Descriptor"));

    field_min_size(bit_size, static_cast<google::protobuf::FieldDescriptor*>(0));
}

void goby::acomms::DCCLFieldCodecBase::field_min_size(unsigned* bit_size,
                                                         const google::protobuf::FieldDescriptor* field)
    
{
    MessageHandler msg_handler(field);
    
    if(this_field())
        *bit_size += this_field()->is_repeated() ? min_size_repeated() : min_size();
    else
        *bit_size += min_size();
}

            
void goby::acomms::DCCLFieldCodecBase::base_validate(const google::protobuf::Descriptor* desc,
                                                     MessageHandler::MessagePart part)
{
    part_ = part;

    MessageHandler msg_handler;
    if(desc)
        msg_handler.push(desc);
    else
        throw(DCCLException("Validate called with NULL Descriptor"));

    bool b = false;
    field_validate(&b, static_cast<google::protobuf::FieldDescriptor*>(0));
}


void goby::acomms::DCCLFieldCodecBase::field_validate(bool* b,
                                                     const google::protobuf::FieldDescriptor* field)
{
    MessageHandler msg_handler(field);

    if(field && dccl_field_options().in_head() && variable_size())
        throw(DCCLException("Variable size codec used in header - header fields must be encoded with fixed size codec."));
    
    validate();
}
            
void goby::acomms::DCCLFieldCodecBase::base_info(std::ostream* os, const google::protobuf::Descriptor* desc, MessageHandler::MessagePart part)
{
    part_ = part;

    MessageHandler msg_handler;
    if(desc)
        msg_handler.push(desc);
    else
        throw(DCCLException("info called with NULL Descriptor"));

    field_info(os, static_cast<google::protobuf::FieldDescriptor*>(0));
}


void goby::acomms::DCCLFieldCodecBase::field_info(std::ostream* os,
                                                 const google::protobuf::FieldDescriptor* field)
{
    MessageHandler msg_handler(field);

    std::string indent = " ";

    std::string s;
    
    if(this_field())
    {
        s += this_field()->DebugString();
    }
    else
    {
        s += this_descriptor()->full_name() + "\n";
        indent = "";
    }
    
    bool is_zero_size = false;
    
    std::string specific_info = info();

    if(!specific_info.empty())
        s += specific_info;

    if(variable_size())
    {
        unsigned max_sz = 0, min_sz = 0;
        field_max_size(&max_sz, field);
        field_min_size(&min_sz, field);
        if(max_sz != min_sz)
        {
            s += ":: min size = " + goby::util::as<std::string>(min_sz) + " bit(s)\n"
                + ":: max size = " + goby::util::as<std::string>(max_sz) + " bit(s)";
        }
        else
        {
            if(!max_sz) is_zero_size = true;
            s += ":: size = " + goby::util::as<std::string>(max_sz) + " bit(s)";
        }
    }
    else
    {
        unsigned sz = 0;
        field_max_size(&sz, field);
        if(!sz) is_zero_size = true;
        s += ":: size = " + goby::util::as<std::string>(sz) + " bit(s)";
    }

    boost::replace_all(s, "\n", "\n" + indent);    
    s = indent + s;

    if(!is_zero_size)
        *os << s << "\n";
}


//
// DCCLFieldCodecBase protected
//

std::string goby::acomms::DCCLFieldCodecBase::info()
{
    return std::string();
}

void goby::acomms::DCCLFieldCodecBase::any_encode_repeated(goby::acomms::Bitset* bits, const std::vector<boost::any>& wire_values)
{
    // out_bits = [field_values[2]][field_values[1]][field_values[0]]
    for(unsigned i = 0, n = dccl_field_options().max_repeat(); i < n; ++i)
    {
        Bitset new_bits;
        if(i < wire_values.size())
            any_encode(&new_bits, wire_values[i]);
        else
            any_encode(&new_bits, boost::any());
        bits->append(new_bits);
        
    }
}


void goby::acomms::DCCLFieldCodecBase::any_decode_repeated(Bitset* repeated_bits, std::vector<boost::any>* wire_values)
{
    for(unsigned i = 0, n = dccl_field_options().max_repeat(); i < n; ++i)
    {
        Bitset these_bits(repeated_bits);        
        these_bits.get_more_bits(min_size());
        
        boost::any value;
        
        if(wire_values->size() > i)
            value = (*wire_values)[i];
        
        any_decode(&these_bits, &value);
        wire_values->push_back(value);
    }
}

unsigned goby::acomms::DCCLFieldCodecBase::any_size_repeated(const std::vector<boost::any>& wire_values)
{
    unsigned out = 0;
    for(unsigned i = 0, n = dccl_field_options().max_repeat(); i < n; ++i)
    {
        if(i < wire_values.size())
            out += any_size(wire_values[i]);
        else
            out += any_size(boost::any());
    }    
    return out;
}

unsigned goby::acomms::DCCLFieldCodecBase::max_size_repeated()
{    
    if(!dccl_field_options().has_max_repeat())
        throw(DCCLException("Missing (goby.field).dccl.max_repeat option on `repeated` field: " + this_field()->DebugString()));
    else
        return max_size() * dccl_field_options().max_repeat();
}

unsigned goby::acomms::DCCLFieldCodecBase::min_size_repeated()
{    
    if(!dccl_field_options().has_max_repeat())
        throw(DCCLException("Missing (goby.field).dccl.max_repeat option on `repeated` field " + this_field()->DebugString()));
    else
        return min_size() * dccl_field_options().max_repeat();
}

void goby::acomms::DCCLFieldCodecBase::any_pre_encode_repeated(std::vector<boost::any>* wire_values, const std::vector<boost::any>& field_values)
{
    BOOST_FOREACH(const boost::any& field_value, field_values)
    {
        boost::any wire_value;
        any_pre_encode(&wire_value, field_value);
        wire_values->push_back(wire_value);
    }
    
}
void goby::acomms::DCCLFieldCodecBase::any_post_decode_repeated(
    const std::vector<boost::any>& wire_values, std::vector<boost::any>* field_values)
{
    BOOST_FOREACH(const boost::any& wire_value, wire_values)
    {
        boost::any field_value;
        any_post_decode(wire_value, &field_value);
        field_values->push_back(field_value);
    }
}


//
// DCCLFieldCodecBase private
//

