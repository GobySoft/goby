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

#ifndef MESSAGE_VAR_ENUM20100317H
#define MESSAGE_VAR_ENUM20100317H

#include "message_var.h"
namespace goby
{

namespace dccl
{   
    class MessageVarEnum : public MessageVar
    {
      public:
        int calc_size() const
        { return ceil(log(enums_.size()+1)/log(2)); }        
        
        void add_enum(std::string senum) {enums_.push_back(senum);}

        std::vector<std::string>* enums() { return &enums_; }

        DCCLType type()  const { return dccl_enum; }
        
      private:
        void initialize_specific()
        { }
        
        boost::dynamic_bitset<unsigned char> encode_specific(const MessageVal& v)
        {
            std::string s = v;
            // find the iterator within the std::vector of enumerator values for *this* enumerator value
            std::vector<std::string>::iterator pos;
            pos = find(enums_.begin(), enums_.end(), s);
            
            // now convert that iterator into a number (think traditional array index)
            unsigned long t = (unsigned long)distance(enums_.begin(), pos);
            
            if(pos == enums_.end())
                t = 0;
            else
                ++t;
            
            return boost::dynamic_bitset<unsigned char>(calc_size(), t);            
        }        

        MessageVal decode_specific(boost::dynamic_bitset<unsigned char>& b)
        {
            unsigned long t = b.to_ulong();
            if(t)
            {
                --t;
                return MessageVal(enums_.at(t));
            }
            else
                return MessageVal();            
        }

        void get_display_specific(std::stringstream& ss) const
        {
            ss << "\t\tvalues:{"; 
            for (std::vector<std::string>::size_type j = 0, m = enums_.size(); j < m; ++j)
            {
                if(j)
                    ss << ",";
                ss << enums_[j];
            }
            
            ss << "}" << std::endl;
        }

      private:
        std::vector<std::string> enums_;
    };
}
}
#endif
