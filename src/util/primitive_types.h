
#ifndef PrimitiveTypes20110721H
#define PrimitiveTypes20110721H

#include <google/protobuf/stubs/common.h>

namespace goby
{
    // use the Google Protobuf types as they handle system quirks already
    /// an unsigned 32 bit integer
    typedef google::protobuf::uint32 uint32;
    /// a signed 32 bit integer
    typedef google::protobuf::int32 int32;
    /// an unsigned 64 bit integer
    typedef google::protobuf::uint64 uint64;
    /// a signed 64 bit integer
    typedef google::protobuf::int64 int64;
}


#endif
