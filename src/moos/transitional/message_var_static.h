
#ifndef MESSAGE_VAR_STATIC20100317H
#define MESSAGE_VAR_STATIC20100317H

#include "message_var.h"
namespace goby
{
    namespace transitional
    {   
        class DCCLMessageVarStatic : public DCCLMessageVar
        {
          public:
            void set_static_val(const std::string& static_val)
            { static_val_ = static_val; }

            std::string static_val() const {return static_val_;}

            DCCLType type() const  { return dccl_static; }
        
          private:
            void initialize_specific()
            { }

            void get_display_specific(std::stringstream& ss) const
            {
                ss << "\t\t" << "value: \"" << static_val_ << "\"" << std::endl;
            }

          private:
            std::string static_val_;
        };
    }
}
#endif
