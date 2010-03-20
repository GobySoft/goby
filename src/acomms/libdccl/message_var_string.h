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

#include "message_var.h"

namespace dccl
{   
    class MessageVarString : public MessageVar
    {
      public:
      MessageVarString()
          : MessageVar(),
            max_length_(0)
            { }

        int calc_size() const
        { return max_length_*acomms::BITS_IN_BYTE; }

        void set_max_length(unsigned max_length) {max_length_ = max_length;}
        void set_max_length(const std::string& s) { set_max_length(boost::lexical_cast<unsigned>(s)); }

        unsigned max_length() const {return max_length_;}        

        DCCLType type() const  { return dccl_string; }
        
      private:
        void initialize_specific()
        { }
        
        boost::dynamic_bitset<unsigned char> encode_specific(const MessageVal& v)
        {
            unsigned size = calc_size();            
            boost::dynamic_bitset<unsigned char> bits(size);

            std::string s = v;
            
            // tack on null terminators (probably a byte of zeros in ASCII)
            s += std::string(max_length_, '\0');
            
            // one byte per char
            for (size_t j = 0; j < (size_t)max_length_; ++j)
            {
                bits <<= acomms::BITS_IN_BYTE;
                bits |= boost::dynamic_bitset<unsigned char>(size, s[j]);;
            }
            return bits;
        }
        

        MessageVal decode_specific(boost::dynamic_bitset<unsigned char>& b)
        {
            char s[max_length_+1];
            s[max_length_] = '\0';
            
            for (size_t j = 0; j < max_length_; ++j)
            {
                s[max_length_-j-1] = (b & boost::dynamic_bitset<unsigned char>(calc_size(), 0xff)).to_ulong();
                b >>= acomms::BITS_IN_BYTE;
            }
            
            if(!std::string(s).empty())
                return MessageVal(std::string(s));
            else
                return MessageVal();
        }

        void get_display_specific(std::stringstream& ss) const
        { }        

      private:
        unsigned max_length_;

    };
}

#endif
