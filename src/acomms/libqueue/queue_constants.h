#ifndef QueueConstants20091205H
#define QueueConstants20091205H

namespace acomms_util
{
    // largest allowed id 
    const unsigned MAX_ID = 63;
    const unsigned MULTIMESSAGE_MASK = 1 << 7;
    const unsigned BROADCAST_MASK = 1 << 6;
    const unsigned VAR_ID_MASK = 0xFF ^ MULTIMESSAGE_MASK ^ BROADCAST_MASK;
}

#endif
