// copyright 2009 t. schneider tes@mit.edu
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

#ifndef QueueConstants20091205H
#define QueueConstants20091205H

#include "goby/util/time.h"
#include "goby/acomms/acomms_constants.h"

namespace goby
{
    namespace acomms
    {
        // smallest allowed id - 0 is reserved for non-existent message
        const unsigned MIN_ID = 1;
        // largest allowed id 
        const unsigned MAX_ID = 1 << HEAD_DCCL_ID_SIZE;
        
        const unsigned MULTIMESSAGE_MASK = 1 << 7;
        const unsigned BROADCAST_MASK = 1 << 6;
        const unsigned VAR_ID_MASK = 0xFF ^ MULTIMESSAGE_MASK ^ BROADCAST_MASK;

        // number of bytes used to store the size of the following user frame
        const unsigned USER_FRAME_NEXT_SIZE_BYTES = 1;
        
        // how old an on_demand message can be before re-encoding
        const boost::posix_time::time_duration ON_DEMAND_SKEW = boost::posix_time::seconds(1);    
    }
}
#endif
