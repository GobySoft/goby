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

#include <algorithm>

#include <boost/foreach.hpp>
#include <boost/assign.hpp>
#include <crypto++/filters.h>
#include <crypto++/sha.h>
#include <crypto++/modes.h>
#include <crypto++/aes.h>

#include "dccl.h"
#include "dccl_field_codec_default.h"
#include "goby/util/as.h"
#include "goby/protobuf/acomms_proto_helpers.h"
#include "goby/protobuf/dccl_option_extensions.pb.h"
#include "goby/protobuf/dynamic_protobuf_manager.h"
#include "goby/protobuf/header.pb.h"

using goby::util::goby_time;
using goby::util::as;
using goby::util::hex_encode;
using goby::util::hex_decode;
using goby::glog;

using google::protobuf::FieldDescriptor;
using google::protobuf::Descriptor;
using google::protobuf::Reflection;

boost::shared_ptr<goby::acomms::DCCLCodec> goby::acomms::DCCLCodec::inst_;

//
// DCCLCodec
//

goby::acomms::DCCLCodec::DCCLCodec()
    : DEFAULT_CODEC_NAME("_default"),
      current_id_codec_(DEFAULT_CODEC_NAME)
{
    id_codec_[current_id_codec_] = boost::shared_ptr<DCCLTypedFieldCodec<uint32> >(new DCCLDefaultIdentifierCodec);

    glog.add_group("dccl.encode", util::Colors::lt_magenta, "encoder messages (goby_dccl)");
    glog.add_group("dccl.decode", util::Colors::lt_blue, "decoder messages (goby_dccl)");

    set_default_codecs();
}

void goby::acomms::DCCLCodec::set_default_codecs()
{
    using google::protobuf::FieldDescriptor;
    DCCLFieldCodecManager::add<DCCLDefaultArithmeticFieldCodec<double> >(DEFAULT_CODEC_NAME);
    DCCLFieldCodecManager::add<DCCLDefaultArithmeticFieldCodec<float> >(DEFAULT_CODEC_NAME);
    DCCLFieldCodecManager::add<DCCLDefaultBoolCodec>(DEFAULT_CODEC_NAME);
    DCCLFieldCodecManager::add<DCCLDefaultArithmeticFieldCodec<int32> >(DEFAULT_CODEC_NAME);
    DCCLFieldCodecManager::add<DCCLDefaultArithmeticFieldCodec<int64> >(DEFAULT_CODEC_NAME);
    DCCLFieldCodecManager::add<DCCLDefaultArithmeticFieldCodec<uint32> >(DEFAULT_CODEC_NAME);
    DCCLFieldCodecManager::add<DCCLDefaultArithmeticFieldCodec<uint64> >(DEFAULT_CODEC_NAME);
    DCCLFieldCodecManager::add<DCCLDefaultStringCodec, FieldDescriptor::TYPE_STRING>(DEFAULT_CODEC_NAME);
    DCCLFieldCodecManager::add<DCCLDefaultBytesCodec, FieldDescriptor::TYPE_BYTES>(DEFAULT_CODEC_NAME);
    DCCLFieldCodecManager::add<DCCLDefaultEnumCodec>(DEFAULT_CODEC_NAME);
    DCCLFieldCodecManager::add<DCCLDefaultMessageCodec, FieldDescriptor::TYPE_MESSAGE>(DEFAULT_CODEC_NAME);

    DCCLFieldCodecManager::add<DCCLTimeCodec>("_time");
    DCCLFieldCodecManager::add<DCCLStaticCodec<std::string> >("_static"); 
    DCCLFieldCodecManager::add<DCCLModemIdConverterCodec>("_platform<->modem_id");
}



std::string goby::acomms::DCCLCodec::encode(const google::protobuf::Message& msg)
{
    const Descriptor* desc = msg.GetDescriptor();

    try
    {
        if(!msg.IsInitialized())
            throw(DCCLException("Message is not properly initialized. All `required` fields must be set."));
        
        if(!id2desc_.count(id(desc)))
            throw(DCCLException("Message id " +
                                as<std::string>(id(desc))+
                                " has not been validated. Call validate() before encoding this type."));
    

        //fixed header
        Bitset fixed_id_bits = id_codec_[current_id_codec_]->encode(id(desc));
        Bitset head_bits, body_bits;

        boost::shared_ptr<DCCLFieldCodecBase> codec =
            DCCLFieldCodecManager::find(desc, desc->options().GetExtension(goby::msg).dccl().codec());
        boost::shared_ptr<FromProtoCppTypeBase> helper =
            DCCLTypeHelper::find(desc);

        if(codec)
        {
            DCCLFieldCodecBase::MessageHandler msg_handler;
            msg_handler.push(msg.GetDescriptor());
            codec->base_encode(&head_bits, helper->get_value(msg), DCCLFieldCodecBase::HEAD);
            codec->base_encode(&body_bits, helper->get_value(msg), DCCLFieldCodecBase::BODY);
        }
        else
        {
            throw(DCCLException("Failed to find (goby.msg).dccl.codec `" + desc->options().GetExtension(goby::msg).dccl().codec() + "`"));
        }
        
        // given header of not even byte size (e.g. 01011), make even byte size (e.g. 00001011)
        // and shift bytes to MSB (e.g. 01011000).
        unsigned head_byte_size = ceil_bits2bytes(head_bits.size() + fixed_id_bits.size());
        unsigned head_bits_diff = head_byte_size * BITS_IN_BYTE - (head_bits.size() + fixed_id_bits.size());
        head_bits.resize(head_bits.size() + head_bits_diff);
        for(int i = 0, n = fixed_id_bits.size(); i < n; ++i)
            head_bits.push_back(fixed_id_bits[i]);

        std::string head_bytes, body_bytes;
        bitset2string(head_bits, &head_bytes);
        bitset2string(body_bits, &body_bytes);
    
        if(!crypto_key_.empty())
            encrypt(&body_bytes, head_bytes);

        // reverse so the body reads LSB->MSB such that extra chars at
        // the end of the string get tacked on to the MSB, not the LSB where they would cause trouble
        std::reverse(body_bytes.begin(), body_bytes.end());

        glog.is(debug1) && glog  << "(encode) head: " << head_bits << std::endl;
        glog.is(debug1) && glog  << "(encode) body: " << body_bits << std::endl;
        return head_bytes + body_bytes;
    }
    catch(std::exception& e)
    {
        std::stringstream ss;
        
        ss << "Message " << desc->full_name() << " failed to encode. Reason: " << e.what() << std::endl;

        glog.is(warn) && glog <<  ss.str();        
        throw(DCCLException(ss.str()));
    }
}

unsigned goby::acomms::DCCLCodec::id_from_encoded(const std::string& bytes)
{
    if(bytes.length() < id_codec_[current_id_codec_]->min_size() / BITS_IN_BYTE)
        throw(DCCLException("Bytes passed (hex: " + hex_encode(bytes) + ") is too small to be a valid DCCL message"));
        
    Bitset fixed_header_bits;
    string2bitset(&fixed_header_bits, bytes.substr(0, std::ceil(double(id_codec_[current_id_codec_]->max_size()) / BITS_IN_BYTE)));

    glog.is(debug1) && glog  << fixed_header_bits << std::endl;

    Bitset these_bits;
    DCCLFieldCodecBase::BitsHandler bits_handler(&these_bits, &fixed_header_bits, false);
    bits_handler.transfer_bits(id_codec_[current_id_codec_]->min_size());
    return id_codec_[current_id_codec_]->decode(&these_bits);
}


boost::shared_ptr<google::protobuf::Message> goby::acomms::DCCLCodec::decode(const std::string& bytes)   
{
    try
    {
        unsigned id = id_from_encoded(bytes);   
        
        if(!id2desc_.count(id))
            throw(DCCLException("Message id " + as<std::string>(id) + " has not been validated. Call validate() before decoding this type."));

        // ownership of this object goes to the caller of decode()
        boost::shared_ptr<google::protobuf::Message> msg = 
            goby::protobuf::DynamicProtobufManager::new_protobuf_message(id2desc_.find(id)->second);
        
        const Descriptor* desc = msg->GetDescriptor();        
    
        boost::shared_ptr<DCCLFieldCodecBase> codec =
            DCCLFieldCodecManager::find(desc, desc->options().GetExtension(goby::msg).dccl().codec());
        boost::shared_ptr<FromProtoCppTypeBase> helper =
            DCCLTypeHelper::find(desc);

        
        unsigned head_size_bits = id_codec_[current_id_codec_]->size(id), body_size_bits = 0;
        codec->base_max_size(&head_size_bits, desc, DCCLFieldCodecBase::HEAD);
        codec->base_max_size(&body_size_bits, desc, DCCLFieldCodecBase::BODY);

        unsigned head_size_bytes = ceil_bits2bytes(head_size_bits);
        unsigned body_size_bytes = ceil_bits2bytes(body_size_bits);
    
        glog.is(debug1) && glog  << "(decode) head is " << head_size_bits << " bits" << std::endl;
        glog.is(debug1) && glog  << "(decode) body is " << body_size_bits << " bits" << std::endl;
        glog.is(debug1) && glog  << "(decode) head is " << head_size_bytes << " bytes" << std::endl;
        glog.is(debug1) && glog  << "(decode) body is " << body_size_bytes << " bytes" << std::endl;

        std::string head_bytes = bytes.substr(0, head_size_bytes);
        std::string body_bytes = bytes.substr(head_size_bytes);
        // we had reversed the bytes so extraneous zeros will not cause trouble. undo this reversal.
        std::reverse(body_bytes.begin(), body_bytes.end());
        
        glog.is(debug1) && glog  << "(decode) head is " << hex_encode(head_bytes) << std::endl;
        glog.is(debug1) && glog  << "(decode) body is " << hex_encode(body_bytes) << std::endl;

        if(!crypto_key_.empty())
            decrypt(&body_bytes, head_bytes);
    
        Bitset head_bits, body_bits;
        string2bitset(&head_bits, head_bytes);
        string2bitset(&body_bits, body_bytes);
    
        glog.is(debug1) && glog  << "head: " << head_bits << std::endl;
        glog.is(debug1) && glog  << "body: " << body_bits << std::endl;
        
        head_bits.resize(head_size_bits - id_codec_[current_id_codec_]->size(id));

        glog.is(debug1) && glog  << "head after removing fixed portion: " << head_bits << std::endl;

        if(codec)
        {
            DCCLFieldCodecBase::MessageHandler msg_handler;
            msg_handler.push(msg->GetDescriptor());

            boost::any value(msg);
            codec->base_decode(&head_bits, &value, DCCLFieldCodecBase::HEAD);
            helper->set_value(msg.get(), value);
            glog.is(debug1) && glog  << "after header decode, message is: " << *msg << std::endl;
            codec->base_decode(&body_bits, &value, DCCLFieldCodecBase::BODY);
            helper->set_value(msg.get(), value);
            glog.is(debug1) && glog  << "after header & body decode, message is: " << *msg << std::endl;
        }
        else
        {
            throw(DCCLException("Failed to find (goby.msg).dccl.codec `" + desc->options().GetExtension(goby::msg).dccl().codec() + "`"));
        }
        return msg;
    }
    catch(std::exception& e)
    {
        std::stringstream ss;
        
        ss << "Message " << hex_encode(bytes) <<  " failed to decode. Reason: " << e.what() << std::endl;

        glog.is(warn) && glog <<  ss.str();        
        throw(DCCLException(ss.str()));
    }    
}

// makes sure we can actual encode / decode a message of this descriptor given the loaded FieldCodecs
// checks all bounds on the message
bool goby::acomms::DCCLCodec::validate(const google::protobuf::Descriptor* desc)
{    
    try
    {
        if(!desc->options().GetExtension(goby::msg).dccl().has_id())
            throw(DCCLException("Missing message option `(goby.msg).dccl.id`. Specify a unique id (e.g. 3) in the body of your .proto message using \"option (goby.msg).dccl.id = 3\""));
        if(!desc->options().GetExtension(goby::msg).dccl().has_max_bytes())
            throw(DCCLException("Missing message option `(goby.msg).dccl.max_bytes`. Specify a maximum (encoded) message size in bytes (e.g. 32) in the body of your .proto message using \"option (goby.msg).dccl.max_bytes = 32\""));
        
        boost::shared_ptr<DCCLFieldCodecBase> codec =
            DCCLFieldCodecManager::find(desc, desc->options().GetExtension(goby::msg).dccl().codec());

        unsigned dccl_id = id(desc);
        unsigned head_size_bits = id_codec_[current_id_codec_]->size(dccl_id), body_size_bits = 0;
        codec->base_max_size(&head_size_bits, desc, DCCLFieldCodecBase::HEAD);
        codec->base_max_size(&body_size_bits, desc, DCCLFieldCodecBase::BODY);
        
        const unsigned byte_size = ceil_bits2bytes(head_size_bits) + ceil_bits2bytes(body_size_bits);

        if(byte_size > desc->options().GetExtension(goby::msg).dccl().max_bytes())
            throw(DCCLException("Actual maximum size of message exceeds allowed maximum (dccl.max_bytes). Tighten bounds, remove fields, improve codecs, or increase the allowed dccl.max_bytes"));
        
        codec->base_validate(desc, DCCLFieldCodecBase::HEAD);
        codec->base_validate(desc, DCCLFieldCodecBase::BODY);

        if(id2desc_.count(dccl_id) && desc != id2desc_.find(dccl_id)->second)
            throw(DCCLException("`dccl.id` " + as<std::string>(dccl_id) + " is already in use by Message " + id2desc_.find(dccl_id)->second->full_name()));
        else
            id2desc_.insert(std::make_pair(id(desc), desc));
    }
    catch(DCCLException& e)
    {
        try
        {
            info(desc, &glog);
        }
        catch(DCCLException& e)
        { }
        
        glog.is(warn) && glog << "Message " << desc->full_name() << " failed validation. Reason: "
                             << e.what() <<  "\n"
                             << "If possible, information about the Message are printed above. " << std::endl;
        return false;
    }

    return true;
}

unsigned goby::acomms::DCCLCodec::size(const google::protobuf::Message& msg)
{
    const Descriptor* desc = msg.GetDescriptor();

    boost::shared_ptr<DCCLFieldCodecBase> codec =
        DCCLFieldCodecManager::find(desc, desc->options().GetExtension(goby::msg).dccl().codec());
    
    unsigned dccl_id = id(desc);
    unsigned head_size_bits = id_codec_[current_id_codec_]->size(dccl_id);
    codec->base_size(&head_size_bits, msg, DCCLFieldCodecBase::HEAD);
    
    unsigned body_size_bits = 0;
    codec->base_size(&body_size_bits, msg, DCCLFieldCodecBase::BODY);
    
    const unsigned head_size_bytes = ceil_bits2bytes(head_size_bits);
    const unsigned body_size_bytes = ceil_bits2bytes(body_size_bits);

    glog.is(debug1) && glog  << "head size bytes: " << head_size_bytes << std::endl;
    glog.is(debug1) && glog  << "body size bytes: " << body_size_bytes << std::endl;
    
    return head_size_bytes + body_size_bytes;
}



void goby::acomms::DCCLCodec::run_hooks(const google::protobuf::Message& msg)
{
    const Descriptor* desc = msg.GetDescriptor();

    boost::shared_ptr<DCCLFieldCodecBase> codec =
        DCCLFieldCodecManager::find(desc, desc->options().GetExtension(goby::msg).dccl().codec());
    
    codec->base_run_hooks(msg, DCCLFieldCodecBase::HEAD);
    codec->base_run_hooks(msg, DCCLFieldCodecBase::BODY);
}


void goby::acomms::DCCLCodec::info(const google::protobuf::Descriptor* desc, std::ostream* os)
{
    try
    {   
        boost::shared_ptr<DCCLFieldCodecBase> codec =
            DCCLFieldCodecManager::find(desc, desc->options().GetExtension(goby::msg).dccl().codec());

        unsigned config_head_bit_size = 0, body_bit_size = 0;
        codec->base_max_size(&config_head_bit_size, desc, DCCLFieldCodecBase::HEAD);
        codec->base_max_size(&body_bit_size, desc, DCCLFieldCodecBase::BODY);

        unsigned dccl_id = id(desc);
        const unsigned id_bit_size = id_codec_[current_id_codec_]->size(dccl_id);
        const unsigned bit_size = id_bit_size + config_head_bit_size + body_bit_size;

        
        const unsigned byte_size = ceil_bits2bytes(config_head_bit_size + id_bit_size) + ceil_bits2bytes(body_bit_size);
        
        const unsigned allowed_byte_size = desc->options().GetExtension(goby::msg).dccl().max_bytes();
        const unsigned allowed_bit_size = allowed_byte_size * BITS_IN_BYTE;
        
        *os << "== Begin " << desc->full_name() << " ==\n"
            << "Actual maximum size of message: " << byte_size << " bytes / "
            << byte_size*BITS_IN_BYTE  << " bits [dccl.id head: " << id_bit_size
            << ", user head: " << config_head_bit_size << ", body: "
            << body_bit_size << ", padding: " << byte_size * BITS_IN_BYTE - bit_size << "]\n"
            << "Allowed maximum size of message: " << allowed_byte_size << " bytes / "
            << allowed_bit_size << " bits\n";

        *os << "= Header =" << std::endl;
        codec->base_info(os, desc, DCCLFieldCodecBase::HEAD);
        *os << "= Body =" << std::endl;
        codec->base_info(os, desc, DCCLFieldCodecBase::BODY);
        
        *os << "== End " << desc->full_name() << " ==" << std::endl;
    }
    catch(DCCLException& e)
    {
        glog.is(warn) && glog << "Message " << desc->full_name() << " cannot provide information due to invalid configuration. Reason: " << e.what() << std::endl;
    }
        
}

bool goby::acomms::DCCLCodec::validate_repeated(const std::list<const google::protobuf::Descriptor*>& desc)
{
    bool out = true;
    BOOST_FOREACH(const google::protobuf::Descriptor* p, desc)
        out &= validate(p);
    return out;
}

void goby::acomms::DCCLCodec::info_repeated(const std::list<const google::protobuf::Descriptor*>& desc, std::ostream* os)
{
    BOOST_FOREACH(const google::protobuf::Descriptor* p, desc)
        info(p, os);
}




std::list<boost::shared_ptr<google::protobuf::Message> > goby::acomms::DCCLCodec::decode_repeated(const std::string& orig_bytes)
{
    std::string bytes = orig_bytes;
    std::list<boost::shared_ptr<google::protobuf::Message> > out;
    while(!bytes.empty())
    {
        try
        {
            out.push_back(decode(bytes));
            unsigned last_size = size(*out.back());
            glog.is(debug1) && glog  << "last message size was: " << last_size << std::endl;
            bytes.erase(0, last_size);
        }
        catch(DCCLException& e)
        {
            if(out.empty())
                throw(e);
            else
            {
                glog.is(warn) && glog << "failed to decode " << goby::util::hex_encode(bytes) << " but returning parts already decoded"  << std::endl;
                return out;
            }
        }        
    }
    return out;
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
        
        glog.is(debug1) && glog << group("dccl.encode") << "cryptography enabled with given passphrase" << std::endl;
    }
    else
    {
        glog.is(debug1) && glog << group("dccl.encode") << "cryptography disabled, set crypto_passphrase to enable." << std::endl;
    }
    
}


