
#ifndef MESSAGE_VAR_BOOL20100317H
#define MESSAGE_VAR_BOOL20100317H

#include "message_var.h"

namespace goby
{
namespace transitional
{   
    class DCCLMessageVarBool : public DCCLMessageVar
    {
      public:
        DCCLType type() const { return dccl_bool; }
        
      private:
        void initialize_specific()
        { }
        

        void get_display_specific(std::stringstream& ss) const
        { }        

    };
}
}

#endif
