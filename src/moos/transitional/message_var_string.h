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
