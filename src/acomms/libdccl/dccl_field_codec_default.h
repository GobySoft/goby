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

// implements DCCLFieldCodecBase for all the basic DCCL types

#ifndef DCCLFIELDCODECDEFAULT20110322H
#define DCCLFIELDCODECDEFAULT20110322H

#include <boost/utility.hpp>
#include <boost/type_traits.hpp>
#include <boost/static_assert.hpp>
#include <boost/bimap.hpp>

#include <google/protobuf/descriptor.h>

#include "goby/protobuf/dccl_option_extensions.pb.h"
#include "goby/util/as.h"
#include "goby/util/sci.h"
#include "goby/util/time.h"
#include "goby/acomms/acomms_constants.h"

#include "dccl_field_codec_default_message.h"
#include "dccl_field_codec_fixed.h"
#include "dccl_field_codec.h"

namespace goby
{
    namespace acomms
    {

        class DCCLDefaultIdentifierCodec : public DCCLTypedFieldCodec<uint32>
        {
          private:
            Bitset encode();
            Bitset encode(const uint32& wire_value);
            uint32 decode(Bitset* bits);
            unsigned size();
            unsigned size(const uint32& field_value);
            unsigned max_size();
            unsigned min_size();
            void validate() { }
            bool variable_size() { return true; }
            
            // maximum id we can fit in short or long header (MSB reserved to indicate
            // short or long header)
            enum { ONE_BYTE_MAX_ID = (1 << 7) - 1,
                   TWO_BYTE_MAX_ID = (1 << 15) - 1};
            
            enum { SHORT_FORM_ID_BYTES = 1,
                   LONG_FORM_ID_BYTES = 2 };
        };

        

        template<typename WireType, typename FieldType = WireType>
            class DCCLDefaultArithmeticFieldCodec : public DCCLTypedFixedFieldCodec<WireType, FieldType>
        {
          protected:

          virtual double max()
          { return DCCLFieldCodecBase::dccl_field_options().max(); }

          virtual double min()
          { return DCCLFieldCodecBase::dccl_field_options().min(); }

          virtual double precision()
          { return DCCLFieldCodecBase::dccl_field_options().precision(); }
            
          virtual void validate()
          {
              DCCLFieldCodecBase::require(DCCLFieldCodecBase::dccl_field_options().has_min(),
                      "missing (goby.field).dccl.min");
              DCCLFieldCodecBase::require(DCCLFieldCodecBase::dccl_field_options().has_max(),
                      "missing (goby.field).dccl.max");
          }

          Bitset encode()
          {
              return Bitset(size());
          }
          
          
          virtual Bitset encode(const WireType& value)
          {
              WireType wire_value = value;
              
              goby::glog << "starting encode of field with max " << max() << ", min " << min() << ", prec " << precision() << std::endl;
                
              if(wire_value < min() || wire_value > max())
                  return Bitset(size());
              
              wire_value = goby::util::unbiased_round(wire_value, precision());
              
              goby::glog << debug1 << "using value " << wire_value << std::endl;
              
              
              wire_value -= min();
              wire_value *= std::pow(10.0, precision());

              // "presence" value (0)
              if(!DCCLFieldCodecBase::this_field()->is_required())
                  wire_value += 1;
              
              return Bitset(size(), goby::util::as<unsigned long>(wire_value));
          }
          
          virtual WireType decode(Bitset* bits)
          {
              unsigned long t = bits->to_ulong();
              
              if(!DCCLFieldCodecBase::this_field()->is_required())
              {
                  if(!t) throw(DCCLNullValueException());
                  --t;
              }
              
              return goby::util::unbiased_round(
                  t / (std::pow(10.0, precision())) + min(), precision());
              
          }

          unsigned size()
          {
              // if not required field, leave one value for unspecified (always encoded as 0)
              const unsigned NULL_VALUE = DCCLFieldCodecBase::this_field()->is_required() ? 0 : 1;
              
              return std::ceil(std::log((max()-min())*std::pow(10.0, precision())+1 + NULL_VALUE)/std::log(2));
          }
            
        };

        class DCCLDefaultBoolCodec : public DCCLTypedFixedFieldCodec<bool>
        {
          private:
            Bitset encode(const bool& wire_value);
            Bitset encode();
            bool decode(Bitset* bits);
            unsigned size();
            void validate();
        };
        
        class DCCLDefaultStringCodec : public DCCLTypedFieldCodec<std::string>
        {
          private:
            Bitset encode();
            Bitset encode(const std::string& wire_value);
            std::string decode(Bitset* bits);
            unsigned size();
            unsigned size(const std::string& field_value);
            unsigned max_size();
            unsigned min_size();
            void validate();
            bool variable_size() { return true; }
          private:
            enum { MAX_STRING_LENGTH = 255 };
            
        };


        
        class DCCLDefaultBytesCodec : public DCCLTypedFieldCodec<std::string>
        {
          private:
            Bitset encode();
            Bitset encode(const std::string& wire_value);
            std::string decode(Bitset* bits);
            unsigned size();
            unsigned size(const std::string& field_value);
            unsigned max_size();
            unsigned min_size();
            bool variable_size() { return true; }
            void validate();
        };

        
        class DCCLDefaultEnumCodec
            : public DCCLDefaultArithmeticFieldCodec<int32, const google::protobuf::EnumValueDescriptor*>
        {
          public:
            int32 pre_encode(const google::protobuf::EnumValueDescriptor* const& field_value);
            const google::protobuf::EnumValueDescriptor* post_decode(const int32& wire_value);

          private:
            void validate() { }
            
            double max()
            {
                const google::protobuf::EnumDescriptor* e = this_field()->enum_type();
                return e->value_count()-1;
            }
            double min()
            { return 0; }
        };
        
        
        class DCCLTimeCodec : public DCCLDefaultArithmeticFieldCodec<int32, std::string>
        {
          public:
            int32 pre_encode(const std::string& field_value);
            std::string post_decode(const int32& wire_value);            
 
          private:
            void validate() { }

            double max() { return HOURS_IN_DAY*SECONDS_IN_HOUR; }
            double min() { return 0; }
            enum { HOURS_IN_DAY = 24 };
            enum { SECONDS_IN_HOUR = 3600 };

        };

        
        
        template<typename T>
            class DCCLStaticCodec : public DCCLTypedFixedFieldCodec<T>
        {
            Bitset encode(const T&)
            {
                return Bitset(size());
            }

            Bitset encode()
            {
                return Bitset(size());
            }

            T decode(Bitset* bits)
            {
                std::string t = DCCLFieldCodecBase::dccl_field_options().static_value();
                return t;
            }
            
            unsigned size()
            {
                return 0;
            }
            
            void validate()
            {
                DCCLFieldCodecBase::require(DCCLFieldCodecBase::dccl_field_options().has_static_value(), "missing (goby.field).dccl.static_value");
            }
            
        };
        class DCCLModemIdConverterCodec : public DCCLDefaultArithmeticFieldCodec<int32, std::string>
        {
          public:
            static void add(std::string platform, int32 id)
            {
                platform2modem_id_.left.insert(std::make_pair(platform, id));
            }
            
            int32 pre_encode(const std::string& field_value);
            std::string post_decode(const int32& wire_value);
            

          private:  
            void validate() { }
            double max() { return 30; }
            double min() { return 0; }

          private:
            static boost::bimap<std::string, int32> platform2modem_id_;
        };

        
    }
}

#endif
