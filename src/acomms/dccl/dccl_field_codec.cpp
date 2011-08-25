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
#include "dccl.h"

goby::acomms::DCCLFieldCodecBase::MessagePart goby::acomms::DCCLFieldCodecBase::part_ =
    goby::acomms::DCCLFieldCodecBase::BODY;
boost::signal<void (unsigned size)> goby::acomms::DCCLFieldCodecBase::get_more_bits;
std::vector<const google::protobuf::FieldDescriptor*> goby::acomms::DCCLFieldCodecBase::MessageHandler::field_;
std::vector<const google::protobuf::Descriptor*> goby::acomms::DCCLFieldCodecBase::MessageHandler::desc_;
boost::ptr_map<int, boost::signal<void (const boost::any& field_value, const boost::any& wire_value, const boost::any& extension_value)> >   goby::acomms::DCCLFieldCodecBase::wire_value_hooks_;


using goby::glog;


//
// DCCLFieldCodecBase public
//
goby::acomms::DCCLFieldCodecBase::DCCLFieldCodecBase() { }
            
void goby::acomms::DCCLFieldCodecBase::base_encode(Bitset* bits,
                                          const boost::any& field_value,
                                          MessagePart part)
{
    part_ = part;    
    base_encode(bits, field_value, 0);
}

void goby::acomms::DCCLFieldCodecBase::base_encode(Bitset* bits,
                                          const boost::any& field_value,
                                          const google::protobuf::FieldDescriptor* field)
{
    MessageHandler msg_handler(field);

    if(field)
        glog.is(debug1) && glog << group(DCCLCodec::glog_encode_group()) <<  "Starting encode for field: " << field->DebugString() << std::flush;

    boost::any wire_value;
    base_pre_encode(&wire_value, field_value);
    
    Bitset new_bits;
    any_encode(&new_bits, wire_value);
    __encode_prepend_bits(new_bits, bits);
}

void goby::acomms::DCCLFieldCodecBase::base_encode_repeated(Bitset* bits,
                                                   const std::vector<boost::any>& field_values,
                                                   const google::protobuf::FieldDescriptor* field)
{
    MessageHandler msg_handler(field);

    std::vector<boost::any> wire_values;
    base_pre_encode_repeated(&wire_values, field_values);
    
    Bitset new_bits;
    any_encode_repeated(&new_bits, wire_values);
    __encode_prepend_bits(new_bits, bits);
}


            
void goby::acomms::DCCLFieldCodecBase::base_size(unsigned* bit_size,
                                        const google::protobuf::Message& msg,
                                        MessagePart part)
{
    part_ = part;
    base_size(bit_size, &msg, 0);
}

void goby::acomms::DCCLFieldCodecBase::base_size(unsigned* bit_size,
                                        const boost::any& field_value,
                                        const google::protobuf::FieldDescriptor* field)
{
    MessageHandler msg_handler(field);
    *bit_size += any_size(field_value);
}

void goby::acomms::DCCLFieldCodecBase::base_run_hooks(const google::protobuf::Message& msg,
                                                      MessagePart part)
{
    part_ = part;
    bool b = false;
    base_run_hooks(&b, &msg, 0);
}

void goby::acomms::DCCLFieldCodecBase::base_run_hooks(bool* b,
                                                      const boost::any& field_value,
                                                      const google::protobuf::FieldDescriptor* field)
{
    MessageHandler msg_handler(field);
    any_run_hooks(field_value);
}


void goby::acomms::DCCLFieldCodecBase::base_size_repeated(unsigned* bit_size,
                                                  const std::vector<boost::any>& field_values,
                                                  const google::protobuf::FieldDescriptor* field)
{
    MessageHandler msg_handler(field);
    *bit_size += any_size_repeated(field_values);
}




void goby::acomms::DCCLFieldCodecBase::base_decode(Bitset* bits,
                                          boost::any* field_value,
                                          MessagePart part)
{
    part_ = part;
    base_decode(bits, field_value, 0);
}


void goby::acomms::DCCLFieldCodecBase::base_decode(Bitset* bits,
                                                   boost::any* field_value,
                                                   const google::protobuf::FieldDescriptor* field)
{
    MessageHandler msg_handler(field);
    
    if(!field_value)
        throw(DCCLException("Decode called with NULL boost::any"));
    else if(!bits)
        throw(DCCLException("Decode called with NULL Bitset"));    
    
    if(field)
        glog.is(debug1) && glog << group(DCCLCodec::glog_decode_group()) <<  "Starting decode for field: " << field->DebugString() << std::flush;

    Bitset these_bits;
    BitsHandler bits_handler(&these_bits, bits);

    unsigned bits_to_transfer = 0;
    base_min_size(&bits_to_transfer, field);
    bits_handler.transfer_bits(bits_to_transfer);
    
    glog.is(debug2) && glog  << group(DCCLCodec::glog_decode_group()) << "... using these bits: " << these_bits << std::endl;

    boost::any wire_value = *field_value;
    
    any_decode(&these_bits, &wire_value);
    
    base_post_decode(wire_value, field_value);  
}

void goby::acomms::DCCLFieldCodecBase::base_decode_repeated(Bitset* bits,
                                                            std::vector<boost::any>* field_values,
                                                            const google::protobuf::FieldDescriptor* field)
{
    MessageHandler msg_handler(field);
    
    if(!field_values)
        throw(DCCLException("Decode called with NULL field_values"));
    else if(!bits)
        throw(DCCLException("Decode called with NULL Bitset"));    
    
    if(field)
        glog.is(debug1) && glog  << group(DCCLCodec::glog_decode_group()) <<  "Starting repeated decode for field: " << field->DebugString();
    
    Bitset these_bits;
    BitsHandler bits_handler(&these_bits, bits);
    
    unsigned bits_to_transfer = 0;
    base_min_size(&bits_to_transfer, field);
    bits_handler.transfer_bits(bits_to_transfer);
    
    
    glog.is(debug2) && glog  << group(DCCLCodec::glog_decode_group()) << "using these " <<
        these_bits.size() << " bits: " << these_bits << std::endl;

    std::vector<boost::any> wire_values = *field_values;
    any_decode_repeated(&these_bits, &wire_values);
    
    base_post_decode_repeated(wire_values, field_values);
}


void goby::acomms::DCCLFieldCodecBase::base_max_size(unsigned* bit_size,
                                                         const google::protobuf::Descriptor* desc,
                                                         MessagePart part)
{
    part_ = part;

    MessageHandler msg_handler;
    if(desc)
        msg_handler.push(desc);
    else
        throw(DCCLException("Max Size called with NULL Descriptor"));
    
    base_max_size(bit_size, static_cast<google::protobuf::FieldDescriptor*>(0));
}

void goby::acomms::DCCLFieldCodecBase::base_max_size(unsigned* bit_size,
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
                                                     MessagePart part)
{
    part_ = part;

    MessageHandler msg_handler;
    if(desc)
        msg_handler.push(desc);
    else
        throw(DCCLException("Min Size called with NULL Descriptor"));

    base_min_size(bit_size, static_cast<google::protobuf::FieldDescriptor*>(0));
}

void goby::acomms::DCCLFieldCodecBase::base_min_size(unsigned* bit_size,
                                                         const google::protobuf::FieldDescriptor* field)
    
{
    MessageHandler msg_handler(field);
    
    if(this_field())
        *bit_size += this_field()->is_repeated() ? min_size_repeated() : min_size();
    else
        *bit_size += min_size();
}

            
void goby::acomms::DCCLFieldCodecBase::base_validate(const google::protobuf::Descriptor* desc,
                                                     MessagePart part)
{
    part_ = part;

    MessageHandler msg_handler;
    if(desc)
        msg_handler.push(desc);
    else
        throw(DCCLException("Validate called with NULL Descriptor"));

    bool b = false;
    base_validate(&b, static_cast<google::protobuf::FieldDescriptor*>(0));
}


void goby::acomms::DCCLFieldCodecBase::base_validate(bool* b,
                                                     const google::protobuf::FieldDescriptor* field)
{
    MessageHandler msg_handler(field);

    if(field && dccl_field_options().in_head() && variable_size())
        throw(DCCLException("Variable size codec used in header - header fields must be encoded with fixed size codec."));
    
    validate();
}
            
void goby::acomms::DCCLFieldCodecBase::base_info(std::ostream* os, const google::protobuf::Descriptor* desc, MessagePart part)
{
    part_ = part;

    MessageHandler msg_handler;
    if(desc)
        msg_handler.push(desc);
    else
        throw(DCCLException("info called with NULL Descriptor"));

    base_info(os, static_cast<google::protobuf::FieldDescriptor*>(0));
}


void goby::acomms::DCCLFieldCodecBase::base_info(std::ostream* os,
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
        base_max_size(&max_sz, field);
        base_min_size(&min_sz, field);
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
        base_max_size(&sz, field);
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


void goby::acomms::DCCLFieldCodecBase::BitsHandler::transfer_bits(unsigned size)
{
    glog.is(debug3) && glog  <<  "_get_bits from (" << in_pool_ << ") " << *in_pool_ << " to add to (" << out_pool_ << ") " << *out_pool_ << " number: " << size << std::endl;
    
    if(lsb_first_)
    {
        // grab lowest bits first
        for(int i = 0, n = size; i < n; ++i)
            out_pool_->push_back((*in_pool_)[i]);
        *in_pool_ >>= size;
    }
    else
    {
        // grab highest bits first
        out_pool_->resize(out_pool_->size() + size);
        *out_pool_ <<= size;
        for(int i = 0, n = size; i < n; ++i)
        {
            (*out_pool_)[size-i-1] = (*in_pool_)[in_pool_->size()-i-1];
        }
        
    }
    in_pool_->resize(in_pool_->size()-size);
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
        __encode_prepend_bits(new_bits, bits);
    }
}


void goby::acomms::DCCLFieldCodecBase::any_decode_repeated(Bitset* repeated_bits, std::vector<boost::any>* wire_values)
{
    for(unsigned i = 0, n = dccl_field_options().max_repeat(); i < n; ++i)
    {
        Bitset these_bits;
        
        BitsHandler bits_handler(&these_bits, repeated_bits);
        bits_handler.transfer_bits(min_size());

        boost::any value;
        
        if(wire_values->size() > i)
            value = (*wire_values)[i];
        
        any_decode(&these_bits, &value);
        wire_values->push_back(value);
    }
}

unsigned goby::acomms::DCCLFieldCodecBase::any_size_repeated(const std::vector<boost::any>& field_values)
{
    unsigned out = 0;
    for(unsigned i = 0, n = dccl_field_options().max_repeat(); i < n; ++i)
    {
        if(i < field_values.size())
            out += any_size(field_values[i]);
        else
            out += any_size(boost::any());
    }    
    return out;
}

void goby::acomms::DCCLFieldCodecBase::any_run_hooks(const boost::any& field_value)   
{
    if(this_field())
        glog.is(debug1) && glog  << "running hooks for " << this_field()->DebugString() << std::endl;
    else
        glog.is(debug1) && glog  << "running hooks for base message" << std::endl;


    typedef boost::ptr_map<int, boost::signal<void (const boost::any& field_value,
                                                    const boost::any& wire_value,
                                                    const boost::any& extension_value)> > hook_map;

    for(hook_map::const_iterator i = wire_value_hooks_.begin(), e = wire_value_hooks_.end();
        i != e;
        ++i )
    {
        
        const google::protobuf::FieldDescriptor * extension_desc = this_field()->options().GetReflection()->FindKnownExtensionByNumber(i->first);
        
        boost::shared_ptr<FromProtoCppTypeBase> helper =
            DCCLTypeHelper::find(extension_desc);

        boost::any extension_value = helper->get_value(extension_desc, this_field()->options());

        if(!(extension_value.empty() || field_value.empty()))
        {
            try
            {
                boost::any wire_value;
                base_pre_encode(&wire_value, field_value);
                
                i->second->operator()(field_value, wire_value, extension_value);   
                glog.is(debug2) && glog  << "Found : " << i->first << ": " << extension_desc->DebugString() << std::endl;
            }
            
            catch(std::exception& e)
            {
                glog.is(debug1) && glog  << warn << "failed to run hook for " << i->first << ", exception: " << e.what() << std::endl;
            }
        }
    }
}            



unsigned goby::acomms::DCCLFieldCodecBase::max_size_repeated()
{    
    if(!dccl_field_options().has_max_repeat())
        throw(DCCLException("Missing (goby.field).dccl.max_repeat option on `repeated` field"));
    else
        return max_size() * dccl_field_options().max_repeat();
}

unsigned goby::acomms::DCCLFieldCodecBase::min_size_repeated()
{    
    if(!dccl_field_options().has_max_repeat())
        throw(DCCLException("Missing (goby.field).dccl.max_repeat option on `repeated` field"));
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



//
// DCCLFieldCodecBase::MessageHandler
//

void goby::acomms::DCCLFieldCodecBase::MessageHandler::push(const google::protobuf::Descriptor* desc)
 
{
    desc_.push_back(desc);
    ++descriptors_pushed_;
}

void goby::acomms::DCCLFieldCodecBase::MessageHandler::push(const google::protobuf::FieldDescriptor* field)
{
    field_.push_back(field);
    ++fields_pushed_;
}


void goby::acomms::DCCLFieldCodecBase::MessageHandler::__pop_desc()
{
    if(!desc_.empty())
        desc_.pop_back();
}

void goby::acomms::DCCLFieldCodecBase::MessageHandler::__pop_field()
{
    if(!field_.empty())
        field_.pop_back();
}

goby::acomms::DCCLFieldCodecBase::MessageHandler::MessageHandler(const google::protobuf::FieldDescriptor* field)
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

