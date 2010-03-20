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

#include "message_var.h"

namespace dccl
{   
    class MessageVarInt : public MessageVar
    {
      public:
      MessageVarInt(long max = std::numeric_limits<long>::max(), long min = 0)
          : MessageVar(),
            max_(max),
            min_(min),
            expected_delta_(acomms::NaN),
            delta_var_(false)
            { }        

        virtual int calc_size() const
        { return ceil(log(max()-min()+2)/log(2)); }        

        void set_max(long max) {max_ = max;}
        void set_max(const std::string& s) { set_max(boost::lexical_cast<long>(s)); }
        
        void set_min(long min) {min_ = min;}
        void set_min(const std::string& s) { set_min(boost::lexical_cast<long>(s)); }

        double max() const { return (delta_var_) ? expected_delta_ : max_; }
        double min() const { return (delta_var_) ? -expected_delta_ : min_; }
        
        void set_expected_delta(double expected_delta)
        { expected_delta_ = expected_delta; }
        void set_expected_delta(const std::string& s)
        { set_expected_delta(boost::lexical_cast<double>(s)); }

        void set_delta_var(bool b) { delta_var_ = b; }

        DCCLType type() const { return dccl_int; }
        
      private:
        void initialize_specific()
        {
            // flip max and min if needed
            if(max_ < min_)
            {
                long tmp = max_;
                max_ = min_;
                min_ = tmp;
            }    

            if(isnan(expected_delta_))
                // use default of 10% of range
                expected_delta_ = 0.1 * (max_ - min_);
            else
                expected_delta_ = abs(expected_delta_);
        }
        
        virtual boost::dynamic_bitset<unsigned char> encode_specific(const MessageVal& v)
        {
            long t;
            if(v.get(t) && !(t < min() || t > max()))
            {
                t -= min();
                return boost::dynamic_bitset<unsigned char>(calc_size(), static_cast<unsigned long>(t)+1);
            }
            else
            {
                return boost::dynamic_bitset<unsigned char>();
            }
        }
        

        virtual MessageVal decode_specific(boost::dynamic_bitset<unsigned char>& b)
        {

            unsigned long t = b.to_ulong();
            if(t)
            {
                --t;
                return MessageVal(long(t) + long(min()));
            }
            else
            {                
                return MessageVal();
            }
        }

        void get_display_specific(std::stringstream& ss) const
        {
            ss << "\t\t[min, max] = [" << min_ << "," << max_ << "]" << std::endl;
            if(!isnan(expected_delta_))
                ss << "\t\texpected_delta: {" << expected_delta_ << "}" << std::endl;
        }        


      private:
        long max_;
        long min_;
        
        double expected_delta_;
        bool delta_var_;
        
    };
}

#endif
