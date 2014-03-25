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


#include "dccl_ccl_compatibility.h"
#include "WhoiUtil.h"
#include <ctime>
#include "dccl.h"

// shared library load

extern "C"
{
    void goby_dccl_load(goby::acomms::DCCLCodec* dccl)
    {
        using namespace goby::acomms;
        dccl->add_id_codec<LegacyCCLIdentifierCodec>("_ccl");
        dccl->set_id_codec("_ccl");
            
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
        
        dccl->validate<goby::acomms::protobuf::CCLMDATEmpty>();
        dccl->validate<goby::acomms::protobuf::CCLMDATRedirect>();
        dccl->validate<goby::acomms::protobuf::CCLMDATBathy>();
        dccl->validate<goby::acomms::protobuf::CCLMDATCTD>();
        dccl->validate<goby::acomms::protobuf::CCLMDATState>();
        dccl->validate<goby::acomms::protobuf::CCLMDATCommand>();
        dccl->validate<goby::acomms::protobuf::CCLMDATError>();
    }
}

//
// LegacyCCLLatLonCompressedCodec
//

goby::acomms::Bitset goby::acomms::LegacyCCLLatLonCompressedCodec::encode()
{
    return encode(0);
}

goby::acomms::Bitset goby::acomms::LegacyCCLLatLonCompressedCodec::encode(const double& wire_value)
{
    LONG_AND_COMP encoded;
    encoded.as_long = 0;
    encoded.as_compressed = Encode_latlon(wire_value);
    return Bitset(size(), static_cast<unsigned long>(encoded.as_long));
}

double goby::acomms::LegacyCCLLatLonCompressedCodec::decode(Bitset* bits)
{    
    LONG_AND_COMP decoded;
    decoded.as_long = static_cast<long>(bits->to_ulong());
    return Decode_latlon(decoded.as_compressed);
}

unsigned goby::acomms::LegacyCCLLatLonCompressedCodec::size()
{
    return LATLON_COMPRESSED_BYTE_SIZE * BITS_IN_BYTE;
}

//
// LegacyCCLTimeDateCodec
//

goby::acomms::Bitset goby::acomms::LegacyCCLTimeDateCodec::encode()
{
    return encode(0);
}

goby::acomms::Bitset goby::acomms::LegacyCCLTimeDateCodec::encode(const uint64& wire_value)
{
    TIME_DATE_LONG encoded;
    encoded.as_long = 0;
    encoded.as_time_date = Encode_time_date(wire_value / MICROSECONDS_IN_SECOND);
    return Bitset(size(), static_cast<unsigned long>(encoded.as_long));
}

goby::uint64 goby::acomms::LegacyCCLTimeDateCodec::decode(Bitset* bits)
{
    TIME_DATE_LONG decoded;
    decoded.as_long = bits->to_ulong();
    short mon, day, hour, min, sec;
    Decode_time_date(decoded.as_time_date,
                     &mon, &day, &hour, &min, &sec);

    // assume current year
    boost::posix_time::ptime time_date(
        boost::gregorian::date(boost::gregorian::day_clock::universal_day().year(),
                               mon, day), 
        boost::posix_time::time_duration(hour,min,sec));
    
    return goby::util::as<uint64>(time_date);
}

unsigned goby::acomms::LegacyCCLTimeDateCodec::size()
{
    return TIME_DATE_COMPRESSED_BYTE_SIZE * BITS_IN_BYTE;
}


//
// LegacyCCLHeadingCodec
//
goby::acomms::Bitset goby::acomms::LegacyCCLHeadingCodec::encode(const float& wire_value)
{ return Bitset(size(), Encode_heading(wire_value)); } 

float goby::acomms::LegacyCCLHeadingCodec::decode(Bitset* bits)
{ return Decode_heading(bits->to_ulong()); }


//
// LegacyCCLDepthCodec
//
goby::acomms::Bitset goby::acomms::LegacyCCLDepthCodec::encode(const float& wire_value)
{ return Bitset(size(), Encode_depth(wire_value)); } 

float goby::acomms::LegacyCCLDepthCodec::decode(Bitset* bits)
{ return Decode_depth(bits->to_ulong()); }

//
// LegacyCCLVelocityCodec
//
goby::acomms::Bitset goby::acomms::LegacyCCLVelocityCodec::encode(const float& wire_value)
{
    return Bitset(size(), Encode_est_velocity(wire_value));
} 

float goby::acomms::LegacyCCLVelocityCodec::decode(Bitset* bits)
{ return Decode_est_velocity(bits->to_ulong()); }


//
// LegacyCCLSpeedCodec
//
goby::acomms::Bitset goby::acomms::LegacyCCLSpeedCodec::encode(const float& wire_value)
{
    const google::protobuf::Message* root = DCCLFieldCodecBase::root_message();
    const google::protobuf::FieldDescriptor* thrust_mode_field_desc =
        root->GetDescriptor()->FindFieldByNumber(
            DCCLFieldCodecBase::dccl_field_options().GetExtension(ccl).thrust_mode_tag());

    switch(root->GetReflection()->GetEnum(*root, thrust_mode_field_desc)->number())
    {
        default:
        case protobuf::CCLMDATRedirect::RPM:
            return Bitset(size(), Encode_speed(SPEED_MODE_RPM, wire_value));
            
        case protobuf::CCLMDATRedirect::METERS_PER_SECOND:
            return Bitset(size(), Encode_speed(SPEED_MODE_MSEC, wire_value));
    }
} 

float goby::acomms::LegacyCCLSpeedCodec::decode(Bitset* bits)
{
    const google::protobuf::Message* root = DCCLFieldCodecBase::root_message();
    const google::protobuf::FieldDescriptor* thrust_mode_field_desc =
        root->GetDescriptor()->FindFieldByNumber(
            DCCLFieldCodecBase::dccl_field_options().GetExtension(ccl).thrust_mode_tag());

    switch(root->GetReflection()->GetEnum(*root, thrust_mode_field_desc)->number())
    {
        default:
        case protobuf::CCLMDATRedirect::RPM:
            return Decode_speed(SPEED_MODE_RPM, bits->to_ulong());
            
        case protobuf::CCLMDATRedirect::METERS_PER_SECOND:
            return Decode_speed(SPEED_MODE_MSEC, bits->to_ulong());
    }
}



//
// LegacyCCLWattsCodec
//
goby::acomms::Bitset goby::acomms::LegacyCCLWattsCodec::encode(const float& wire_value)
{ return Bitset(size(), Encode_watts(wire_value, 1)); } 

float goby::acomms::LegacyCCLWattsCodec::decode(Bitset* bits)
{ return Decode_watts(bits->to_ulong()); }

//
// LegacyCCLGFIPitchOilCodec
//
goby::acomms::Bitset goby::acomms::LegacyCCLGFIPitchOilCodec::encode(const protobuf::CCLMDATState::GFIPitchOil& wire_value)
{
    return Bitset(size(), Encode_gfi_pitch_oil(wire_value.gfi(), wire_value.pitch(), wire_value.oil()));
}

goby::acomms::protobuf::CCLMDATState::GFIPitchOil goby::acomms::LegacyCCLGFIPitchOilCodec::decode(Bitset* bits)
{
    float gfi, pitch, oil;
    Decode_gfi_pitch_oil(bits->to_ulong(), &gfi, &pitch, &oil);
    protobuf::CCLMDATState::GFIPitchOil decoded;
    decoded.set_gfi(gfi);
    decoded.set_pitch(pitch);
    decoded.set_oil(oil);
    return decoded;
}

//
// LegacyCCLHiResAltitudeCodec
//
goby::acomms::Bitset goby::acomms::LegacyCCLHiResAltitudeCodec::encode(const float& wire_value)
{ return Bitset(size(), Encode_hires_altitude(wire_value)); } 

float goby::acomms::LegacyCCLHiResAltitudeCodec::decode(Bitset* bits)
{ return Decode_hires_altitude(bits->to_ulong()); }

//
// LegacyCCLSalinityCodec
//
goby::acomms::Bitset goby::acomms::LegacyCCLSalinityCodec::encode(const float& wire_value)
{ return Bitset(size(), Encode_salinity(wire_value)); } 

float goby::acomms::LegacyCCLSalinityCodec::decode(Bitset* bits)
{ return Decode_salinity(bits->to_ulong()); }

//
// LegacyCCLTemperatureCodec
//
goby::acomms::Bitset goby::acomms::LegacyCCLTemperatureCodec::encode(const float& wire_value)
{ return Bitset(size(), Encode_temperature(wire_value)); } 

float goby::acomms::LegacyCCLTemperatureCodec::decode(Bitset* bits)
{ return Decode_temperature(bits->to_ulong()); }

//
// LegacyCCLSoundSpeedCodec
//
goby::acomms::Bitset goby::acomms::LegacyCCLSoundSpeedCodec::encode(const float& wire_value)
{ return Bitset(size(), Encode_sound_speed(wire_value)); } 

float goby::acomms::LegacyCCLSoundSpeedCodec::decode(Bitset* bits)
{ return Decode_sound_speed(bits->to_ulong()); }
