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
#include <boost/assign.hpp>
#include <crypto++/filters.h>
#include <crypto++/sha.h>
#include <crypto++/modes.h>
#include <crypto++/aes.h>

#include "dccl.h"
#include "dccl_field_codec_default.h"

#include "goby/util/logger.h"
#include "goby/util/string.h"
#include "goby/protobuf/acomms_proto_helpers.h"
#include "goby/protobuf/dccl_option_extensions.pb.h"
#include "goby/protobuf/header.pb.h"

using goby::util::goby_time;
using goby::util::as;
using google::protobuf::FieldDescriptor;
using google::protobuf::Descriptor;
using google::protobuf::Reflection;


const char* goby::acomms::DCCLCodec::DEFAULT_CODEC_NAME = "_default";


/////////////////////
// public methods (general use)
/////////////////////
goby::acomms::DCCLCodec::DCCLCodec(std::ostream* log /* =0 */)
    : log_(log)
{
    using namespace google::protobuf;    
    using boost::shared_ptr;
    boost::assign::insert (cpptype_helper_)
        (static_cast<FieldDescriptor::CppType>(0),
         shared_ptr<FromProtoCppTypeBase>(new FromProtoCppTypeBase))
        (FieldDescriptor::CPPTYPE_DOUBLE,
         shared_ptr<FromProtoCppTypeBase>(new FromProtoCppType<FieldDescriptor::CPPTYPE_DOUBLE>))
        (FieldDescriptor::CPPTYPE_FLOAT,
         shared_ptr<FromProtoCppTypeBase>(new FromProtoCppType<FieldDescriptor::CPPTYPE_FLOAT>))
        (FieldDescriptor::CPPTYPE_UINT64,
         shared_ptr<FromProtoCppTypeBase>(new FromProtoCppType<FieldDescriptor::CPPTYPE_UINT64>))
        (FieldDescriptor::CPPTYPE_UINT32,
         shared_ptr<FromProtoCppTypeBase>(new FromProtoCppType<FieldDescriptor::CPPTYPE_UINT32>))
        (FieldDescriptor::CPPTYPE_INT32,
         shared_ptr<FromProtoCppTypeBase>(new FromProtoCppType<FieldDescriptor::CPPTYPE_INT32>))
        (FieldDescriptor::CPPTYPE_INT64,
         shared_ptr<FromProtoCppTypeBase>(new FromProtoCppType<FieldDescriptor::CPPTYPE_INT64>))
        (FieldDescriptor::CPPTYPE_BOOL,
         shared_ptr<FromProtoCppTypeBase>(new FromProtoCppType<FieldDescriptor::CPPTYPE_BOOL>))
         (FieldDescriptor::CPPTYPE_STRING,
         shared_ptr<FromProtoCppTypeBase>(new FromProtoCppType<FieldDescriptor::CPPTYPE_STRING>))
        (FieldDescriptor::CPPTYPE_MESSAGE,
         shared_ptr<FromProtoCppTypeBase>(new FromProtoCppType<FieldDescriptor::CPPTYPE_MESSAGE>))
        (FieldDescriptor::CPPTYPE_ENUM,
         shared_ptr<FromProtoCppTypeBase>(new FromProtoCppType<FieldDescriptor::CPPTYPE_ENUM>));
    
    add_field_codec<double, DCCLDefaultFieldCodec>(DEFAULT_CODEC_NAME);
    add_field_codec<int32, DCCLDefaultFieldCodec>(DEFAULT_CODEC_NAME);    

    add_field_codec<google::protobuf::Message, DCCLDefaultFieldCodec>(DEFAULT_CODEC_NAME);
    
//    add_field_codec<FieldDescriptor::CPPTYPE_STRING, DCCLTimeCodec>("_time");
//    add_field_codec<FieldDescriptor::CPPTYPE_STRING, DCCLModemIdConverterCodec>(
    //"_platform<->modem_id");
}

void goby::acomms::DCCLCodec::add_flex_groups(util::FlexOstream* tout)
{
    tout->add_group("dccl_enc", util::Colors::lt_magenta, "encoder messages (goby_dccl)");
    tout->add_group("dccl_dec", util::Colors::lt_blue, "decoder messages (goby_dccl)");
}


bool goby::acomms::DCCLCodec::encode(std::string* bytes, const google::protobuf::Message& msg)
{
    //fixed header
    Bitset ccl_id_bits(HEAD_CCL_ID_SIZE + HEAD_DCCL_ID_SIZE, DCCL_CCL_HEADER);
    Bitset dccl_id_bits(HEAD_CCL_ID_SIZE + HEAD_DCCL_ID_SIZE, msg.GetDescriptor()->options().GetExtension(dccl::id));
    ccl_id_bits <<= HEAD_DCCL_ID_SIZE;    
    Bitset head_bits = ccl_id_bits | dccl_id_bits;
    Bitset body_bits;
    
    const Descriptor* desc = msg.GetDescriptor();
    try
    {
        DCCLFieldCodec::set_message(&msg);
        DCCLFieldCodec::set_in_header(true);
        field_codecs_[FieldDescriptor::CPPTYPE_MESSAGE]
            [desc->options().GetExtension(dccl::message_codec)]->encode(&head_bits, &msg, 0);

        DCCLFieldCodec::set_in_header(false);
        field_codecs_[FieldDescriptor::CPPTYPE_MESSAGE]
            [desc->options().GetExtension(dccl::message_codec)]->encode(&body_bits, &msg, 0);
    }
    catch(DCCLException& e)
    {
        if(log_) *log_ << warn << "Message " << desc->full_name() << " failed to encode. Reason: " << e.what() << std::endl;
        return false;
    }

    std::string head_bytes, body_bytes;
    bitset2string(head_bits, &head_bytes);
    bitset2string(body_bits, &body_bytes);
    
    if(!crypto_key_.empty())
        encrypt(&body_bytes, head_bytes);

    *bytes = head_bytes + body_bytes;

    std::cout << "head: " << head_bits << std::endl;
    std::cout << "body: " << body_bits << std::endl;    
    
    return true;
}

bool goby::acomms::DCCLCodec::decode(const std::string& bytes, google::protobuf::Message* msg)   
{

    unsigned head_size_bits = size_recursive(*msg, true) + HEAD_CCL_ID_SIZE + HEAD_DCCL_ID_SIZE;
    unsigned body_size_bits = size_recursive(*msg, false);

    unsigned head_size_bytes = ceil_bits2bytes(head_size_bits);
    unsigned body_size_bytes = ceil_bits2bytes(body_size_bits);
    
    std::cout << "head is " << head_size_bits << " bits" << std::endl;
    std::cout << "body is " << body_size_bits << " bits" << std::endl;
    std::cout << "head is " << head_size_bytes << " bytes" << std::endl;
    std::cout << "body is " << body_size_bytes << " bytes" << std::endl;

    std::string head_bytes = bytes.substr(0, head_size_bytes);
    std::string body_bytes = bytes.substr(head_size_bytes, body_size_bytes);

    std::cout << "head is " << hex_encode(head_bytes) << std::endl;
    std::cout << "body is " << hex_encode(body_bytes) << std::endl;

    if(!crypto_key_.empty())
        decrypt(&body_bytes, head_bytes);
    
    Bitset head_bits, body_bits;
    string2bitset(&head_bits, head_bytes);
    string2bitset(&body_bits, body_bytes);

    head_bits.resize(head_size_bits - HEAD_CCL_ID_SIZE + HEAD_DCCL_ID_SIZE);
    body_bits.resize(body_size_bits);
    
    
    std::cout << "head: " << head_bits << std::endl;
    std::cout << "body: " << body_bits << std::endl;    
    
    DCCLFieldCodec::set_message(msg);

    try
     {
         decode_recursive(&head_bits, msg, true);
         decode_recursive(&body_bits, msg, false);
     }
     catch(DCCLException& e)
     {
         const Descriptor* desc = msg->GetDescriptor();
         if(log_) *log_ << warn << "Message " << desc->full_name() << " failed to decode. Reason: " << e.what() << std::endl;
         return false;
     }
    return true;
}

unsigned goby::acomms::DCCLCodec::size_recursive(const google::protobuf::Message& msg,
                                                 bool is_header)
{
    unsigned sum = 0;
    const Descriptor* desc = msg.GetDescriptor();
    const Reflection* refl = msg.GetReflection();       
    for(int i = 0, n = desc->field_count(); i < n; ++i)
    {
        const FieldDescriptor* field_desc = desc->field(i);
        std::string field_codec = field_desc->options().GetExtension(dccl::codec);

        if(field_desc->options().GetExtension(dccl::omit)
           || (is_header && !field_desc->options().GetExtension(dccl::in_head))
           || (!is_header && field_desc->options().GetExtension(dccl::in_head)))
        {
            continue;
        }
        else if(field_desc->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE)
        {
            size_recursive(refl->GetMessage(msg, field_desc), is_header);
        }
        else
        {
            sum += field_codecs_[field_desc->cpp_type()][field_codec]->size(field_desc);
        }
    }
    
    return sum;
}


void goby::acomms::DCCLCodec::encode_recursive(Bitset* bits,
                                               const google::protobuf::Message& msg,
                                               bool is_header)
{
}

void goby::acomms::DCCLCodec::decode_recursive(Bitset* bits, google::protobuf::Message* msg, bool is_header)
{
    const Descriptor* desc = msg->GetDescriptor();
    const Reflection* refl = msg->GetReflection();
    
    for(int i = desc->field_count()-1, n = 0; i >= n; --i)
    {
        const FieldDescriptor* field_desc = desc->field(i);
        std::string field_codec = field_desc->options().HasExtension(dccl::codec) ?
            field_desc->options().GetExtension(dccl::codec) : DEFAULT_CODEC_NAME;
            
        if(field_desc->options().GetExtension(dccl::omit)
           || (is_header && !field_desc->options().GetExtension(dccl::in_head))
           || (!is_header && field_desc->options().GetExtension(dccl::in_head)))
        {
            continue;
        }
        else if(field_desc->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE)
        {
            if(field_desc->is_repeated())
            {
            }
            else
            {
            }
        }
        else if(field_desc->is_repeated())
        {   
            std::vector<boost::any> field_values;
            
            field_codecs_[field_desc->cpp_type()][field_codec]->decode(bits, &field_values,
                                                                       field_desc);

            for(int j = field_values.size()-1, m = 0; j >= m; --j)
                cpptype_helper_[field_desc->cpp_type()]->add_value(field_desc, msg, field_values[j]);

        }
        else
        {
            boost::any field_value;
            
            field_codecs_[field_desc->cpp_type()][field_codec]->decode(bits,
                                                                       &field_value,
                                                                       field_desc);
            cpptype_helper_[field_desc->cpp_type()]->set_value(field_desc, msg, field_value);
        }
    }    
}

// makes sure we can actual encode / decode a message of this descriptor given the loaded FieldCodecs
// checks all bounds on the message
bool goby::acomms::DCCLCodec::validate(const Descriptor* desc)
{
    try
    {
        if(!desc->options().HasExtension(dccl::id))
            throw(DCCLException("Missing message option `dccl.id`. Specify a unique id (e.g. 3) in the body of your .proto message using \"option (dccl.id) = 3\""));
        else if(!desc->options().HasExtension(dccl::max_bytes))
            throw(DCCLException("Missing message option `dccl.max_bytes`. Specify a maximum (encoded) message size in bytes (e.g. 32) in the body of your .proto message using \"option (dccl.max_bytes) = 32\"")); 
        
        validate_recursive(desc);
    }
    catch(DCCLException& e)
    {
        if(log_) *log_ << warn << "Message " << desc->full_name() << " failed validation. Reason: " << e.what() << std::endl;
        return false;
    }

    return true;
}

void goby::acomms::DCCLCodec::validate_recursive(const Descriptor* desc)
{
    for(int i = 0, n = desc->field_count(); i < n; ++i)
    {
        const FieldDescriptor* field_desc = desc->field(i);
        std::string field_codec = field_desc->options().HasExtension(dccl::codec) ?
            field_desc->options().GetExtension(dccl::codec) : DEFAULT_CODEC_NAME;

        if(field_desc->options().GetExtension(dccl::omit))
        {
            continue;
        }
        else if(field_desc->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE)
        {
            validate_recursive(field_desc->message_type());
        }
        else if(!field_codecs_[field_desc->cpp_type()].count(field_codec))
        {
            throw(DCCLException("Missing Field Codec called " + field_codec + " for CppType: " + cpptype_helper_[field_desc->cpp_type()]->as_str() + ", used by field: " + field_desc->name()));
        }
        else
        {
            field_codecs_[field_desc->cpp_type()][field_codec]->validate(field_desc);
            
        }
    }
}

void goby::acomms::DCCLCodec::encrypt(std::string* s, const std::string& nonce /* message head */)
{
    using namespace CryptoPP;

    std::string iv;
    SHA256 hash;
    StringSource unused(nonce, true, new HashFilter(hash, new StringSink(iv)));
    
    CTR_Mode<AES>::Encryption encryptor;
    encryptor.SetKeyWithIV((byte*)crypto_key_.c_str(), crypto_key_.size(), (byte*)iv.c_str());

    std::string cipher;
    StreamTransformationFilter in(encryptor, new StringSink(cipher));
    in.Put((byte*)s->c_str(), s->size());
    in.MessageEnd();
    *s = cipher;
}

void goby::acomms::DCCLCodec::decrypt(std::string* s, const std::string& nonce)
{
    using namespace CryptoPP;

    std::string iv;
    SHA256 hash;
    StringSource unused(nonce, true, new HashFilter(hash, new StringSink(iv)));
    
    CTR_Mode<AES>::Decryption decryptor;    
    decryptor.SetKeyWithIV((byte*)crypto_key_.c_str(), crypto_key_.size(), (byte*)iv.c_str());
    
    std::string recovered;
    StreamTransformationFilter out(decryptor, new StringSink(recovered));
    out.Put((byte*)s->c_str(), s->size());
    out.MessageEnd();
    *s = recovered;
}

void goby::acomms::DCCLCodec::merge_cfg(const protobuf::DCCLConfig& cfg)
{
    cfg_.MergeFrom(cfg);
    process_cfg();
}

void goby::acomms::DCCLCodec::set_cfg(const protobuf::DCCLConfig& cfg)
{
    cfg_.CopyFrom(cfg);
    process_cfg();
}

void goby::acomms::DCCLCodec::process_cfg()
{    
    using namespace CryptoPP;
    
    SHA256 hash;
    StringSource unused(cfg_.crypto_passphrase(), true, new HashFilter(hash, new StringSink(crypto_key_)));

    if(log_) *log_ << group("dccl_enc") << "cryptography enabled with given passphrase" << std::endl;
}


