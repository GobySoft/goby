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

#ifndef DCCL20091211H
#define DCCL20091211H

#include <string>
#include <set>
#include <map>
#include <ostream>
#include <stdexcept>
#include <vector>

#include <google/protobuf/descriptor.h>
#include <google/protobuf/dynamic_message.h>

#include <boost/shared_ptr.hpp>

#include "goby/util/time.h"
#include "goby/util/logger.h"
#include "goby/core/core_constants.h"
#include "goby/protobuf/dccl.pb.h"
#include "goby/protobuf/modem_message.pb.h"
#include "goby/acomms/acomms_helpers.h"
#include "protobuf_cpp_type_helpers.h"

#include "dccl_exception.h"
#include "dccl_field_codec.h"

/// The global namespace for the Goby project.
namespace goby
{
    namespace util
    { class FlexOstream; }
    

    /// Objects pertaining to acoustic communications (acomms)
    namespace acomms
    {
        class DCCLFieldCodec;
        
        /// \class DCCLCodec dccl.h goby/acomms/dccl.h
        /// \brief provides an API to the Dynamic CCL Codec.
        /// \ingroup acomms_api
        /// \sa  dccl.proto and modem_message.proto for definition of Google Protocol Buffers messages (namespace goby::acomms::protobuf).
        class DCCLCodec  // TODO(tes): Make singleton class
        {
          public:        
            /// \name Initialization Methods.
            ///
            /// These methods are intended to be called before doing any work with the class. However,
            /// they may be called at any time as desired.
            //@{

            /// \brief Set (and overwrite completely if present) the current configuration. (protobuf::DCCLConfig defined in dccl.proto)
            static void set_cfg(const protobuf::DCCLConfig& cfg);
            
            /// \brief Set (and merge "repeat" fields) the current configuration. (protobuf::DCCLConfig defined in dccl.proto)
            static void merge_cfg(const protobuf::DCCLConfig& cfg);
            

            class FieldCodecManager
            {
              public:
                template<typename T, template <typename T> class Codec>
                    void add(const char* name);

                const boost::shared_ptr<DCCLFieldCodec> find(
                    google::protobuf::FieldDescriptor::CppType cpp_type,
                    const std::string& name) const;
                
              private:
                typedef std::map<std::string, boost::shared_ptr<DCCLFieldCodec> > InsideMap;
                
                std::map<google::protobuf::FieldDescriptor::CppType, InsideMap> codecs_;
            };

            class CppTypeHelper
            {
              public:
                CppTypeHelper();
                const boost::shared_ptr<FromProtoCppTypeBase> find(google::protobuf::FieldDescriptor::CppType cpptype) const;
                
                boost::shared_ptr<FromProtoCppTypeBase> find(google::protobuf::FieldDescriptor::CppType cpptype);

                        
              private:
              typedef std::map<google::protobuf::FieldDescriptor::CppType,
              boost::shared_ptr<FromProtoCppTypeBase> > Map;
              Map map_;
            
            };
            
            /// \brief Messages must be validated before they can be encoded/decoded
            static bool validate(const google::protobuf::Message& msg);
            
            /// Registers the group names used for the FlexOstream logger
            static void add_flex_groups(util::FlexOstream* tout);
            
            //@}
        
            /// \name Codec functions.
            ///
            /// This is where the real work happens.
            //@{
            static bool encode(std::string* bytes, const google::protobuf::Message& msg);
            static bool decode(const std::string& bytes, google::protobuf::Message* msg); 
            //@}

            static const FieldCodecManager& codec_manager()
            { return codec_manager_; }
            static const CppTypeHelper& cpptype_helper()
            { return cpptype_helper_; }
            
            static void set_log(std::ostream* log)
            {
                log_ = log;

                util::FlexOstream* flex_log = dynamic_cast<util::FlexOstream*>(log);
                if(flex_log)
                    add_flex_groups(flex_log);
            }  
            /// \example acomms/chat/chat.cpp
            
          private:

            /// \name Constructors/Destructor
            //@{
            /// \brief Instantiate optionally with a ostream logger (for human readable output)
            /// \param log std::ostream object or FlexOstream to capture all humanly readable runtime and debug information (optional).
            DCCLCodec() { }
            
            /// destructor
            ~DCCLCodec() { }
            

            DCCLCodec(const DCCLCodec&);
            DCCLCodec& operator= (const DCCLCodec&);
            //@}

            static void set_default_codecs();
            
            
            static void encrypt(std::string* s, const std::string& nonce);
            static void decrypt(std::string* s, const std::string& nonce);
            static void process_cfg();
            
            static void bitset2string(const Bitset& bits, std::string* str)
            {
                str->resize(bits.num_blocks()); // resize the string to fit the bitset
                to_block_range(bits, str->rbegin());
            }
            
            static void string2bitset(Bitset* bits, const std::string& str)
            {
                bits->resize(str.size() * BITS_IN_BYTE);
                from_block_range(str.rbegin(), str.rend(), *bits);
            }
            
            // more efficient way to do ceil(total_bits / 8)
            // to get the number of bytes rounded up.
            enum { BYTE_MASK = 7 }; // 00000111
            static unsigned ceil_bits2bytes(unsigned bits)
            {
                return (bits& BYTE_MASK) ?
                    floor_bits2bytes(bits) + 1 :
                    floor_bits2bytes(bits);
            }
            static unsigned floor_bits2bytes(unsigned bits)
            { return bits >> 3; }
            
            
                   
          private:
            static const char* DEFAULT_CODEC_NAME;
            static std::ostream* log_;
            static protobuf::DCCLConfig cfg_;
            // SHA256 hash of the crypto passphrase
            static std::string crypto_key_;

            // maps protocol buffers CppTypes to a map of field codec names & codecs themselves
            static FieldCodecManager codec_manager_;
            static CppTypeHelper cpptype_helper_;
            
            static google::protobuf::DynamicMessageFactory message_factory_;

            static bool default_codecs_set_;
        };
    }
}

template<typename T, template <typename T> class Codec>
    void goby::acomms::DCCLCodec::FieldCodecManager::add(const char* name)
{
    const google::protobuf::FieldDescriptor::CppType cpp_type = ToProtoCppType<T>::as_enum();
    
    if(!codecs_[cpp_type].count(name))
    {
        codecs_[cpp_type][name] = boost::shared_ptr<DCCLFieldCodec>(new Codec<T>());
        if(log_)
            *log_ << "Adding codec " << name
                  << " for CppType "
                  << cpptype_helper_.find(cpp_type)->as_str()
                  << std::endl;
    }
    
    else
    {
        if(log_)
            *log_ << warn << "Ignoring duplicate codec " << name
                  << " for CppType "
                  << cpptype_helper_.find(cpp_type)->as_str()
                  << std::endl;
    }
}


#endif
