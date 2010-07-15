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

#ifndef MESSAGE_VAR_INT20100317H
#define MESSAGE_VAR_INT20100317H

#include "message_var_float.h"
namespace goby
{
    namespace acomms
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
