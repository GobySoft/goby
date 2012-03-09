
#ifndef AcommsHelpers20110208H
#define AcommsHelpers20110208H

#include <string>
#include <limits>
#include <bitset>

#include <google/protobuf/descriptor.h>

#include "goby/acomms/protobuf/modem_message.pb.h"

namespace goby
{

    namespace acomms
    {            
        // provides stream output operator for Google Protocol Buffers Message 
        inline std::ostream& operator<<(std::ostream& out, const google::protobuf::Message& msg)
        {
            return (out << "[[" << msg.GetDescriptor()->name() <<"]] " << msg.DebugString());
        }
        

        
    }
}

#endif
