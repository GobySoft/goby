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


#ifndef DCCLFIELDCODECTYPED20120312H
#define DCCLFIELDCODECTYPED20120312H

#include "dccl_field_codec.h"

namespace goby
{
    namespace acomms
    {
        
        /// \brief if WireType == FieldType, we don't have to add any more virtual methods for converting between them.
        template <typename WireType, typename FieldType, class Enable = void> 
            class DCCLFieldCodecSelector : public DCCLFieldCodecBase
            { };
        
        // if not the same WireType and FieldType, add these extra methods to
        // handle them 
        /// \brief If WireType != FieldType, adds some more pure virtual methods to handle the type conversions (pre_encode() and post_decode()). If WireType == FieldType this class is not inherited and this pure virtual methods do not exist (and thus can be omitted in the child class).
        template <typename WireType, typename FieldType>
            class DCCLFieldCodecSelector<WireType, FieldType,
            typename boost::disable_if<boost::is_same<WireType, FieldType> >::type>
            : public DCCLFieldCodecBase
            {
              protected:
                /// \brief Convert from the FieldType representation (used in the Google Protobuf message) to the WireType representation (used with encode() and decode(), i.e. "on the wire").
                /// 
                /// \param field_value Value to convert
                /// \return Converted value
                virtual WireType pre_encode(const FieldType& field_value) = 0;

                /// \brief Convert from the WireType representation (used with encode() and decode(), i.e. "on the wire") to the FieldType representation (used in the Google Protobuf message).
                /// 
                /// \param wire_value Value to convert
                /// \return Converted value
                virtual FieldType post_decode(const WireType& wire_value) = 0;

              private:
                virtual void any_pre_encode(boost::any* wire_value,
                                            const boost::any& field_value) 
                {
                    try
                    {
                        if(!field_value.empty())
                            *wire_value = pre_encode(boost::any_cast<FieldType>(field_value));
                    }
                    catch(boost::bad_any_cast&)
                    {
                        throw(type_error("pre_encode", typeid(FieldType), field_value.type()));
                    }
                    catch(DCCLNullValueException&)
                    {
                        *wire_value = boost::any();
                    }
                }
          
                virtual void any_post_decode(const boost::any& wire_value,
                                             boost::any* field_value)
                {
                    try
                    {
                        if(!wire_value.empty())
                            *field_value = post_decode(boost::any_cast<WireType>(wire_value));
                    }
                    catch(boost::bad_any_cast&)
                    {
                        throw(type_error("post_decode", typeid(WireType), wire_value.type()));
                    }
                    catch(DCCLNullValueException&)
                    {
                        *field_value = boost::any();
                    }
                }
            };

        /// \brief Base class for static-typed (no boost::any) field encoders/decoders. Most user defined variable length codecs will start with this class. Use DCCLTypedFixedFieldCodec if your codec is fixed length (always uses the same number of bits on the wire).
        /// \ingroup dccl_api
        ///
        ///\tparam WireType the type used for the encode and decode functions. This can be any C++ type, and is often the same as FieldType, unless a type conversion should be performed. The reason for using a different WireType and FieldType should be clear from the DCCLDefaultEnumCodec which uses DCCLDefaultArithmeticFieldCodec to do all the numerical encoding / decoding while DCCLDefaultEnumCodec does the type conversion (pre_encode() and post_decode()).
        ///\tparam FieldType the type used in the Google Protobuf message that is exposed to the end-user DCCLCodec::decode(), DCCLCodec::encode(), etc. functions.
        template<typename WireType, typename FieldType = WireType>
            class DCCLTypedFieldCodec : public DCCLFieldCodecSelector<WireType, FieldType>
        {
          public:
          typedef WireType wire_type;
          typedef FieldType field_type;

          public:
          
          /// \brief Encode an empty field
          ///
          /// \return Bits represented the encoded field.
          virtual Bitset encode() = 0;

          /// \brief Encode a non-empty field 
          ///
          /// \param wire_value Value to encode.
          /// \return Bits represented the encoded field.
          virtual Bitset encode(const WireType& wire_value) = 0;

          /// \brief Decode a field. If the field is empty (i.e. was encoded using the zero-argument encode()), throw DCCLNullValueException to indicate this.
          ///
          /// \param bits Bits to use for decoding.
          /// \return the decoded value.
          virtual WireType decode(const Bitset& bits) = 0;

          /// \brief Calculate the size (in bits) of an empty field.
          ///
          /// \return the size (in bits) of the empty field.
          virtual unsigned size() = 0;

          /// \brief Calculate the size (in bits) of a non-empty field.
          ///
          /// \param wire_value Value to use when calculating the size of the field. If calculating the size requires encoding the field completely, cache the encoded value for a likely future call to encode() for the same wire_value.
          /// \return the size (in bits) of the field.
          virtual unsigned size(const FieldType& wire_value) = 0;
          
          private:
          unsigned any_size(const boost::any& field_value)
          {
              try
              { return field_value.empty() ? size() : size(boost::any_cast<FieldType>(field_value)); }
              catch(boost::bad_any_cast&)
              { throw(type_error("size", typeid(FieldType), field_value.type())); }
          }          
          
          void any_encode(Bitset* bits, const boost::any& wire_value)
          {
              try
              { *bits = wire_value.empty() ? encode() : encode(boost::any_cast<WireType>(wire_value)); }
              catch(boost::bad_any_cast&)
              { throw(type_error("encode", typeid(WireType), wire_value.type())); }
          }

          void any_decode(Bitset* bits, boost::any* wire_value)
          {
              any_decode_specific<WireType>(bits, wire_value);
          }

          template<typename T>
          typename boost::enable_if<boost::is_base_of<google::protobuf::Message, T>, void>::type
          any_decode_specific(Bitset* bits, boost::any* wire_value)
          {
              try
              {
                  google::protobuf::Message* msg = boost::any_cast<google::protobuf::Message* >(*wire_value);  
                  msg->CopyFrom(decode(*bits));
              }
              catch(DCCLNullValueException&)
              {
                  if(DCCLFieldCodecBase::this_field())
                      *wire_value = boost::any();
              }              
          }
          
          template<typename T>
          typename boost::disable_if<boost::is_base_of<google::protobuf::Message, T>, void>::type
          any_decode_specific(Bitset* bits, boost::any* wire_value)
          {
              try
              { *wire_value = decode(*bits); }
              catch(DCCLNullValueException&)
              { *wire_value = boost::any(); }              
          }

          
        };
        
    }
}

#endif
