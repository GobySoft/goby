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

#ifndef DCCLCCLCOMPATIBILITY20120426H
#define DCCLCCLCOMPATIBILITY20120426H

#include "dccl_field_codec_default.h"
#include "goby/acomms/acomms_constants.h"

namespace goby
{
    namespace acomms
    {        
        class LegacyCCLIdentifierCodec : public DCCLDefaultIdentifierCodec
        {
          private:
            Bitset encode()
            {
                return DCCLDefaultIdentifierCodec::encode().prepend(
                    Bitset(BITS_IN_BYTE, DCCL_CCL_HEADER));
            }
            
            Bitset encode(const uint32& wire_value)
            {
                return DCCLDefaultIdentifierCodec::encode(wire_value).prepend(
                    Bitset(BITS_IN_BYTE, DCCL_CCL_HEADER));
            }
                
            uint32 decode(Bitset* bits)
            {
                (*bits) >>= BITS_IN_BYTE;
                return DCCLDefaultIdentifierCodec::decode(bits);
            }
            
            unsigned size()
            { return BITS_IN_BYTE + DCCLDefaultIdentifierCodec::size(); }
            
            unsigned size(const uint32& field_value)
            { return BITS_IN_BYTE + DCCLDefaultIdentifierCodec::size(field_value); }
            
            unsigned max_size()
            { return BITS_IN_BYTE + DCCLDefaultIdentifierCodec::max_size(); }

            unsigned min_size()
            { return BITS_IN_BYTE + DCCLDefaultIdentifierCodec::min_size(); }

        };

    }
}

#endif
