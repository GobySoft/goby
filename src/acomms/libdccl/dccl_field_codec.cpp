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
boost::ptr_map<goby::acomms::protobuf::HookKey, boost::signal<void (const boost::any& wire_value, const boost::any& extension_value)> >   goby::acomms::DCCLFieldCodecBase::wire_value_hooks_;


using goby::glog;


//
// DCCLFieldCodecBase public
//
            
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
    Bitset new_bits = any_encode(base_pre_encode(field_value));
    __encode_prepend_bits(new_bits, bits);
}

void goby::acomms::DCCLFieldCodecBase::base_encode_repeated(Bitset* bits,
                                                   const std::vector<boost::any>& field_values,
                                                   const google::protobuf::FieldDescriptor* field)
{
    MessageHandler msg_handler(field);
    Bitset new_bits = any_encode_repeated(base_pre_encode_repeated(field_values));
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
    // if(field)
    //     glog.is(debug1) && glog  << "field: " << field->DebugString() << "size: " << any_size(field_value) << std::endl;    
    *bit_size += any_size(field_value);
}

void goby::acomms::DCCLFieldCodecBase::base_run_hooks(const google::protobuf::Message& msg,
                                                      MessagePart part)
{
    part_ = part;
    base_run_hooks(&msg, 0);
}

void goby::acomms::DCCLFieldCodecBase::base_run_hooks(const boost::any& field_value,
                                                      const google::protobuf::FieldDescriptor* field)
{
    MessageHandler msg_handler(field);

    if(field)
        glog.is(debug1) && glog  << "field: " << field->DebugString() << std::endl;

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

    // MessageHandler msg_handler;
    // try
    // {
    //     boost::shared_ptr<google::protobuf::Message> msg =
    //         boost::any_cast<boost::shared_ptr<google::protobuf::Message> >(*field_value);
    //     msg_handler.push(msg->GetDescriptor());

    //         glog.is(debug1) && glog  <<  "Starting decode for root message (in header = "
    //                          << std::boolalpha
    //                          << (part_ == HEAD) << "): "
    //                          << msg->GetDescriptor()->full_name() << std::endl;
        
    // }
    // catch(boost::bad_any_cast& e)
    // {
    //         glog.is(debug1) && glog  << warn << "Initial message must be custom message" << std::endl;
    // }    

    //boost::any a(msg);
    base_decode(bits, field_value, 0);
    // if(!a.empty())
    //     msg->MergeFrom(*boost::any_cast<boost::shared_ptr<google::protobuf::Message> >(a));
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
        glog.is(debug1) && glog  <<  "Starting decode for field: " << field->DebugString();

    Bitset these_bits;
    BitsHandler bits_handler(&these_bits, bits);
    bits_handler.transfer_bits(base_min_size(field));
    
    
    glog.is(debug1) && glog  << "using these bits: " << these_bits << std::endl;
    
    *field_value = base_post_decode(any_decode(&these_bits));    
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
        glog.is(debug1) && glog  <<  "Starting repeated decode for field: " << field->DebugString();
    
    Bitset these_bits;
    BitsHandler bits_handler(&these_bits, bits);
    bits_handler.transfer_bits(base_min_size(field));    
    
    glog.is(debug1) && glog  << "using these bits: " << these_bits << std::endl;
    
    *field_values = base_post_decode_repeated(any_decode_repeated(&these_bits));
}


unsigned goby::acomms::DCCLFieldCodecBase::base_max_size(const google::protobuf::Descriptor* desc,
                                                MessagePart part)
{
    part_ = part;

    MessageHandler msg_handler;
    if(desc)
        msg_handler.push(desc);
    else
        throw(DCCLException("Max Size called with NULL Descriptor"));
    
    return base_max_size(static_cast<google::protobuf::FieldDescriptor*>(0));
}

unsigned goby::acomms::DCCLFieldCodecBase::base_max_size(const google::protobuf::FieldDescriptor* field)
{
    MessageHandler msg_handler(field);
    
    if(this_field())
        return this_field()->is_repeated() ? max_size_repeated() : max_size();
    else
        return max_size();
}


            
unsigned goby::acomms::DCCLFieldCodecBase::base_min_size(const google::protobuf::Descriptor* desc,
                                                MessagePart part)
{
    part_ = part;

    MessageHandler msg_handler;
    if(desc)
        msg_handler.push(desc);
    else
        throw(DCCLException("Min Size called with NULL Descriptor"));

    return base_min_size(static_cast<google::protobuf::FieldDescriptor*>(0));
}

unsigned goby::acomms::DCCLFieldCodecBase::base_min_size(const google::protobuf::FieldDescriptor* field)
    
{
    MessageHandler msg_handler(field);
    
    if(this_field())
        return this_field()->is_repeated() ? min_size_repeated() : min_size();
    else
        return min_size();
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

    base_validate(static_cast<google::protobuf::FieldDescriptor*>(0));
}


void goby::acomms::DCCLFieldCodecBase::base_validate(const google::protobuf::FieldDescriptor* field)
{
    MessageHandler msg_handler(field);

    if(field && (field->options().GetExtension(dccl::in_head) && variable_size()))
        throw(DCCLException("Variable size codec used in header - header fields must be encoded with fixed size codec."));
    
    validate();
}
            
void goby::acomms::DCCLFieldCodecBase::base_info(const google::protobuf::Descriptor* desc, std::ostream* os, MessagePart part)
{
    part_ = part;

    MessageHandler msg_handler;
    if(desc)
        msg_handler.push(desc);
    else
        throw(DCCLException("info called with NULL Descriptor"));

    base_info(static_cast<google::protobuf::FieldDescriptor*>(0), os);
}


void goby::acomms::DCCLFieldCodecBase::base_info(const google::protobuf::FieldDescriptor* field,
                                        std::ostream* os)
{
    MessageHandler msg_handler(field);

    std::string indent = "  ";

    std::string s;

    if(this_field())
        s = this_field()->DebugString();
    else
        indent = "";

    std::string specific_info = info();

    if(!specific_info.empty())
        s += boost::trim_copy(info()) + "\n";

    if(variable_size())
    {
        unsigned max_sz = base_max_size(field);
        unsigned min_sz = base_min_size(field);
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
        unsigned sz = base_max_size(field);
        s += ":: size = " + goby::util::as<std::string>(sz) + " bit(s)";
    }
    
    
    boost::trim(s);
    boost::replace_all(s, "\n", "\n" + indent);
    s = indent + s;

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
    glog.is(debug2) && glog  <<  "_get_bits from (" << in_pool_ << ") " << *in_pool_ << " to add to (" << out_pool_ << ") " << *out_pool_ << " number: " << size << std::endl;
    
    for(int i = 0, n = size; i < n; ++i)
        out_pool_->push_back((*in_pool_)[i]);

    *in_pool_ >>= size;
    in_pool_->resize(in_pool_->size()-size);
}


goby::acomms::Bitset
goby::acomms::DCCLFieldCodecBase::any_encode_repeated(const std::vector<boost::any>& field_values)
{
    Bitset out_bits;
    // out_bits = [field_values[2]][field_values[1]][field_values[0]]
    for(unsigned i = 0, n = this_field()->options().GetExtension(dccl::max_repeat); i < n; ++i)
    {
        Bitset new_bits;
        if(i < field_values.size())
            new_bits = any_encode(field_values[i]);
        else
            new_bits = any_encode(boost::any());
        __encode_prepend_bits(new_bits, &out_bits);
    }    
    return out_bits;
    
}

std::vector<boost::any>
goby::acomms::DCCLFieldCodecBase::any_decode_repeated(Bitset* repeated_bits)
{
    std::vector<boost::any> out_values;
    for(int i = 0, n = this_field()->options().GetExtension(dccl::max_repeat); i < n; ++i)
    {
        Bitset these_bits;

        BitsHandler bits_handler(&these_bits, repeated_bits);
        bits_handler.transfer_bits(min_size());

        boost::any value = any_decode(&these_bits);
        out_values.push_back(value);
    }
    return out_values;
}

unsigned goby::acomms::DCCLFieldCodecBase::any_size_repeated(const std::vector<boost::any>& field_values)
{
    unsigned out = 0;
    for(unsigned i = 0, n = this_field()->options().GetExtension(dccl::max_repeat); i < n; ++i)
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


    typedef boost::ptr_map<protobuf::HookKey,
                           boost::signal<void (const boost::any& wire_value,
                                               const boost::any& extension_value)> > hook_map;

    for(hook_map::const_iterator i = wire_value_hooks_.begin(), e = wire_value_hooks_.end();
        i != e;
        ++i )
    {
        
        const google::protobuf::FieldDescriptor * extension_desc = this_field()->options().GetReflection()->FindKnownExtensionByNumber(i->first.field_option_extension_number());
        
        boost::shared_ptr<FromProtoCppTypeBase> helper =
            DCCLTypeHelper::find(extension_desc);

        boost::any extension_value = helper->get_value(extension_desc, this_field()->options());

        if(!(extension_value.empty() || field_value.empty()))
        {
            try
            {
                if(i->first.value_requested() == protobuf::HookKey::WIRE_VALUE)
                    i->second->operator()(base_pre_encode(field_value), extension_value);
                else if(i->first.value_requested() == protobuf::HookKey::FIELD_VALUE)
                    i->second->operator()(field_value, extension_value);
                
                glog.is(debug1) && glog  << "Found : " << i->first << ": " << extension_desc->DebugString() << std::endl;
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
    if(!this_field()->options().HasExtension(dccl::max_repeat))
        throw(DCCLException("Missing dccl.max_repeat option on `repeated` field"));
    else
        return max_size() * this_field()->options().GetExtension(dccl::max_repeat);
}

unsigned goby::acomms::DCCLFieldCodecBase::min_size_repeated()
{    
    if(!this_field()->options().HasExtension(dccl::max_repeat))
        throw(DCCLException("Missing dccl.max_repeat option on `repeated` field"));
    else
        return min_size() * this_field()->options().GetExtension(dccl::max_repeat);
}

std::vector<boost::any> goby::acomms::DCCLFieldCodecBase::any_pre_encode_repeated(const std::vector<boost::any>& field_values)
{
    std::vector<boost::any> return_values;
    BOOST_FOREACH(const boost::any& field_value, field_values)
        return_values.push_back(any_pre_encode(field_value));
    return return_values;
}
std::vector<boost::any> goby::acomms::DCCLFieldCodecBase::any_post_decode_repeated(
    const std::vector<boost::any>& wire_values)
{
    std::vector<boost::any> return_values;
    BOOST_FOREACH(const boost::any& wire_value, wire_values)
        return_values.push_back(any_post_decode(wire_value));
    return return_values;
}


//
// DCCLFieldCodecBase private
//


void goby::acomms::DCCLFieldCodecBase::__encode_prepend_bits(const Bitset& new_bits, Bitset* bits)
{
    glog.is(debug1) && glog  << "got these " << new_bits.size() << " bits: " << new_bits << std::endl;
    
    for(int i = 0, n = new_bits.size(); i < n; ++i)
        bits->push_back(new_bits[i]);
}



//
// DCCLFieldCodecBase::MessageHandler
//

void goby::acomms::DCCLFieldCodecBase::MessageHandler::push(const google::protobuf::Descriptor* desc)
 
{
    desc_.push_back(desc);

    ++descriptors_pushed_;
    
    glog.is(debug1) && glog  <<  "Added descriptor  " << desc->full_name() << std::endl;
}

void goby::acomms::DCCLFieldCodecBase::MessageHandler::push(const google::protobuf::FieldDescriptor* field)
{
    field_.push_back(field);
    ++fields_pushed_;
    
    //     glog.is(debug1) && glog  <<  "Added field  " << field->name() << std::endl;
}


void goby::acomms::DCCLFieldCodecBase::MessageHandler::__pop_desc()
{
    //     glog.is(debug1) && glog  <<  "Removed descriptor  " << desc_.back()->full_name() << std::endl;

    if(!desc_.empty())
        desc_.pop_back();
}

void goby::acomms::DCCLFieldCodecBase::MessageHandler::__pop_field()
{
    //     glog.is(debug1) && glog  <<  "Removed field  " << field_.back()->name() << std::endl;

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

