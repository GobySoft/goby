// copyright 2009-2011 t. schneider tes@mit.edu
// 
// this file is part of goby-acomms, a collection of libraries for acoustic underwater networking
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

#ifndef AcommsConstants20091122H
#define AcommsConstants20091122H

#include <string>
#include <limits>
#include <bitset>

#include "goby/core/core_constants.h"

namespace goby
{
    namespace acomms
    {
        const unsigned BITS_IN_BYTE = 8;
        // one hex char is a nibble (4 bits), two nibbles per byte
        const unsigned NIBS_IN_BYTE = 2;

        /// special modem id for the broadcast destination - no one is assigned this address. Analogous to 192.168.1.255 on a 192.168.1.0 subnet
        const int BROADCAST_ID = 0;
        /// special modem id used internally to goby-acomms for indicating that the MAC layer (libamac) is agnostic to the next destination. The next destination is thus set by the data provider (typically libqueue).
        const int QUERY_DESTINATION_ID = -1;        

        const unsigned char DCCL_CCL_HEADER = 32;
        
        //const double NaN = std::numeric_limits<double>::quiet_NaN();
        
    }
}
#endif
