// Copyright 2009-2012 Toby Schneider (https://launchpad.net/~tes)
//                     Massachusetts Institute of Technology (2007-)
//                     Woods Hole Oceanographic Institution (2007-)
//                     Goby Developers Team (https://launchpad.net/~goby-dev)
// 
//
// This file is part of the Goby Underwater Autonomy Project Libraries
// ("The Goby Libraries").
//
// The Goby Libraries are free software: you can redistribute them and/or modify
// them under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// The Goby Libraries are distributed in the hope that they will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with Goby.  If not, see <http://www.gnu.org/licenses/>.


#include <algorithm>

#include <boost/foreach.hpp>
#include <boost/assign.hpp>

#include <dlfcn.h> // for shared library loading

#ifdef HAS_CRYPTOPP
#if CRYPTOPP_PATH_USES_PLUS_SIGN
#include <crypto++/filters.h>
#include <crypto++/sha.h>
#include <crypto++/modes.h>
#include <crypto++/aes.h>
#else
#include <cryptopp/filters.h>
#include <cryptopp/sha.h>
#include <cryptopp/modes.h>
#include <cryptopp/aes.h>
#endif // CRYPTOPP_PATH_USES_PLUS_SIGN
#endif // HAS_CRYPTOPP

#include "dccl.h"
#include "dccl_field_codec_default.h"
#include "dccl_ccl_compatibility.h"
#include "goby/util/as.h"
#include "goby/common/protobuf/acomms_option_extensions.pb.h"
//#include "goby/common/header.pb.h"

using goby::common::goby_time;
using goby::util::as;
using goby::util::hex_encode;
using goby::util::hex_decode;
using goby::glog;
using namespace goby::common::logger;

using google::protobuf::FieldDescriptor;
using google::protobuf::Descriptor;
using google::protobuf::Reflection;

boost::shared_ptr<goby::acomms::DCCLCodec> goby::acomms::DCCLCodec::inst_;

const std::string goby::acomms::DCCLCodec::DEFAULT_CODEC_NAME = "";
std::string goby::acomms::DCCLCodec::glog_encode_group_;
std::string goby::acomms::DCCLCodec::glog_decode_group_;

//
// DCCLCodec
//

goby::acomms::DCCLCodec::DCCLCodec()
    : current_id_codec_(DEFAULT_CODEC_NAME)
{

    glog_encode_group_ = "goby::acomms::dccl::encode";
    glog_decode_group_ = "goby::acomms::dccl::decode";
    
    glog.add_group(glog_encode_group_, common::Colors::lt_magenta);
    glog.add_group(glog_decode_group_, common::Colors::lt_blue);

    set_default_codecs();
    process_cfg();
}

void goby::acomms::DCCLCodec::set_default_codecs()
{
    using google::protobuf::FieldDescriptor;
    DCCLFieldCodecManager::add<DCCLDefaultNumericFieldCodec<double> >(DEFAULT_CODEC_NAME);
    DCCLFieldCodecManager::add<DCCLDefaultNumericFieldCodec<float> >(DEFAULT_CODEC_NAME);
    DCCLFieldCodecManager::add<DCCLDefaultBoolCodec>(DEFAULT_CODEC_NAME);
    DCCLFieldCodecManager::add<DCCLDefaultNumericFieldCodec<int32> >(DEFAULT_CODEC_NAME);
    DCCLFieldCodecManager::add<DCCLDefaultNumericFieldCodec<int64> >(DEFAULT_CODEC_NAME);
    DCCLFieldCodecManager::add<DCCLDefaultNumericFieldCodec<uint32> >(DEFAULT_CODEC_NAME);
    DCCLFieldCodecManager::add<DCCLDefaultNumericFieldCodec<uint64> >(DEFAULT_CODEC_NAME);
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

    glog.is(DEBUG1) && glog << group(glog_encode_group_) << "Began encoding message of type: " << desc->full_name() << std::endl;    

    try
    {
        if(!msg.IsInitialized())
            throw(DCCLException("Message is not properly initialized. All `required` fields must be set."));
        
        if(!id2desc_.count(id(desc)))
            throw(DCCLException("Message id " +
                                as<std::string>(id(desc))+
                                " has not been validated. Call validate() before encoding this type."));
    
        
        //fixed header
        Bitset head_bits = id_codec_[current_id_codec_]->encode(id(desc));        
        Bitset body_bits;
        
        boost::shared_ptr<DCCLFieldCodecBase> codec = DCCLFieldCodecManager::find(desc);
        boost::shared_ptr<FromProtoCppTypeBase> helper = DCCLTypeHelper::find(desc);

        if(codec)
        {
            MessageHandler msg_handler;
            msg_handler.push(msg.GetDescriptor());
            codec->base_encode(&head_bits, msg, MessageHandler::HEAD);
            codec->base_encode(&body_bits, msg, MessageHandler::BODY);
        }
        else
        {
            throw(DCCLException("Failed to find (goby.msg).dccl.codec `" + desc->options().GetExtension(goby::msg).dccl().codec() + "`"));
        }
        
        // given header of not even byte size (e.g. 01011), make even byte size (e.g. 00001011)
        unsigned head_byte_size = ceil_bits2bytes(head_bits.size());
        unsigned head_bits_diff = head_byte_size * BITS_IN_BYTE - (head_bits.size());
        head_bits.resize(head_bits.size() + head_bits_diff);

        std::string head_bytes = head_bits.to_byte_string();
        std::string body_bytes = body_bits.to_byte_string();
        
        glog.is(DEBUG2) && glog << group(glog_encode_group_) << "Head bytes (bits): " << head_bytes.size() << "(" << head_bits.size()
                                << "), body bytes (bits): " <<  body_bytes.size() << "(" << body_bits.size() << ")" <<  std::endl;
        glog.is(DEBUG3) && glog << group(glog_encode_group_) << "Unencrypted Head (bin): " << head_bits << std::endl;
        glog.is(DEBUG3) && glog << group(glog_encode_group_) << "Unencrypted Body (bin): " << body_bits << std::endl;
        glog.is(DEBUG3) && glog << group(glog_encode_group_) << "Unencrypted Head (hex): " << hex_encode(head_bytes) << std::endl;
        glog.is(DEBUG3) && glog << group(glog_encode_group_) << "Unencrypted Body (hex): " << hex_encode(body_bytes) << std::endl;
        
        if(!crypto_key_.empty())
            encrypt(&body_bytes, head_bytes);

        glog.is(DEBUG3) && glog << group(glog_encode_group_) << "Encrypted Body (hex): " << hex_encode(body_bytes) << std::endl;


        glog.is(DEBUG1) && glog << group(glog_encode_group_) << "Successfully encoded message of type: " << desc->full_name() << std::endl;

        *bytes =  head_bytes + body_bytes;
    }
    catch(std::exception& e)
    {
        std::stringstream ss;
        
        ss << "Message " << desc->full_name() << " failed to encode. Reason: " << e.what();

        glog.is(DEBUG1) && glog << group(glog_encode_group_) << warn << ss.str() << std::endl;  
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
    fixed_header_bits.from_byte_string(bytes.substr(0, std::ceil(double(id_max_size) / BITS_IN_BYTE)));

    Bitset these_bits(&fixed_header_bits);
    these_bits.get_more_bits(id_min_size);
    
    return id_codec_[current_id_codec_]->decode(&these_bits);
}

void goby::acomms::DCCLCodec::decode(const std::string& bytes, google::protobuf::Message* msg)
{
    try
    {
        unsigned id = id_from_encoded(bytes);

        glog.is(DEBUG1) && glog << group(glog_decode_group_) << "Began decoding message of id: " << id << std::endl;
        
        if(!id2desc_.count(id))
            throw(DCCLException("Message id " + as<std::string>(id) + " has not been validated. Call validate() before decoding this type."));

        const Descriptor* desc = msg->GetDescriptor();
        
        glog.is(DEBUG1) && glog << group(glog_decode_group_) << "Type name: " << desc->full_name() << std::endl;
        
        boost::shared_ptr<DCCLFieldCodecBase> codec = DCCLFieldCodecManager::find(desc);
        boost::shared_ptr<FromProtoCppTypeBase> helper = DCCLTypeHelper::find(desc);

        
        unsigned head_size_bits, body_size_bits;
        codec->base_max_size(&head_size_bits, desc, MessageHandler::HEAD);
        codec->base_max_size(&body_size_bits, desc, MessageHandler::BODY);
        head_size_bits += id_codec_[current_id_codec_]->size(id);
        
        unsigned head_size_bytes = ceil_bits2bytes(head_size_bits);
        unsigned body_size_bytes = ceil_bits2bytes(body_size_bits);
    
        glog.is(DEBUG2) && glog << group(glog_decode_group_) << "Head bytes (bits): " << head_size_bytes << "(" << head_size_bits
                                << "), max body bytes (bits): " << body_size_bytes << "(" << body_size_bits << ")" <<  std::endl;

        std::string head_bytes = bytes.substr(0, head_size_bytes);
        std::string body_bytes = bytes.substr(head_size_bytes);


        glog.is(DEBUG3) && glog << group(glog_decode_group_) << "Encrypted Body (hex): " << hex_encode(body_bytes) << std::endl;


        if(!crypto_key_.empty())
            decrypt(&body_bytes, head_bytes);

        glog.is(DEBUG3) && glog << group(glog_decode_group_) << "Unencrypted Head (hex): " << hex_encode(head_bytes) << std::endl;
        glog.is(DEBUG3) && glog << group(glog_decode_group_) << "Unencrypted Body (hex): " << hex_encode(body_bytes) << std::endl;

        
        Bitset head_bits, body_bits;
        head_bits.from_byte_string(head_bytes);
        body_bits.from_byte_string(body_bytes);
    
        glog.is(DEBUG3) && glog << group(glog_decode_group_) << "Unencrypted Head (bin): " << head_bits << std::endl;
        glog.is(DEBUG3) && glog << group(glog_decode_group_) << "Unencrypted Body (bin): " << body_bits << std::endl;

        // shift off ID bits
        head_bits >>= id_codec_[current_id_codec_]->size(id);

        glog.is(DEBUG3) && glog << group(glog_decode_group_) << "Unencrypted Head after ID bits removal (bin): " << head_bits << std::endl;
        
        if(codec)
        {
            MessageHandler msg_handler;
            msg_handler.push(msg->GetDescriptor());
            
            codec->base_decode(&head_bits, msg, MessageHandler::HEAD);
            glog.is(DEBUG2) && glog << group(glog_decode_group_) << "after header decode, message is: " << *msg << std::endl;
            codec->base_decode(&body_bits, msg, MessageHandler::BODY);
            glog.is(DEBUG2) && glog << group(glog_decode_group_) << "after header & body decode, message is: " << *msg << std::endl;
        }
        else
        {
            throw(DCCLException("Failed to find (goby.msg).dccl.codec `" + desc->options().GetExtension(goby::msg).dccl().codec() + "`"));
        }

        glog.is(DEBUG1) && glog << group(glog_decode_group_) << "Successfully decoded message of type: " << desc->full_name() << std::endl;
    }
    catch(std::exception& e)
    {
        std::stringstream ss;
        
        ss << "Message " << hex_encode(bytes) <<  " failed to decode. Reason: " << e.what() << std::endl;

        glog.is(DEBUG1) && glog << group(glog_decode_group_) << warn << ss.str() << std::endl;  
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
        codec->base_max_size(&head_size_bits, desc, MessageHandler::HEAD);
        codec->base_max_size(&body_size_bits, desc, MessageHandler::BODY);
        head_size_bits += id_codec_[current_id_codec_]->size(dccl_id);
        
        const unsigned byte_size = ceil_bits2bytes(head_size_bits) + ceil_bits2bytes(body_size_bits);

        if(byte_size > desc->options().GetExtension(goby::msg).dccl().max_bytes())
            throw(DCCLException("Actual maximum size of message exceeds allowed maximum (dccl.max_bytes). Tighten bounds, remove fields, improve codecs, or increase the allowed dccl.max_bytes"));
        
        codec->base_validate(desc, MessageHandler::HEAD);
        codec->base_validate(desc, MessageHandler::BODY);

        if(id2desc_.count(dccl_id) && desc != id2desc_.find(dccl_id)->second)
            throw(DCCLException("`dccl.id` " + as<std::string>(dccl_id) + " is already in use by Message " + id2desc_.find(dccl_id)->second->full_name() + ": " + as<std::string>(id2desc_.find(dccl_id)->second)));
        else
            id2desc_.insert(std::make_pair(id(desc), desc));

        glog.is(DEBUG1) && glog << group(glog_encode_group_) << "Successfully validated message of type: " << desc->full_name() << std::endl;

    }
    catch(DCCLException& e)
    {
        try
        {
            info(desc, &glog);
        }
        catch(DCCLException& e)
        { }
        
        glog.is(DEBUG1) && glog << group(glog_encode_group_) << "Message " << desc->full_name() << ": " << desc << " failed validation. Reason: "
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
    codec->base_size(&head_size_bits, msg, MessageHandler::HEAD);
    head_size_bits += id_codec_[current_id_codec_]->size(dccl_id);
    
    unsigned body_size_bits;
    codec->base_size(&body_size_bits, msg, MessageHandler::BODY);
    
    const unsigned head_size_bytes = ceil_bits2bytes(head_size_bits);
    const unsigned body_size_bytes = ceil_bits2bytes(body_size_bits);

    return head_size_bytes + body_size_bytes;
}



void goby::acomms::DCCLCodec::run_hooks(const google::protobuf::Message& msg)
{
    const Descriptor* desc = msg.GetDescriptor();

    boost::shared_ptr<DCCLFieldCodecBase> codec = DCCLFieldCodecManager::find(desc);
    
    codec->base_run_hooks(msg, MessageHandler::HEAD);
    codec->base_run_hooks(msg, MessageHandler::BODY);
}


void goby::acomms::DCCLCodec::info(const google::protobuf::Descriptor* desc, std::ostream* os) const
{
    try
    {   
        boost::shared_ptr<DCCLFieldCodecBase> codec = DCCLFieldCodecManager::find(desc);

        unsigned config_head_bit_size, body_bit_size;
        codec->base_max_size(&config_head_bit_size, desc, MessageHandler::HEAD);
        codec->base_max_size(&body_bit_size, desc, MessageHandler::BODY);

        unsigned dccl_id = id(desc);
        const unsigned id_bit_size = id_codec_.find(current_id_codec_)->second->size(dccl_id);
        const unsigned bit_size = id_bit_size + config_head_bit_size + body_bit_size;

        
        const unsigned byte_size = ceil_bits2bytes(config_head_bit_size + id_bit_size) + ceil_bits2bytes(body_bit_size);
        
        const unsigned allowed_byte_size = desc->options().GetExtension(goby::msg).dccl().max_bytes();
        const unsigned allowed_bit_size = allowed_byte_size * BITS_IN_BYTE;
        
        *os << "= Begin " << desc->full_name() << " =\n"
            << "Actual maximum size of message: " << byte_size << " bytes / "
            << byte_size*BITS_IN_BYTE  << " bits [dccl.id head: " << id_bit_size
            << ", user head: " << config_head_bit_size << ", body: "
            << body_bit_size << ", padding: " << byte_size * BITS_IN_BYTE - bit_size << "]\n"
            << "Allowed maximum size of message: " << allowed_byte_size << " bytes / "
            << allowed_bit_size << " bits\n";

        *os << "== Begin Header ==" << std::endl;
        codec->base_info(os, desc, MessageHandler::HEAD);
        *os << "== End Header ==" << std::endl;
        *os << "== Begin Body ==" << std::endl;
        codec->base_info(os, desc, MessageHandler::BODY);
        *os << "== End Body ==" << std::endl;
        
        *os << "= End " << desc->full_name() << " =" << std::endl;
    }
    catch(DCCLException& e)
    {
        glog.is(DEBUG1) && glog << group(glog_encode_group_) << warn << "Message " << desc->full_name() << " cannot provide information due to invalid configuration. Reason: " << e.what() << std::endl;
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

void goby::acomms::DCCLCodec::load_shared_library_codecs(void* dl_handle)
{
    if(!dl_handle)
        throw(DCCLException("Null shared library handle passed to load_shared_library_codecs"));
    
    // load any shared library codecs
    void (*dccl_load_ptr)(goby::acomms::DCCLCodec*);
    dccl_load_ptr = (void (*)(goby::acomms::DCCLCodec*)) dlsym(dl_handle, "goby_dccl_load");
    if(dccl_load_ptr)
        (*dccl_load_ptr)(this);
}



void goby::acomms::DCCLCodec::process_cfg()
{
    if(!crypto_key_.empty())
        crypto_key_.clear();
    if(cfg_.has_crypto_passphrase())
    {
#ifdef HAS_CRYPTOPP
        using namespace CryptoPP;
        
        SHA256 hash;
        StringSource unused(cfg_.crypto_passphrase(), true, new HashFilter(hash, new StringSink(crypto_key_)));
        
        glog.is(DEBUG1) && glog << group(glog_encode_group_) << "Cryptography enabled with given passphrase" << std::endl;
#else
        glog.is(DEBUG1) && glog << group(glog_encode_group_) << warn << "Cryptography disabled because Goby was compiled without support of Crypto++. Install Crypto++ and recompile to enable cryptography." << std::endl;
#endif
    }
    else
    {
        glog.is(DEBUG1) && glog << group(glog_encode_group_) << "Cryptography disabled, set crypto_passphrase to enable." << std::endl;
    }

    switch(cfg_.id_codec())
    {
        default:
        case protobuf::DCCLConfig::VARINT:
        {
            add_id_codec<DCCLDefaultIdentifierCodec>(DEFAULT_CODEC_NAME);
            set_id_codec(DEFAULT_CODEC_NAME);
            break;
        }
        
        case protobuf::DCCLConfig::LEGACY_CCL:
        {
            add_id_codec<LegacyCCLIdentifierCodec>("_ccl");
            set_id_codec("_ccl");
            
            DCCLFieldCodecManager::add<LegacyCCLLatLonCompressedCodec>("_ccl_latloncompressed");
            DCCLFieldCodecManager::add<LegacyCCLFixAgeCodec>("_ccl_fix_age");
            DCCLFieldCodecManager::add<LegacyCCLTimeDateCodec>("_ccl_time_date");
            DCCLFieldCodecManager::add<LegacyCCLHeadingCodec>("_ccl_heading");
            DCCLFieldCodecManager::add<LegacyCCLDepthCodec>("_ccl_depth");
            DCCLFieldCodecManager::add<LegacyCCLVelocityCodec>("_ccl_velocity");
            DCCLFieldCodecManager::add<LegacyCCLWattsCodec>("_ccl_watts");
            DCCLFieldCodecManager::add<LegacyCCLGFIPitchOilCodec>("_ccl_gfi_pitch_oil");
            DCCLFieldCodecManager::add<LegacyCCLSpeedCodec>("_ccl_speed");
            DCCLFieldCodecManager::add<LegacyCCLHiResAltitudeCodec>("_ccl_hires_altitude");
            DCCLFieldCodecManager::add<LegacyCCLTemperatureCodec>("_ccl_temperature");
            DCCLFieldCodecManager::add<LegacyCCLSalinityCodec>("_ccl_salinity");
            DCCLFieldCodecManager::add<LegacyCCLSoundSpeedCodec>("_ccl_sound_speed");
            
            validate<goby::acomms::protobuf::CCLMDATEmpty>();
            validate<goby::acomms::protobuf::CCLMDATRedirect>();
            validate<goby::acomms::protobuf::CCLMDATBathy>();
            validate<goby::acomms::protobuf::CCLMDATCTD>();
            validate<goby::acomms::protobuf::CCLMDATState>();
            validate<goby::acomms::protobuf::CCLMDATCommand>();
            validate<goby::acomms::protobuf::CCLMDATError>();
        }
    }
    
}

void goby::acomms::DCCLCodec::info_all(std::ostream* os) const
{
    *os << "=== Begin DCCLCodec ===" << "\n";
    *os << id2desc_.size() << " messages loaded.\n";            
            
    for(std::map<int32, const google::protobuf::Descriptor*>::const_iterator it = id2desc_.begin(), n = id2desc_.end(); it != n; ++it)
        info(it->second, os);
                
    *os << "=== End DCCLCodec ===";
}
