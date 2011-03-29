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
std::ostream*  goby::acomms::DCCLCodec::log_ = 0;

goby::acomms::DCCLCodec::FieldCodecManager goby::acomms::DCCLCodec::codec_manager_;
goby::acomms::DCCLCodec::CppTypeHelper goby::acomms::DCCLCodec::cpptype_helper_;
goby::acomms::protobuf::DCCLConfig goby::acomms::DCCLCodec::cfg_;
std::string goby::acomms::DCCLCodec::crypto_key_;
bool goby::acomms::DCCLCodec::default_codecs_set_ = false;

//
// DCCLCodec
//

void goby::acomms::DCCLCodec::set_default_codecs()
{    
    codec_manager_.add<double, DCCLDefaultFieldCodec>(DEFAULT_CODEC_NAME);
    codec_manager_.add<float, DCCLDefaultFieldCodec>(DEFAULT_CODEC_NAME);
    codec_manager_.add<bool, DCCLDefaultFieldCodec>(DEFAULT_CODEC_NAME);
    codec_manager_.add<int32, DCCLDefaultFieldCodec>(DEFAULT_CODEC_NAME);
    codec_manager_.add<int64, DCCLDefaultFieldCodec>(DEFAULT_CODEC_NAME);
    codec_manager_.add<uint32, DCCLDefaultFieldCodec>(DEFAULT_CODEC_NAME);
    codec_manager_.add<uint64, DCCLDefaultFieldCodec>(DEFAULT_CODEC_NAME);
    codec_manager_.add<std::string, DCCLDefaultFieldCodec>(DEFAULT_CODEC_NAME);
    codec_manager_.add<google::protobuf::EnumDescriptor, DCCLDefaultFieldCodec>(DEFAULT_CODEC_NAME);
    codec_manager_.add<google::protobuf::Message, DCCLDefaultFieldCodec>(DEFAULT_CODEC_NAME);

    //    add_field_codec<FieldDescriptor::CPPTYPE_STRING, DCCLTimeCodec>("_time");
//    add_field_codec<FieldDescriptor::CPPTYPE_STRING, DCCLModemIdConverterCodec>(
    //"_platform<->modem_id");

    default_codecs_set_ = true;
}

void goby::acomms::DCCLCodec::add_flex_groups(util::FlexOstream* tout)
{
    tout->add_group("dccl_enc", util::Colors::lt_magenta, "encoder messages (goby_dccl)");
    tout->add_group("dccl_dec", util::Colors::lt_blue, "decoder messages (goby_dccl)");
}


bool goby::acomms::DCCLCodec::encode(std::string* bytes, const google::protobuf::Message& msg)
{
    if(!default_codecs_set_)
        set_default_codecs();
    
    //fixed header
    Bitset ccl_id_bits(HEAD_CCL_ID_SIZE + HEAD_DCCL_ID_SIZE, DCCL_CCL_HEADER);
    Bitset dccl_id_bits(HEAD_CCL_ID_SIZE + HEAD_DCCL_ID_SIZE, msg.GetDescriptor()->options().GetExtension(dccl::id));
    ccl_id_bits <<= HEAD_DCCL_ID_SIZE;    
    Bitset head_bits = ccl_id_bits | dccl_id_bits;
    Bitset body_bits;
    
    const Descriptor* desc = msg.GetDescriptor();
    try
    {
        boost::shared_ptr<DCCLFieldCodec> codec =
            codec_manager_.find(FieldDescriptor::CPPTYPE_MESSAGE,
                                desc->options().GetExtension(dccl::message_codec));
        if(codec)
        {
            DCCLFieldCodec::set_in_header(true);
            codec->encode(&head_bits, &msg, 0);
            DCCLFieldCodec::set_in_header(false);
            codec->encode(&body_bits, &msg, 0);
        }
        else
        {
            throw(DCCLException("Failed to find dccl.message_codec `" + desc->options().GetExtension(dccl::message_codec) + "`"));
        }
    }
    catch(std::exception& e)
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
    if(!default_codecs_set_)
        set_default_codecs();
    
    const Descriptor* desc = msg->GetDescriptor();
    boost::shared_ptr<DCCLFieldCodec> codec =
        codec_manager_.find(FieldDescriptor::CPPTYPE_MESSAGE,
                            desc->options().GetExtension(dccl::message_codec));
    
    DCCLFieldCodec::set_in_header(true);
    unsigned head_size_bits = codec->max_size(msg, 0) + HEAD_CCL_ID_SIZE + HEAD_DCCL_ID_SIZE;
    DCCLFieldCodec::set_in_header(false);  
    unsigned body_size_bits = codec->max_size(msg, 0);

    unsigned head_size_bytes = ceil_bits2bytes(head_size_bits);
    unsigned body_size_bytes = ceil_bits2bytes(body_size_bits);
    
    std::cout << "head is " << head_size_bits << " bits" << std::endl;
    std::cout << "body is " << body_size_bits << " bits" << std::endl;
    std::cout << "head is " << head_size_bytes << " bytes" << std::endl;
    std::cout << "body is " << body_size_bytes << " bytes" << std::endl;

    std::string head_bytes = bytes.substr(0, head_size_bytes);
    std::string body_bytes = bytes.substr(head_size_bytes);

    std::cout << "head is " << hex_encode(head_bytes) << std::endl;
    std::cout << "body is " << hex_encode(body_bytes) << std::endl;

    if(!crypto_key_.empty())
        decrypt(&body_bytes, head_bytes);
    
    Bitset head_bits, body_bits;
    string2bitset(&head_bits, head_bytes);
    string2bitset(&body_bits, body_bytes);

    head_bits.resize(head_size_bits - HEAD_CCL_ID_SIZE + HEAD_DCCL_ID_SIZE);
    
    
    std::cout << "head: " << head_bits << std::endl;
    std::cout << "body: " << body_bits << std::endl;    
    
    try
    {
        if(codec)
        {
            boost::any a(msg);
            DCCLFieldCodec::set_in_header(true);
            codec->decode(&body_bits, &a, 0);
            DCCLFieldCodec::set_in_header(false);
            codec->decode(&body_bits, &a, 0);
        }
        else
        {
            throw(DCCLException("Failed to find dccl.message_codec `" + desc->options().GetExtension(dccl::message_codec) + "`"));
        }
    }
    catch(std::exception& e)
    {
        const Descriptor* desc = msg->GetDescriptor();
        if(log_) *log_ << warn << "Message " << desc->full_name() << " failed to decode. Reason: " << e.what() << std::endl;
        return false;
    }
    return true;
}

// makes sure we can actual encode / decode a message of this descriptor given the loaded FieldCodecs
// checks all bounds on the message
bool goby::acomms::DCCLCodec::validate(const google::protobuf::Message& msg)
{
    if(!default_codecs_set_)
        set_default_codecs();

    const Descriptor* desc = msg.GetDescriptor();
    try
    {
        if(!desc->options().HasExtension(dccl::id))
            throw(DCCLException("Missing message option `dccl.id`. Specify a unique id (e.g. 3) in the body of your .proto message using \"option (dccl.id) = 3\""));
        else if(!desc->options().HasExtension(dccl::max_bytes))
            throw(DCCLException("Missing message option `dccl.max_bytes`. Specify a maximum (encoded) message size in bytes (e.g. 32) in the body of your .proto message using \"option (dccl.max_bytes) = 32\"")); 
        
        boost::shared_ptr<DCCLFieldCodec> codec =
            codec_manager_.find(FieldDescriptor::CPPTYPE_MESSAGE,
                                desc->options().GetExtension(dccl::message_codec));

        const unsigned bit_size = codec->max_size(&msg, 0);
        const unsigned byte_size = ceil_bits2bytes(bit_size);

        if(byte_size > desc->options().GetExtension(dccl::max_bytes))
            throw(DCCLException("Actual maximum size of message exceeds allowed maximum (dccl.max_bytes). Tighten bounds, remove fields, improve codecs, or increase the allowed dccl.max_bytes"));
        
        codec->validate(&msg, 0);
        
    }
    catch(DCCLException& e)
    {
        if(log_) *log_ << warn << "Message " << desc->full_name() << " failed validation. Reason: " << e.what() << std::endl;
        return false;
    }

    return true;
}

void goby::acomms::DCCLCodec::info(const google::protobuf::Message& msg, std::ostream* os)
{
    if(!default_codecs_set_)
        set_default_codecs();
    
    const Descriptor* desc = msg.GetDescriptor();   
    try
    {   
        boost::shared_ptr<DCCLFieldCodec> codec =
            codec_manager_.find(FieldDescriptor::CPPTYPE_MESSAGE,
                                desc->options().GetExtension(dccl::message_codec));

        const unsigned bit_size = codec->max_size(&msg, 0);
        const unsigned byte_size = ceil_bits2bytes(bit_size);

        const unsigned allowed_byte_size = desc->options().GetExtension(dccl::max_bytes);
        const unsigned allowed_bit_size = allowed_byte_size * BITS_IN_BYTE;
        
        *os << "== Begin " << desc->full_name() << " ==\n"
            << "Actual maximum size of message: " << byte_size << " bytes / " << bit_size << " bits\n"
            << "Allowed maximum size of message: " << allowed_byte_size << " bytes / " << allowed_bit_size << " bits\n";
        
        codec->info(&msg, 0, os);
        
        *os << "== End " << desc->full_name() << " ==" << std::endl;
    }
    catch(DCCLException& e)
    {
        if(log_) *log_ << warn << "Message " << desc->full_name() << " cannot provide information due to invalid configuration. Reason: " << e.what() << std::endl;
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
    if(cfg_.has_crypto_passphrase())
    {
        using namespace CryptoPP;
        
        SHA256 hash;
        StringSource unused(cfg_.crypto_passphrase(), true, new HashFilter(hash, new StringSink(crypto_key_)));
        
        if(log_) *log_ << group("dccl_enc") << "cryptography enabled with given passphrase" << std::endl;
    }
    else
    {
        if(log_) *log_ << group("dccl_enc") << "cryptography disabled, set crypto_passphrase to enable." << std::endl;
    }
    
}

//
// FieldCodecManager
//


const boost::shared_ptr<goby::acomms::DCCLFieldCodec>
goby::acomms::DCCLCodec::FieldCodecManager::find(
    google::protobuf::FieldDescriptor::CppType cpp_type, const std::string& name) const
{
    typedef InsideMap::const_iterator InsideIterator;
    typedef std::map<google::protobuf::FieldDescriptor::CppType, InsideMap>::const_iterator Iterator;
                    
    Iterator it = codecs_.find(cpp_type);
    if(it != codecs_.end())
    {
        InsideIterator inside_it = it->second.find(name);
        if(inside_it != it->second.end())
        {
            return inside_it->second;
        }
    }                    
    throw(DCCLException("No codec by the name `" + name + "` found for type: " + DCCLCodec::cpptype_helper().find(cpp_type)->as_str()));
}



//
// CppTypeHelper
//

goby::acomms::DCCLCodec::CppTypeHelper::CppTypeHelper()                
{
    using namespace google::protobuf;    
    using boost::shared_ptr;
    boost::assign::insert (map_)
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
}

boost::shared_ptr<goby::acomms::FromProtoCppTypeBase> goby::acomms::DCCLCodec::CppTypeHelper::find(google::protobuf::FieldDescriptor::CppType cpptype)
{
    Map::iterator it = map_.find(cpptype);
    if(it != map_.end())
        return it->second;
    else
        return boost::shared_ptr<FromProtoCppTypeBase>();
}

const boost::shared_ptr<goby::acomms::FromProtoCppTypeBase> goby::acomms::DCCLCodec::CppTypeHelper::find(google::protobuf::FieldDescriptor::CppType cpptype) const
{
    Map::const_iterator it = map_.find(cpptype);
    if(it != map_.end())
        return it->second;
    else
        return boost::shared_ptr<FromProtoCppTypeBase>();
}
