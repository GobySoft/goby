// copyright 2008, 2009 t. schneider tes@mit.edu
// 
// this file is part of the Dynamic Compact Control Language (DCCL),
// the goby-acomms codec. goby-acomms is a collection of libraries 
// for acoustic underwater networking
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This software is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this software.  If not, see <http://www.gnu.org/licenses/>.

#ifndef MESSAGE_VAR_HEX20100317H
#define MESSAGE_VAR_HEX20100317H

#include "message_var.h"
#include "goby/acomms/libdccl/dccl_exception.h"

#include "dccl_constants.h"
#include "goby/util/binary.h"
#include "goby/util/string.h"

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
