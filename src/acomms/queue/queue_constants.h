
#ifndef QueueConstants20091205H
#define QueueConstants20091205H

#include "goby/common/time.h"
#include "goby/acomms/acomms_constants.h"

namespace goby
{
    namespace acomms
    {
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
