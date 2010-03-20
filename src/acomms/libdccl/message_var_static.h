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

#ifndef MESSAGE_VAR_STATIC20100317H
#define MESSAGE_VAR_STATIC20100317H

#include "message_var.h"

namespace dccl
{   
    class MessageVarStatic : public MessageVar
    {
      public:
        int calc_size() const
        { return 0; }

        void set_static_val(const std::string& static_val)
        { static_val_ = static_val; }

        std::string static_val() const {return static_val_;}

        DCCLType type() const  { return dccl_static; }
        
      private:
        void initialize_specific()
        { }
        
        boost::dynamic_bitset<unsigned char> encode_specific(const MessageVal& v)
        {
            return boost::dynamic_bitset<unsigned char>();
        }        

        MessageVal decode_specific(boost::dynamic_bitset<unsigned char>& b)
        {
            return MessageVal(static_val_);
        }

        void get_display_specific(std::stringstream& ss) const
        {
            ss << "\t\t" << "value: \"" << static_val_ << "\"" << std::endl;
        }

      private:
        std::string static_val_;
    };
}

#endif
