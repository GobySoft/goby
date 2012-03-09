
#include <string>

#include "MOOSLIB/MOOSMsg.h"

namespace goby
{
    namespace moos
    {
 
        class MOOSSerializer
        {
          public:
            static void serialize(const CMOOSMsg& const_msg, std::string* data)
            {
                // copy because Serialize wants to modify the CMOOSMsg
                CMOOSMsg msg(const_msg);
                // adapted from MOOSCommPkt.cpp
                const unsigned PKT_TMP_BUFFER_SIZE = 40000;
                int serialized_size = PKT_TMP_BUFFER_SIZE;
                data->resize(serialized_size);
                serialized_size = msg.Serialize(reinterpret_cast<unsigned char*>(&(*data)[0]), serialized_size);
                data->resize(serialized_size);
            }
            
            static void parse(CMOOSMsg* msg, std::string data)
            {
                msg->Serialize(reinterpret_cast<unsigned char*>(&data[0]), data.size(), false);
            }
            
        };
    }
}
