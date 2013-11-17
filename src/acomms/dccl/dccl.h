// Copyright 2009-2013 Toby Schneider (https://launchpad.net/~tes)
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


#ifndef DCCLCOMPAT20131116H
#define DCCLCOMPAT20131116H

#include "goby/util/binary.h"
#include "goby/acomms/protobuf/dccl.pb.h"
#include "goby/common/logger.h"

#include "dccl/codec.h"

/// The global namespace for the Goby project.
namespace goby
{
    namespace util
    { class FlexOstream; }
    
 
    /// Objects pertaining to acoustic communications (acomms)
    namespace acomms
    {
        typedef dccl::Exception DCCLException;
        typedef dccl::NullValueException DCCLNullValueException;

        typedef dccl::DefaultIdentifierCodec DCCLDefaultIdentifierCodec;
//        typedef dccl::DefaultNumericFieldCodec DCCLDefaultNumericFieldCodec;
        typedef dccl::DefaultBoolCodec DCCLDefaultBoolCodec;
        typedef dccl::DefaultStringCodec DCCLDefaultStringCodec;
        typedef dccl::DefaultBytesCodec DCCLDefaultBytesCodec;
        typedef dccl::DefaultEnumCodec DCCLDefaultEnumCodec;
//        typedef dccl::TimeCodec DCCLTimeCodec;
//        typedef dccl::StaticCodec DCCLStaticCodec;
        typedef dccl::DefaultMessageCodec DCCLDefaultMessageCodec;
//        typedef dccl::TypedFixedFieldCodec DCCLTypedFixedFieldCodec;
        typedef dccl::FieldCodecManager DCCLFieldCodecManager;
//        typedef dccl::RepeatedTypedFieldCodec DCCLRepeatedTypedFieldCodec;
//        typedef dccl::TypedFieldCodec DCCLTypedFieldCodec;
        typedef dccl::FieldCodecManager DCCLFieldCodecManager;
        typedef dccl::FromProtoCppTypeBase FromProtoCppTypeBase;
        //       typedef dccl::FromProtoType FromProtoType;
        // typedef dccl::FromProtoCppType FromProtoCppType;
        //typedef dccl::ToProtoCppType ToProtoCppType;
        typedef dccl::Bitset Bitset;
        typedef dccl::FieldCodecBase DCCLFieldCodecBase; 
        typedef dccl::TypeHelper DCCLTypeHelper;
        
        class DCCLCodec
        {
          public:
            /// \brief DCCLCodec is a singleton class; use this to get a pointer to the class.
            static DCCLCodec* get() 
            {
                // set these now so that the user has a chance of setting the logger
                if(!inst_)
                    inst_.reset(new DCCLCodec);

                return inst_.get();
            }
            
            void set_cfg(const protobuf::DCCLConfig& cfg)
            {
                cfg_.CopyFrom(cfg);
                process_cfg();
            }
                
            void merge_cfg(const protobuf::DCCLConfig& cfg)
            {
                cfg_.MergeFrom(cfg);
                process_cfg();
            }

            void load_shared_library_codecs(void* dl_handle)
            { codec_.load_library(dl_handle); }

            template<typename ProtobufMessage>
                void validate()
            { validate(ProtobufMessage::descriptor()); }

            template<typename ProtobufMessage>
                void info(std::ostream* os) const
            { info(ProtobufMessage::descriptor(), os); }
          
            void info_all(std::ostream* os) const
            { codec_.info_all(os); }

            template <typename ProtobufMessage>
                unsigned id() const
            { return id(ProtobufMessage::descriptor()); }
            
            unsigned size(const google::protobuf::Message& msg)
            { return codec_.size(msg); } 

            void encode(std::string* bytes, const google::protobuf::Message& msg, bool header_only = false)
            { codec_.encode(bytes, msg, header_only); }

            void decode(const std::string& bytes, google::protobuf::Message* msg, bool header_only = false)
            { codec_.decode(bytes, msg, header_only); }

            unsigned id_from_encoded(const std::string& bytes)
            { return codec_.id(bytes); }

            void validate(const google::protobuf::Descriptor* desc)
            { codec_.load(desc); }

            void validate_repeated(const std::list<const google::protobuf::Descriptor*>& descs)
            {
                BOOST_FOREACH(const google::protobuf::Descriptor* p, descs)
                    validate(p);
            }
            
            void info(const google::protobuf::Descriptor* desc, std::ostream* os) const
            { codec_.info(desc, os); }

            void info_repeated(const std::list<const google::protobuf::Descriptor*>& desc, std::ostream* os) const
            {
                BOOST_FOREACH(const google::protobuf::Descriptor* p, desc)
                    info(p, os);
            }

            unsigned id(const google::protobuf::Descriptor* desc) const {
                return desc->options().GetExtension(dccl::msg).id();
            }
            
            template<typename GoogleProtobufMessagePointer>
                unsigned size_repeated(const std::list<GoogleProtobufMessagePointer>& msgs)
            {
                unsigned out = 0;
                BOOST_FOREACH(const GoogleProtobufMessagePointer& msg, msgs)
                    out += size(*msg);
                return out;
            }

            template<typename GoogleProtobufMessagePointer>
                GoogleProtobufMessagePointer decode(const std::string& bytes, bool header_only = false)
                {
                    return codec_.decode<GoogleProtobufMessagePointer>(bytes, header_only);
                }
            
            
            template<typename GoogleProtobufMessagePointer>
                std::string encode_repeated(const std::list<GoogleProtobufMessagePointer>& msgs)
            {
                std::string out;
                BOOST_FOREACH(const GoogleProtobufMessagePointer& msg, msgs)
                {
                    std::string piece;
                    encode(&piece, *msg);
                    out += piece;
                }
    
                return out;
            }
            
            template<typename GoogleProtobufMessagePointer>
                std::list<GoogleProtobufMessagePointer> decode_repeated(const std::string& orig_bytes)
                {
                    std::string bytes = orig_bytes;
                    std::list<GoogleProtobufMessagePointer> out;
                    while(!bytes.empty())
                    {
                        try
                        {
                            out.push_back(decode<GoogleProtobufMessagePointer>(bytes));
                            unsigned last_size = size(*out.back());
                            glog.is(common::logger::DEBUG1) && glog  << "last message size was: " << last_size << std::endl;
                            bytes.erase(0, last_size);
                        }
                        catch(dccl::Exception& e)
                        {
                            if(out.empty())
                                throw(e);
                            else
                            {
                                glog.is(common::logger::WARN) && glog << "failed to decode " << goby::util::hex_encode(bytes) << " but returning parts already decoded"  << std::endl;
                                return out;
                            }
                        }        
                    }
                    return out;

                }

            //@}           

            

          private:

            // so we can use shared_ptr to hold the singleton
            template<typename T>
                friend void boost::checked_delete(T*);
            
            DCCLCodec()
            { }
            
            ~DCCLCodec() { }
            DCCLCodec(const DCCLCodec&);
            DCCLCodec& operator= (const DCCLCodec&);
            
            void process_cfg()
            {
                if(cfg_.has_crypto_passphrase())
                {
                    std::set<unsigned> skip_crypto_ids;
                    for(int i = 0, n = cfg_.skip_crypto_for_id_size(); i < n; ++i)
                        skip_crypto_ids.insert(cfg_.skip_crypto_for_id(i));
                    codec_.set_crypto_passphrase(cfg_.crypto_passphrase(),
                                                 skip_crypto_ids);
                }
            }

            
          private:
            static boost::shared_ptr<DCCLCodec> inst_;

            static std::string glog_encode_group_;
            static std::string glog_decode_group_;
            
            protobuf::DCCLConfig cfg_;

            dccl::Codec codec_;
        };

        inline std::ostream& operator<<(std::ostream& os, const DCCLCodec& codec)
        {
            codec.info_all(&os);
            return os;
        }
    }
}

#endif
