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

#ifndef MESSAGE_VAR_BOOL20100317H
#define MESSAGE_VAR_BOOL20100317H

#include "message_var.h"

namespace goby
{
namespace acomms
{   
    class DCCLMessageVarBool : public DCCLMessageVar
    {
      public:
        int calc_size() const
        { return 1 + 1; }

        DCCLType type() const { return dccl_bool; }
        
      private:
        void initialize_specific()
        { }
        
        
        boost::dynamic_bitset<unsigned char> encode_specific(const DCCLMessageVal& v)
        {
            bool b;
            if(v.get(b))
                return boost::dynamic_bitset<unsigned char>(calc_size(), ((b) ? 1 : 0) + 1);
            else
                return boost::dynamic_bitset<unsigned char>();
        }
        

        DCCLMessageVal decode_specific(boost::dynamic_bitset<unsigned char>& b)
        {

            unsigned long t = b.to_ulong();
            if(t)
            {
                --t;
                return DCCLMessageVal(bool(t));
            }
            else
                return DCCLMessageVal();
        }

        void get_display_specific(std::stringstream& ss) const
        { }        

    };
}
}

#endif
