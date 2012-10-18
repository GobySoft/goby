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


// implements DCCLFieldCodecBase for all the basic DCCL types

#ifndef DCCLFIELDCODECDEFAULT20110322H
#define DCCLFIELDCODECDEFAULT20110322H

#include <boost/utility.hpp>
#include <boost/type_traits.hpp>
#include <boost/static_assert.hpp>
#include <boost/bimap.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/numeric/conversion/bounds.hpp>

#include <google/protobuf/descriptor.h>

#include "goby/common/protobuf/acomms_option_extensions.pb.h"
#include "goby/util/as.h"
#include "goby/util/sci.h"
#include "goby/common/time.h"
#include "goby/acomms/acomms_constants.h"

#include "dccl_field_codec_default_message.h"
#include "dccl_field_codec_fixed.h"
#include "dccl_field_codec.h"

namespace goby
{
    namespace acomms
    {
        
        /// \brief Provides the default 1 byte or 2 byte DCCL ID codec
        class DCCLDefaultIdentifierCodec : public DCCLTypedFieldCodec<uint32>
        {
          protected:
            virtual Bitset encode();
            virtual Bitset encode(const uint32& wire_value);
            virtual uint32 decode(Bitset* bits);
            virtual unsigned size();
            virtual unsigned size(const uint32& wire_value);
            virtual unsigned max_size();
            virtual unsigned min_size();
            virtual void validate() { }

          private:
            unsigned this_size(const uint32& wire_value);
            // maximum id we can fit in short or long header (MSB reserved to indicate
            // short or long header)
            enum { ONE_BYTE_MAX_ID = (1 << 7) - 1,
                   TWO_BYTE_MAX_ID = (1 << 15) - 1};
            
            enum { SHORT_FORM_ID_BYTES = 1,
                   LONG_FORM_ID_BYTES = 2 };
        };

        

        /// \brief Provides a basic bounded arbitrary length numeric (double, float, uint32, uint64, int32, int64) encoder.
        ///
        /// Takes ceil(log2((max-min)*10^precision)+1) bits for required fields, ceil(log2((max-min)*10^precision)+2) for optional fields.
        template<typename WireType, typename FieldType = WireType>
            class DCCLDefaultNumericFieldCodec : public DCCLTypedFixedFieldCodec<WireType, FieldType>
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


              // ensure given max and min fit within WireType ranges
              DCCLFieldCodecBase::require(min() >= boost::numeric::bounds<WireType>::lowest(),
                                          "(goby.field).dccl.min must be >= minimum of this field type.");
              DCCLFieldCodecBase::require(max() <= boost::numeric::bounds<WireType>::highest(),
                                          "(goby.field).dccl.max must be <= maximum of this field type.");
          }

          Bitset encode()
          {
              return Bitset(size());
          }
          
          
          virtual Bitset encode(const WireType& value)
          {
              WireType wire_value = value;
                
              if(wire_value < min() || wire_value > max())
                  return Bitset(size());
              
              
//              goby::glog.is(common::logger::DEBUG2) && goby::glog << group(DCCLCodec::glog_encode_group()) << "(DCCLDefaultNumericFieldCodec) Encoding using wire value (=field value) " << wire_value << std::endl;
              
              wire_value -= min();
              wire_value *= std::pow(10.0, precision());

              // "presence" value (0)
              if(!DCCLFieldCodecBase::this_field()->is_required())
                  wire_value += 1;

              wire_value = goby::util::unbiased_round(wire_value, 0);
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
              
              WireType return_value = goby::util::unbiased_round(
                  t / (std::pow(10.0, precision())) + min(), precision());
              
//              goby::glog.is(common::logger::DEBUG2) && goby::glog << group(DCCLCodec::glog_decode_group()) << "(DCCLDefaultNumericFieldCodec) Decoding received wire value (=field value) " << return_value << std::endl;

              return return_value;
              
          }

          unsigned size()
          {
              // if not required field, leave one value for unspecified (always encoded as 0)
              const unsigned NULL_VALUE = DCCLFieldCodecBase::this_field()->is_required() ? 0 : 1;
              
              return util::ceil_log2((max()-min())*std::pow(10.0, precision())+1 + NULL_VALUE);
          }
            
        };

        /// \brief Provides a bool encoder. Uses 1 bit if field is `required`, 2 bits if `optional`
        ///
        /// [presence bit (0 bits if required, 1 bit if optional)][value (1 bit)]
        class DCCLDefaultBoolCodec : public DCCLTypedFixedFieldCodec<bool>
        {
          private:
            Bitset encode(const bool& wire_value);
            Bitset encode();
            bool decode(Bitset* bits);
            unsigned size();
            void validate();
        };
        
        /// \brief Provides an variable length ASCII string encoder. Can encode strings up to 255 bytes by using a length byte preceeding the string.
        ///
        /// [length of following string (1 byte)][string (0-255 bytes)]
        class DCCLDefaultStringCodec : public DCCLTypedFieldCodec<std::string>
        {
          private:
            Bitset encode();
            Bitset encode(const std::string& wire_value);
            std::string decode(Bitset* bits);
            unsigned size();
            unsigned size(const std::string& wire_value);
            unsigned max_size();
            unsigned min_size();
            void validate();
          private:
            enum { MAX_STRING_LENGTH = 255 };
            
        };


        /// \brief Provides an fixed length byte string encoder.        
        class DCCLDefaultBytesCodec : public DCCLTypedFieldCodec<std::string>
        {
          private:
            Bitset encode();
            Bitset encode(const std::string& wire_value);
            std::string decode(Bitset* bits);
            unsigned size();
            unsigned size(const std::string& wire_value);
            unsigned max_size();
            unsigned min_size();
            void validate();
        };

        /// \brief Provides an enum encoder. This converts the enumeration to an integer (based on the enumeration <i>index</i> (<b>not</b> its <i>value</i>) and uses DCCLDefaultNumericFieldCodec to encode the integer.
        class DCCLDefaultEnumCodec
            : public DCCLDefaultNumericFieldCodec<int32, const google::protobuf::EnumValueDescriptor*>
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
        
        
        /// \brief Encodes time of day (second precision) for times represented by the string representation of boost::posix_time::ptime (e.g. obtained from goby_time<std::string>()).
        ///
        /// \tparam TimeType A type representing time: See the various specializations of goby_time() for allowed types.
        template<typename TimeType>
            class DCCLTimeCodec : public DCCLDefaultNumericFieldCodec<int32, TimeType>
        {
          public:
            
            goby::int32 pre_encode(const TimeType& field_value)
            {
                return util::as<boost::posix_time::ptime>(field_value).time_of_day().total_seconds();
            }


            TimeType post_decode(const int32& wire_value)
            {
                using namespace boost::posix_time;
                using namespace boost::gregorian;
        
                ptime now = common::goby_time();
                date day_sent;
                // if message is from part of the day removed from us by 12 hours, we assume it
                // was sent yesterday
                if(abs(now.time_of_day().total_seconds() - double(wire_value)) > hours(12).total_seconds())
                    day_sent = now.date() - days(1);
                else // otherwise figure it was sent today
                    day_sent = now.date();
                
                // this logic will break if there is a separation between message sending and
                // message receipt of greater than 1/2 day (twelve hours)               
                return util::as<TimeType>(ptime(day_sent,seconds(wire_value)));
            }
 
          private:
            void validate() { }

            double max() { return HOURS_IN_DAY*SECONDS_IN_HOUR; }
            double min() { return 0; }
            enum { HOURS_IN_DAY = 24 };
            enum { SECONDS_IN_HOUR = 3600 };

        };
        
        
        /// \brief Placeholder codec that takes no space on the wire (0 bits).
        template<typename T>
            class DCCLStaticCodec : public DCCLTypedFixedFieldCodec<T>
        {
            Bitset encode(const T&)
            { return Bitset(size()); }

            Bitset encode()
            { return Bitset(size()); }

            T decode(Bitset* bits)
            {
                std::string t = DCCLFieldCodecBase::dccl_field_options().static_value();
                return util::as<T>(t);
            }
            
            unsigned size()
            { return 0; }
            
            void validate()
            {
                DCCLFieldCodecBase::require(DCCLFieldCodecBase::dccl_field_options().has_static_value(), "missing (goby.field).dccl.static_value");
            }
            
        };

        
        /// \brief Codec that converts string names (e.g. "AUV-Unicorn") to integer MAC addresses (modem ID) and encodes the modem ID using DCCLDefaultNumericFieldCodec. The conversion is done using a lookup table.
        class DCCLModemIdConverterCodec : public DCCLDefaultNumericFieldCodec<int32, std::string>
        {
          public:
            /// \brief Add an entry to the lookup table used for conversions. 
            static void add(std::string platform, int32 id)
            {
                boost::to_lower(platform);
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
