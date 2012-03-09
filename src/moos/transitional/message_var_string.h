
#ifndef MESSAGE_VAR_STRING20100317H
#define MESSAGE_VAR_STRING20100317H

#include "goby/util/as.h"

#include "message_var.h"

namespace goby
{
    namespace transitional
    {   
        class DCCLMessageVarString : public DCCLMessageVar
        {
          public:
          DCCLMessageVarString()
              : DCCLMessageVar(),
                max_length_(0)
                { }

            void set_max_length(unsigned max_length) {max_length_ = max_length;}
            void set_max_length(const std::string& s) { set_max_length(util::as<unsigned>(s)); }

            unsigned max_length() const {return max_length_;}        

            DCCLType type() const  { return dccl_string; }
        
          private:
            void initialize_specific()
            { }
        
            void get_display_specific(std::stringstream& ss) const
            { }        

          private:
            unsigned max_length_;

        };
    }
}
#endif
