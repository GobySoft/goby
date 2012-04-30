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

#include "dccl_ccl_compatibility.h"
#include "WhoiUtil.h"

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
    return goby::util::unbiased_round(Decode_latlon(decoded.as_compressed),
                                      DECIMAL_PRECISION);
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
    return Bitset();
}

goby::acomms::Bitset goby::acomms::LegacyCCLTimeDateCodec::encode(const double& wire_value)
{
    return Bitset();
}

double goby::acomms::LegacyCCLTimeDateCodec::decode(Bitset* bits)
{
    return 0;
}

unsigned goby::acomms::LegacyCCLTimeDateCodec::size()
{
    return 0;
}


