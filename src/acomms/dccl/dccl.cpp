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

#ifdef HAS_CRYPTOPP
#include <crypto++/filters.h>
#include <crypto++/sha.h>
#include <crypto++/modes.h>
#include <crypto++/aes.h>
#endif // HAS_CRYPTOPP

#include "dccl.h"
#include "dccl_field_codec_default.h"
#include "goby/util/as.h"
#include "goby/protobuf/acomms_option_extensions.pb.h"
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

std::string goby::acomms::DCCLCodec::glog_encode_group_;
std::string goby::acomms::DCCLCodec::glog_decode_group_;

//
// DCCLCodec
//

goby::acomms::DCCLCodec::DCCLCodec()
    : DEFAULT_CODEC_NAME("_default"),
      current_id_codec_(DEFAULT_CODEC_NAME)
{
    id_codec_[current_id_codec_] = boost::shared_ptr<DCCLTypedFieldCodec<uint32> >(new DCCLDefaultIdentifierCodec);

    glog_encode_group_ = "goby::acomms::dccl::encode";
    glog_decode_group_ = "goby::acomms::dccl::decode";
    
    glog.add_group(glog_encode_group_, util::Colors::lt_magenta);
    glog.add_group(glog_decode_group_, util::Colors::lt_blue);

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

    DCCLFieldCodecManager::add<DCCLTimeCodec<std::string> >("_time");
    DCCLFieldCodecManager::add<DCCLTimeCodec<uint64> >("_time");
    DCCLFieldCodecManager::add<DCCLTimeCodec<double> >("_time");

    DCCLFieldCodecManager::add<DCCLStaticCodec<std::string> >("_static"); 
    DCCLFieldCodecManager::add<DCCLModemIdConverterCodec>("_platform<->modem_id");
}


void goby::acomms::DCCLCodec::encode(std::string* bytes, const google::protobuf::Message& msg)
{
    const Descriptor* desc = msg.GetDescriptor();

    glog.is(debug1) && glog << group(glog_encode_group_) << "Began encoding message of type: " << desc->full_name() << std::endl;    

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

        boost::shared_ptr<DCCLFieldCodecBase> codec = DCCLFieldCodecManager::find(desc);
        boost::shared_ptr<FromProtoCppTypeBase> helper = DCCLTypeHelper::find(desc);

        if(codec)
        {
            MessageHandler msg_handler;
            msg_handler.push(msg.GetDescriptor());
            codec->base_encode(&head_bits, msg, DCCLFieldCodecBase::HEAD);
            codec->base_encode(&body_bits, msg, DCCLFieldCodecBase::BODY);
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

        
        glog.is(debug2) && glog << group(glog_encode_group_) << "Head bytes (bits): " << head_bytes.size() << "(" << head_bits.size()
                                << "), body bytes (bits): " <<  body_bytes.size() << "(" << body_bits.size() << ")" <<  std::endl;
        glog.is(debug3) && glog << group(glog_encode_group_) << "Unencrypted Head (bin): " << head_bits << std::endl;
        glog.is(debug3) && glog << group(glog_encode_group_) << "Unencrypted Body (bin): " << body_bits << std::endl;
        glog.is(debug3) && glog << group(glog_encode_group_) << "Unencrypted Head (hex): " << hex_encode(head_bytes) << std::endl;
        glog.is(debug3) && glog << group(glog_encode_group_) << "Unencrypted Body (hex): " << hex_encode(body_bytes) << std::endl;
        
        if(!crypto_key_.empty())
            encrypt(&body_bytes, head_bytes);

        glog.is(debug3) && glog << group(glog_encode_group_) << "Encrypted Body (hex): " << hex_encode(body_bytes) << std::endl;

        
        // reverse so the body reads LSB->MSB such that extra chars at
        // the end of the string get tacked on to the MSB, not the LSB where they would cause trouble
        std::reverse(body_bytes.begin(), body_bytes.end());
        
        glog.is(debug3) && glog << group(glog_encode_group_) << "Reversed Encrypted Body (hex): " << hex_encode(body_bytes) << std::endl;

        glog.is(debug1) && glog << group(glog_encode_group_) << "Successfully encoded message of type: " << desc->full_name() << std::endl;

        *bytes = head_bytes + body_bytes;
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
    unsigned id_min_size = 0, id_max_size = 0;
    id_codec_[current_id_codec_]->field_min_size(&id_min_size, 0);
    id_codec_[current_id_codec_]->field_max_size(&id_max_size, 0);
    
    if(bytes.length() < (id_min_size / BITS_IN_BYTE))
        throw(DCCLException("Bytes passed (hex: " + hex_encode(bytes) + ") is too small to be a valid DCCL message"));
        
    Bitset fixed_header_bits;
    string2bitset(&fixed_header_bits, bytes.substr(0, std::ceil(double(id_max_size) / BITS_IN_BYTE)));

    Bitset these_bits;
    BitsHandler bits_handler(&these_bits, &fixed_header_bits, false);
    bits_handler.transfer_bits(id_min_size);
    
    return id_codec_[current_id_codec_]->decode(these_bits);
}

void goby::acomms::DCCLCodec::decode(const std::string& bytes, google::protobuf::Message* msg)
{
    try
    {
        unsigned id = id_from_encoded(bytes);   

        glog.is(debug1) && glog << group(glog_decode_group_) << "Began decoding message of id: " << id << std::endl;
        
        if(!id2desc_.count(id))
            throw(DCCLException("Message id " + as<std::string>(id) + " has not been validated. Call validate() before decoding this type."));

        const Descriptor* desc = msg->GetDescriptor();
        
        glog.is(debug1) && glog << group(glog_decode_group_) << "Type name: " << desc->full_name() << std::endl;
        
        boost::shared_ptr<DCCLFieldCodecBase> codec = DCCLFieldCodecManager::find(desc);
        boost::shared_ptr<FromProtoCppTypeBase> helper = DCCLTypeHelper::find(desc);

        
        unsigned head_size_bits, body_size_bits;
        codec->base_max_size(&head_size_bits, desc, DCCLFieldCodecBase::HEAD);
        codec->base_max_size(&body_size_bits, desc, DCCLFieldCodecBase::BODY);
        head_size_bits += id_codec_[current_id_codec_]->size(id);
        
        unsigned head_size_bytes = ceil_bits2bytes(head_size_bits);
        unsigned body_size_bytes = ceil_bits2bytes(body_size_bits);
    
        glog.is(debug2) && glog << group(glog_decode_group_) << "Head bytes (bits): " << head_size_bytes << "(" << head_size_bits
                                << "), body bytes (bits): " << body_size_bytes << "(" << body_size_bits << ")" <<  std::endl;

        std::string head_bytes = bytes.substr(0, head_size_bytes);
        std::string body_bytes = bytes.substr(head_size_bytes);


        glog.is(debug3) && glog << group(glog_decode_group_) << "Reversed Encrypted Body (hex): " << hex_encode(body_bytes) << std::endl;

        // we had reversed the bytes so extraneous zeros will not cause trouble. undo this reversal.
        std::reverse(body_bytes.begin(), body_bytes.end());

        glog.is(debug3) && glog << group(glog_decode_group_) << "Encrypted Body (hex): " << hex_encode(body_bytes) << std::endl;


        if(!crypto_key_.empty())
            decrypt(&body_bytes, head_bytes);

        glog.is(debug3) && glog << group(glog_decode_group_) << "Unencrypted Head (hex): " << hex_encode(head_bytes) << std::endl;
        glog.is(debug3) && glog << group(glog_decode_group_) << "Unencrypted Body (hex): " << hex_encode(body_bytes) << std::endl;

        
        Bitset head_bits, body_bits;
        string2bitset(&head_bits, head_bytes);
        string2bitset(&body_bits, body_bytes);
    
        glog.is(debug3) && glog << group(glog_decode_group_) << "Unencrypted Head (bin): " << head_bits << std::endl;
        glog.is(debug3) && glog << group(glog_decode_group_) << "Unencrypted Body (bin): " << body_bits << std::endl;
        
        head_bits.resize(head_size_bits - id_codec_[current_id_codec_]->size(id));

        if(codec)
        {
            MessageHandler msg_handler;
            msg_handler.push(msg->GetDescriptor());

            codec->base_decode(&head_bits, msg, DCCLFieldCodecBase::HEAD);
            glog.is(debug2) && glog << group(glog_decode_group_) << "after header decode, message is: " << *msg << std::endl;
            codec->base_decode(&body_bits, msg, DCCLFieldCodecBase::BODY);
            glog.is(debug2) && glog << group(glog_decode_group_) << "after header & body decode, message is: " << *msg << std::endl;
        }
        else
        {
            throw(DCCLException("Failed to find (goby.msg).dccl.codec `" + desc->options().GetExtension(goby::msg).dccl().codec() + "`"));
        }

        glog.is(debug1) && glog << group(glog_decode_group_) << "Successfully decoded message of type: " << desc->full_name() << std::endl;
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
void goby::acomms::DCCLCodec::validate(const google::protobuf::Descriptor* desc)
{    
    try
    {
        if(!desc->options().GetExtension(goby::msg).dccl().has_id())
            throw(DCCLException("Missing message option `(goby.msg).dccl.id`. Specify a unique id (e.g. 3) in the body of your .proto message using \"option (goby.msg).dccl.id = 3\""));
        if(!desc->options().GetExtension(goby::msg).dccl().has_max_bytes())
            throw(DCCLException("Missing message option `(goby.msg).dccl.max_bytes`. Specify a maximum (encoded) message size in bytes (e.g. 32) in the body of your .proto message using \"option (goby.msg).dccl.max_bytes = 32\""));
        
        boost::shared_ptr<DCCLFieldCodecBase> codec = DCCLFieldCodecManager::find(desc);

        unsigned dccl_id = id(desc);
        unsigned head_size_bits, body_size_bits;
        codec->base_max_size(&head_size_bits, desc, DCCLFieldCodecBase::HEAD);
        codec->base_max_size(&body_size_bits, desc, DCCLFieldCodecBase::BODY);
        head_size_bits += id_codec_[current_id_codec_]->size(dccl_id);
        
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

        throw;
    }
}

unsigned goby::acomms::DCCLCodec::size(const google::protobuf::Message& msg)
{
    const Descriptor* desc = msg.GetDescriptor();

    boost::shared_ptr<DCCLFieldCodecBase> codec = DCCLFieldCodecManager::find(desc);
    
    unsigned dccl_id = id(desc);
    unsigned head_size_bits;
    codec->base_size(&head_size_bits, msg, DCCLFieldCodecBase::HEAD);
    head_size_bits += id_codec_[current_id_codec_]->size(dccl_id);
    
    unsigned body_size_bits;
    codec->base_size(&body_size_bits, msg, DCCLFieldCodecBase::BODY);
    
    const unsigned head_size_bytes = ceil_bits2bytes(head_size_bits);
    const unsigned body_size_bytes = ceil_bits2bytes(body_size_bits);

    return head_size_bytes + body_size_bytes;
}



void goby::acomms::DCCLCodec::run_hooks(const google::protobuf::Message& msg)
{
    const Descriptor* desc = msg.GetDescriptor();

    boost::shared_ptr<DCCLFieldCodecBase> codec = DCCLFieldCodecManager::find(desc);
    
    codec->base_run_hooks(msg, DCCLFieldCodecBase::HEAD);
    codec->base_run_hooks(msg, DCCLFieldCodecBase::BODY);
}


void goby::acomms::DCCLCodec::info(const google::protobuf::Descriptor* desc, std::ostream* os) const
{
    try
    {   
        boost::shared_ptr<DCCLFieldCodecBase> codec = DCCLFieldCodecManager::find(desc);

        unsigned config_head_bit_size, body_bit_size;
        codec->base_max_size(&config_head_bit_size, desc, DCCLFieldCodecBase::HEAD);
        codec->base_max_size(&body_bit_size, desc, DCCLFieldCodecBase::BODY);

        unsigned dccl_id = id(desc);
        const unsigned id_bit_size = id_codec_.find(current_id_codec_)->second->size(dccl_id);
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

void goby::acomms::DCCLCodec::validate_repeated(const std::list<const google::protobuf::Descriptor*>& desc)
{
    BOOST_FOREACH(const google::protobuf::Descriptor* p, desc)
        validate(p);
}

void goby::acomms::DCCLCodec::info_repeated(const std::list<const google::protobuf::Descriptor*>& desc, std::ostream* os) const
{
    BOOST_FOREACH(const google::protobuf::Descriptor* p, desc)
        info(p, os);
}




void goby::acomms::DCCLCodec::encrypt(std::string* s, const std::string& nonce /* message head */)
{
#ifdef HAS_CRYPTOPP
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
#endif
}

void goby::acomms::DCCLCodec::decrypt(std::string* s, const std::string& nonce)
{
#ifdef HAS_CRYPTOPP
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
#endif
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
#ifdef HAS_CRYPTOPP
        using namespace CryptoPP;
        
        SHA256 hash;
        StringSource unused(cfg_.crypto_passphrase(), true, new HashFilter(hash, new StringSink(crypto_key_)));
        
        glog.is(debug1) && glog << group(glog_encode_group_) << "cryptography enabled with given passphrase" << std::endl;
#else
        glog.is(warn) && glog << group(glog_encode_group_) << "cryptography disabled because Goby was compiled without support of Crypto++. Install Crypto++ and recompile to enable cryptography." << std::endl;
#endif
    }
    else
    {
        glog.is(debug1) && glog << group(glog_encode_group_) << "cryptography disabled, set crypto_passphrase to enable." << std::endl;
    }
    
}


