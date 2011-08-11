// copyright 2011 t. schneider tes@mit.edu
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

#ifndef DCCLFIELDCODEC20110510H
#define DCCLFIELDCODEC20110510H

#include "dccl_field_codec.h"

namespace goby
{
    namespace acomms
    {        
        template<typename WireType, typename FieldType = WireType>
            class DCCLTypedFixedFieldCodec : public DCCLTypedFieldCodec<WireType, FieldType>
        {
          protected:
          virtual unsigned size() = 0;
          
          private:
          unsigned size(const FieldType& field_value)
          { return size(); }
          
          unsigned max_size()
          { return size(); }
          
          unsigned min_size()
          { return size(); }
          
          bool variable_size()
          { return false; }
        };
    }
}


#endif
