
#ifndef AcommsConstants20091122H
#define AcommsConstants20091122H

#include <string>
#include <limits>
#include <bitset>


namespace goby
{
    namespace acomms
    {
        const unsigned BITS_IN_BYTE = 8;
        // one hex char is a nibble (4 bits), two nibbles per byte
        const unsigned NIBS_IN_BYTE = 2;

        /// special modem id for the broadcast destination - no one is assigned this address. Analogous to 192.168.1.255 on a 192.168.1.0 subnet
        const int BROADCAST_ID = 0;
        /// special modem id used internally to goby-acomms for indicating that the MAC layer (\c amac) is agnostic to the next destination. The next destination is thus set by the data provider (typically \c queue).
        const int QUERY_DESTINATION_ID = -1;        

        const int QUERY_SOURCE_ID = -1;        

        const unsigned char DCCL_CCL_HEADER = 32;
        
        //const double NaN = std::numeric_limits<double>::quiet_NaN();
        
    }
}
#endif
