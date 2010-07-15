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

#ifndef MESSAGE_VAR_FLOAT20100317H
#define MESSAGE_VAR_FLOAT20100317H

#include "message_var.h"
namespace goby
{
    namespace acomms
    {   
        class DCCLMessageVarFloat : public DCCLMessageVar
        {
          public:
            DCCLMessageVarFloat(double max = std::numeric_limits<double>::max(), double min = 0, double precision = 0);

            virtual int calc_size() const
            { return is_delta() ? delta_size() : key_size(); }

            virtual int calc_total_size() const;
        
            void set_max(double max) {max_ = max;}
            void set_max(const std::string& s) { set_max(boost::lexical_cast<double>(s)); }
        
            void set_min(double min) {min_ = min;}
            void set_min(const std::string& s) { set_min(boost::lexical_cast<double>(s)); }

            void set_precision(int precision) {precision_ = precision;}
            void set_precision(const std::string& s) { set_precision(boost::lexical_cast<int>(s)); }
        
            int precision() const { return precision_; }  

            double min() const { return min_; }
            double max() const { return max_; }
        
            void set_max_delta(double max_delta)
            { max_delta_ = max_delta; }
            void set_max_delta(const std::string& s)
            { set_max_delta(boost::lexical_cast<double>(s)); }

            virtual DCCLType type() const { return dccl_float; }

            unsigned key_size() const
            { return ceil(log((max_-min_)*pow(10.0,static_cast<double>(precision_))+2)/log(2)); }

            unsigned delta_size() const
            { return ceil(log((2*max_delta_)*pow(10.0,static_cast<double>(precision_))+2)/log(2)); }

          protected:
            virtual DCCLMessageVal cast(double d, int precision) { return DCCLMessageVal(d, precision); }
            virtual void initialize_specific();
        
          private:
            bool is_delta() const
            { return using_delta_differencing() && !is_key_frame_; }
        
            bool using_delta_differencing() const
            { return !isnan(max_delta_); }
        
            boost::dynamic_bitset<unsigned char> encode_specific(const DCCLMessageVal& v);
            DCCLMessageVal decode_specific(boost::dynamic_bitset<unsigned char>& b);

            void get_display_specific(std::stringstream& ss) const;
        
          private:
            double max_;
            double min_;
            int precision_;

            double max_delta_;
        };

    }
}
#endif
