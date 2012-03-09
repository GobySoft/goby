
#ifndef MESSAGE_VAR_INT20100317H
#define MESSAGE_VAR_INT20100317H

#include "message_var_float.h"
namespace goby
{
    namespace transitional
    {
        // <int> is a <float> with <precision>0</precision>
        class DCCLMessageVarInt : public DCCLMessageVarFloat
        {
          public:
          DCCLMessageVarInt(long max = std::numeric_limits<long>::max(), long min = 0)
              : DCCLMessageVarFloat(max, min)
            { }

            virtual DCCLType type() const { return dccl_int; }
        
          private:
            DCCLMessageVal cast(double d, int precision) { return long(d); }
        
        };    
    }
}
#endif
