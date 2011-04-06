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
#include "dccl_type_helper.h"
#include "dccl_field_codec_manager.h"

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
            

            /// \brief Messages must be validated before they can be encoded/decoded
            static bool validate(const google::protobuf::Descriptor* desc);
            static void info(const google::protobuf::Descriptor* desc, std::ostream* os);

            
            /// Registers the group names used for the FlexOstream logger
            static void add_flex_groups(util::FlexOstream* tout);
            
            //@}
        
            /// \name Codec functions.
            ///
            /// This is where the real work happens.
            //@{
            static std::string encode(const google::protobuf::Message& msg);
            static boost::shared_ptr<google::protobuf::Message> decode(const std::string& bytes);
            //@}

            
            static void set_log(std::ostream* log)
            {
                log_ = log;

                util::FlexOstream* flex_log = dynamic_cast<util::FlexOstream*>(log);
                if(flex_log)
                    add_flex_groups(flex_log);
            }  

            static std::ostream* log_;
            static google::protobuf::DynamicMessageFactory message_factory_;


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

            static unsigned fixed_head_size()
            {
                return HEAD_CCL_ID_SIZE + HEAD_DCCL_ID_SIZE;
            }
            
            
            
          private:
            static const std::string DEFAULT_CODEC_NAME;

            static protobuf::DCCLConfig cfg_;
            // SHA256 hash of the crypto passphrase
            static std::string crypto_key_;
            
            // maps `dccl.id`s onto Message Descriptors
            static std::map<int32, const google::protobuf::Descriptor*> id2desc_;
            
            static bool default_codecs_set_;
        };
                                      
        
    }
}

#endif
