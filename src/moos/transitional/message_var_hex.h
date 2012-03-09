
#ifndef MESSAGE_VAR_HEX20100317H
#define MESSAGE_VAR_HEX20100317H

#include "message_var.h"
#include "goby/acomms/dccl/dccl_exception.h"

#include "dccl_constants.h"
#include "goby/util/binary.h"
#include "goby/util/as.h"

namespace goby
{
    namespace transitional
    {   
        class DCCLMessageVarHex : public DCCLMessageVar
        {
          public:

          DCCLMessageVarHex()
              : DCCLMessageVar(),
                num_bytes_(0)
                { }

            /* int calc_size() const */
            /* { return num_bytes_*acomms::BITS_IN_BYTE; } */

            void set_num_bytes(unsigned num_bytes) {num_bytes_ = num_bytes;}
            void set_num_bytes(const std::string& s) { set_num_bytes(boost::lexical_cast<unsigned>(s)); }

            unsigned num_bytes() const {return num_bytes_;}        
            DCCLType type() const { return dccl_hex; }        
        
          private:
            void initialize_specific()
            { }
        
            void get_display_specific(std::stringstream& ss) const
            { }

          private:
            unsigned num_bytes_;

        };
    }
}
#endif
