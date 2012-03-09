
#ifndef MESSAGE_VAR_ENUM20100317H
#define MESSAGE_VAR_ENUM20100317H

#include "message_var.h"
namespace goby
{

    namespace transitional
    {   
        class DCCLMessageVarEnum : public DCCLMessageVar
        {
          public:
        
            void add_enum(std::string senum) {enums_.push_back(senum);}

            std::vector<std::string>* enums() { return &enums_; }

            DCCLType type()  const { return dccl_enum; }
        
          private:
            void initialize_specific()
            { }
        
          private:
            std::vector<std::string> enums_;
        };
    }
}
#endif
