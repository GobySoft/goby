// Copyright 2009-2014 Toby Schneider (https://launchpad.net/~tes)
//                     GobySoft, LLC (2013-)
//                     Massachusetts Institute of Technology (2007-2014)
//                     Goby Developers Team (https://launchpad.net/~goby-dev)
// 
//
// This file is part of the Goby Underwater Autonomy Project Libraries
// ("The Goby Libraries").
//
// The Goby Libraries are free software: you can redistribute them and/or modify
// them under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 2.1 of the License, or
// (at your option) any later version.
//
// The Goby Libraries are distributed in the hope that they will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with Goby.  If not, see <http://www.gnu.org/licenses/>.


#ifndef DCCLCCLCOMPATIBILITY20120426H
#define DCCLCCLCOMPATIBILITY20120426H

#include "dccl_field_codec_default.h"
#include "goby/acomms/acomms_constants.h"
#include "goby/acomms/protobuf/ccl.pb.h"
#include "goby/acomms/protobuf/ccl_extensions.pb.h"

extern "C"
{
    void goby_dccl_load(goby::acomms::DCCLCodec* dccl);
}


namespace goby
{
    namespace acomms
    {   
        class LegacyCCLIdentifierCodec : public DCCLDefaultIdentifierCodec
        {
          private:
            Bitset encode()
            { return encode(0); }
            
            Bitset encode(const uint32& wire_value)
            {
                if((wire_value & 0xFFFF0000) == CCL_DCCL_ID_PREFIX)
                {
                    // CCL message
                    return Bitset(BITS_IN_BYTE, wire_value & 0x0000FFFF);
                }
                else
                {
                    // DCCL message
                    return DCCLDefaultIdentifierCodec::encode(wire_value).prepend(
                        Bitset(BITS_IN_BYTE, DCCL_CCL_HEADER));
                }
                
            }
                
            uint32 decode(Bitset* bits)
            {
                unsigned ccl_id = bits->to_ulong();
                
                if(ccl_id == DCCL_CCL_HEADER)
                {
                    // DCCL message
                    bits->get_more_bits(DCCLDefaultIdentifierCodec::min_size());
                    (*bits) >>= BITS_IN_BYTE;
                    return DCCLDefaultIdentifierCodec::decode(bits);
                }
                else
                {
                    // CCL message
                    return CCL_DCCL_ID_PREFIX + ccl_id;
                }
            }
            
            unsigned size()
            { return size(0); }
            
            unsigned size(const uint32& field_value)
            {
                if((field_value & 0xFFFF0000) == CCL_DCCL_ID_PREFIX)
                {
                    // CCL message
                    return BITS_IN_BYTE;
                }
                else
                {
                    return BITS_IN_BYTE +
                        DCCLDefaultIdentifierCodec::size(field_value);
                }
            }
            
            unsigned max_size()
            { return BITS_IN_BYTE + DCCLDefaultIdentifierCodec::max_size(); }

            unsigned min_size()
            { return BITS_IN_BYTE; }

            // prefixes (goby.msg).dccl.id to indicate that this DCCL
            // message is an encoding of a legacy CCL message
            enum { CCL_DCCL_ID_PREFIX = 0x0CC10000 };
        };

        class LegacyCCLLatLonCompressedCodec : public DCCLTypedFixedFieldCodec<double>
        {
          private:
            Bitset encode();
            Bitset encode(const double& wire_value);
            double decode(Bitset* bits);
            unsigned size();
            enum { LATLON_COMPRESSED_BYTE_SIZE = 3 };            
        };

        class LegacyCCLFixAgeCodec : public DCCLDefaultNumericFieldCodec<uint32>
        {
          private:
            Bitset encode()
            {
                return encode(max());
            }
            
            Bitset encode(const uint32& wire_value)
            {
                return DCCLDefaultNumericFieldCodec<uint32>::encode(
                    std::min<unsigned char>(max(), wire_value / SCALE_FACTOR));
            }
            
            uint32 decode(Bitset* bits)
            {
                return SCALE_FACTOR *
                    DCCLDefaultNumericFieldCodec<uint32>::decode(bits);
            }
                        
            double max() { return (1 << BITS_IN_BYTE) - 1; }
            double min() { return 0; }
            void validate() { }
            
            enum { SCALE_FACTOR = 4 };
            
        };
        
            
        class LegacyCCLTimeDateCodec : public DCCLTypedFixedFieldCodec<uint64>
        {
          private:
            Bitset encode();
            Bitset encode(const uint64& wire_value);
            uint64 decode(Bitset* bits);
            unsigned size();

            enum { MICROSECONDS_IN_SECOND = 1000000 };
            enum { TIME_DATE_COMPRESSED_BYTE_SIZE = 3 };
            
                
            
        };

        class LegacyCCLHeadingCodec : public DCCLTypedFixedFieldCodec<float>
        {
          private:
            Bitset encode() { return encode(0); }
            Bitset encode(const float& wire_value);
            float decode(Bitset* bits);
            unsigned size() { return BITS_IN_BYTE; }
        };


        class LegacyCCLHiResAltitudeCodec : public DCCLTypedFixedFieldCodec<float>
        {
          private:
            Bitset encode() { return encode(0); }
            Bitset encode(const float& wire_value);
            float decode(Bitset* bits);
            unsigned size()
            {
                return HI_RES_ALTITUDE_COMPRESSED_BYTE_SIZE*BITS_IN_BYTE;
            }
            enum { HI_RES_ALTITUDE_COMPRESSED_BYTE_SIZE = 2 };

        };

        
        class LegacyCCLDepthCodec : public DCCLTypedFixedFieldCodec<float>
        {
          private:
            Bitset encode() { return encode(0); }
            Bitset encode(const float& wire_value);
            float decode(Bitset* bits);
            unsigned size()
            {
                return DCCLFieldCodecBase::dccl_field_options().GetExtension(ccl).bit_size();
            }
            
            void validate()
            {
                DCCLFieldCodecBase::require(DCCLFieldCodecBase::dccl_field_options().GetExtension(ccl).has_bit_size(), "missing (goby.field).dccl.ccl.bit_size");
            }
            
        };

        class LegacyCCLVelocityCodec : public DCCLTypedFixedFieldCodec<float>
        {
          private:
            Bitset encode() { return encode(0); }
            Bitset encode(const float& wire_value);
            float decode(Bitset* bits);
            unsigned size() { return BITS_IN_BYTE; }
        };

        class LegacyCCLSpeedCodec : public DCCLTypedFixedFieldCodec<float>
        {
          private:
            Bitset encode() { return encode(0); }
            Bitset encode(const float& wire_value);
            float decode(Bitset* bits);
            unsigned size() { return BITS_IN_BYTE; }

            void validate()
            {
                DCCLFieldCodecBase::require(DCCLFieldCodecBase::dccl_field_options().GetExtension(ccl).has_thrust_mode_tag(), "missing (goby.field).dccl.ccl.thrust_mode_tag");
            }
        };


        
        class LegacyCCLWattsCodec : public DCCLTypedFixedFieldCodec<float>
        {
          private:
            Bitset encode() { return encode(0); }
            Bitset encode(const float& wire_value);
            float decode(Bitset* bits);
            unsigned size() { return BITS_IN_BYTE; }
        };

        class LegacyCCLGFIPitchOilCodec : public DCCLTypedFixedFieldCodec<protobuf::CCLMDATState::GFIPitchOil>
        {
          private:
            Bitset encode() { return encode(protobuf::CCLMDATState::GFIPitchOil()); }
            Bitset encode(const protobuf::CCLMDATState::GFIPitchOil& wire_value);
            protobuf::CCLMDATState::GFIPitchOil decode(Bitset* bits);
            unsigned size() { return GFI_PITCH_OIL_COMPRESSED_BYTE_SIZE*BITS_IN_BYTE; }
            enum { GFI_PITCH_OIL_COMPRESSED_BYTE_SIZE =2};
                        
        };

        class LegacyCCLSalinityCodec : public DCCLTypedFixedFieldCodec<float>
        {
          private:
            Bitset encode() { return encode(0); }
            Bitset encode(const float& wire_value);
            float decode(Bitset* bits);
            unsigned size() { return BITS_IN_BYTE; }
        };

        class LegacyCCLTemperatureCodec : public DCCLTypedFixedFieldCodec<float>
        {
          private:
            Bitset encode() { return encode(0); }
            Bitset encode(const float& wire_value);
            float decode(Bitset* bits);
            unsigned size() { return BITS_IN_BYTE; }
        };

        class LegacyCCLSoundSpeedCodec : public DCCLTypedFixedFieldCodec<float>
        {
          private:
            Bitset encode() { return encode(0); }
            Bitset encode(const float& wire_value);
            float decode(Bitset* bits);
            unsigned size() { return BITS_IN_BYTE; }
        };        
        
    }
}

#endif
