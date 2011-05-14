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

#ifndef DCCLTRANSITIONALCODEC20110513H
#define DCCLTRANSITIONALCODEC20110513H

#include "goby/acomms/libdccl/dccl_field_codec_default.h"


namespace goby
{
    namespace transitional
    {
        

        class DCCL1TimeCodec : public DCCLDefaultArithmeticFieldCodec<int32, std::string>
        {
          public:
            int32 pre_encode(const std::string& field_value);
            std::string post_decode(const int32& wire_value);            
 
          private:
            void validate() { }

            double max() { return HOURS_IN_DAY*SECONDS_IN_HOUR; }
            double min() { return 0; }
            enum { HOURS_IN_DAY = 24 };
            enum { SECONDS_IN_HOUR = 3600 };

        };

    }
}



#endif
