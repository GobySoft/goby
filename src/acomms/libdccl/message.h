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

#ifndef MESSAGE20091211H
#define MESSAGE20091211H

#include <iostream>
#include <vector>
#include <sstream>
#include <bitset>
#include <algorithm>
#include <set>
#include <map>

#include <boost/foreach.hpp>
#include <boost/shared_ptr.hpp>

#include "goby/acomms/modem_message.h"

#include "message_var.h"
#include "message_var_int.h"
#include "message_var_float.h"
#include "message_var_static.h"
#include "message_var_hex.h"
#include "message_var_bool.h"
#include "message_var_string.h"
#include "message_var_enum.h"
#include "message_var_head.h"

#include "message_publish.h"
#include "dccl_constants.h"


namespace goby
{
    namespace acomms
    {
    
// the DCCLMessage itself
        class DCCLMessage 
        {
          public:
            DCCLMessage();
        
            // set
            void set_name(const std::string& name) {name_ = name;}

            void set_id(unsigned id)
            { id_ = id; }
            template<typename T> void set_id(const T& t)
            { set_id(util::as<unsigned>(t)); }
        
            void set_trigger(const std::string& trigger_type) 
            { trigger_type_ = trigger_type; }
        
            void set_trigger_var(const std::string& trigger_var)
            { trigger_var_ = trigger_var; }

            void set_trigger_time(double trigger_time)
            { trigger_time_ = trigger_time; }
            template<typename T> void set_trigger_time(const T& t)
            { set_trigger_time(util::as<double>(t)); }        
        
            void set_trigger_mandatory(const std::string& trigger_mandatory)
            { trigger_mandatory_ = trigger_mandatory; }
    
            void set_in_var(const std::string& in_var)
            { in_var_ = in_var; }

            void set_out_var(const std::string& out_var)
            { out_var_ = out_var; }

            void set_size(unsigned size)
            { size_ = size; }

            template<typename T> void set_size(const T& t)
            { set_size(util::as<unsigned>(t)); }

            void set_repeat_enabled(unsigned repeat_enabled)
            { repeat_enabled_ = repeat_enabled; }
            template<typename T> void set_repeat_enabled(const T& t)
            { set_repeat_enabled(util::as<unsigned>(t)); }
        
            void add_message_var(const std::string& type);        
            void add_publish();

            //get
            std::string name() const              { return name_; }
            unsigned id() const                   { return id_; }
            std::string trigger_var() const       { return trigger_var_; }
            std::string trigger_mandatory() const { return trigger_mandatory_; }
            double trigger_time() const           { return trigger_time_; }
            unsigned trigger_number() const       { return trigger_number_; }
            std::string trigger_type() const      { return trigger_type_; }
            std::string in_var() const            { return in_var_; }
            std::string out_var() const           { return out_var_; }
            bool repeat_enabled() const       { return repeat_enabled_; }
            unsigned repeat() const               { return repeat_; }

            DCCLMessageVar& last_message_var()        { return *layout_.back(); }
            DCCLMessageVar& header_var(acomms::DCCLHeaderPart p) { return *header_[p]; }
            DCCLPublish& last_publish()               { return publishes_.back(); }        
        
            std::vector< boost::shared_ptr<DCCLMessageVar> >& layout()     { return layout_; }
            std::vector< boost::shared_ptr<DCCLMessageVar> >& header()     { return header_; }

            const std::vector< boost::shared_ptr<DCCLMessageVar> >& layout_const() const { return layout_; }
            const std::vector< boost::shared_ptr<DCCLMessageVar> >& header_const() const { return header_; }

            std::vector<DCCLPublish>& publishes()                      { return publishes_; }

            std::set<std::string> get_pubsub_encode_vars();
            std::set<std::string> get_pubsub_decode_vars();
            std::set<std::string> get_pubsub_src_vars();
            std::set<std::string> get_pubsub_all_vars();

            boost::shared_ptr<DCCLMessageVar> name2message_var(const std::string& name) const;
        
        
            //other
            std::string get_display() const;
            std::string get_short_display() const;
            std::map<std::string, std::string> message_var_names() const;
            void preprocess();
            void set_repeat_array_length();
            unsigned calc_total_size();
        
            void set_head_defaults(std::map<std::string, std::vector<DCCLMessageVal> >& in, unsigned modem_id);
        
    
            void head_encode(std::string& head, std::map<std::string, std::vector<DCCLMessageVal> >& in);
            void head_decode(const std::string& head, std::map<std::string, std::vector<DCCLMessageVal> >& out);
        
            void body_encode(std::string& body, std::map<std::string, std::vector<DCCLMessageVal> >& in);
            void body_decode(std::string& body, std::map<std::string, std::vector<DCCLMessageVal> >& out);

            // increment, means increase trigger number
            // prefix ++DCCLMessage
            DCCLMessage& operator++()
            { ++trigger_number_; return(*this); }
            // postfix DCCLMessage++
            const DCCLMessage operator++(int)
            { DCCLMessage tmp(*this); ++(*this); return tmp;}
        
        
          private:
            unsigned bytes_head() const
            { return acomms::DCCL_NUM_HEADER_BYTES; }
            unsigned bits_head() const
            { return bytes2bits(acomms::DCCL_NUM_HEADER_BYTES); }

            // more efficient way to do ceil(total_bits / 8)
            // to get the number of bytes rounded up.
        
            enum { BYTE_MASK = 7 }; // 00000111
            unsigned used_bytes_body() const
            {
                return (body_bits_& BYTE_MASK) ?
                    bits2bytes(body_bits_) + 1 :
                    bits2bytes(body_bits_);
            }

            unsigned used_bytes_total() const
            { return bytes_head() + used_bytes_body(); }

            unsigned used_bits_body() const
            { return body_bits_; }
        
            unsigned used_bits_total() const
            { return bits_head() + used_bits_body(); }
        
            unsigned requested_bytes_body() const
            { return size_ - acomms::DCCL_NUM_HEADER_BYTES; }

            unsigned requested_bytes_total() const
            { return size_; }

            unsigned requested_bits_body() const
            { return bytes2bits(size_ - acomms::DCCL_NUM_HEADER_BYTES); } 

            unsigned requested_bits_total() const
            { return bytes2bits(size_); }
        
        
          private:
            // total request size of message, e.g. 32 bytes
            unsigned size_;
        
            unsigned trigger_number_;
            // actual used bits in body part of message (not including header bits)
            unsigned body_bits_;

            unsigned id_;
            unsigned modem_id_;
            double trigger_time_;        

            bool repeat_enabled_;
            unsigned repeat_;
        
            std::string name_;
            std::string trigger_type_;
            std::string trigger_var_;
            std::string trigger_mandatory_;
            std::string in_var_;
            std::string out_var_;

            std::vector< boost::shared_ptr<DCCLMessageVar> > layout_;
            std::vector< boost::shared_ptr<DCCLMessageVar> > header_;
        
        
            std::vector<DCCLPublish> publishes_;

        };


        inline void bitset2string(const boost::dynamic_bitset<unsigned char>& body_bits,
                                  std::string& body)
        {
            body.resize(body_bits.num_blocks()); // resize the string to fit the bitset
            to_block_range(body_bits, body.rbegin());
        }
    
        inline void string2bitset(boost::dynamic_bitset<unsigned char>& body_bits,
                                  const std::string& body)
        {
            from_block_range(body.rbegin(), body.rend(), body_bits);
        }


        std::ostream& operator<< (std::ostream& out, const DCCLMessage& message);
    }
}
#endif
