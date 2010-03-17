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

#include "util/tes_utils.h"
#include "acomms/modem_message.h"

#include "message_var.h"
#include "message_publish.h"
#include "dccl_constants.h"

namespace dccl
{
    
// the Message itself
    class Message 
    {
      public:
        Message();

        // set
        void set_name(const std::string& name) {name_ = name;}

        void set_id(unsigned id)
        { id_ = id; }
        template<typename T> void set_id(const T& t)
        { set_id(boost::lexical_cast<unsigned>(t)); }
        
        void set_trigger(const std::string& trigger_type) 
        { trigger_type_ = trigger_type; }
        
        void set_trigger_var(const std::string& trigger_var)
        { trigger_var_ = trigger_var; }

        void set_trigger_time(double trigger_time)
        { trigger_time_ = trigger_time; }
        template<typename T> void set_trigger_time(const T& t)
        { set_trigger_time(boost::lexical_cast<double>(t)); }        
        
        void set_trigger_mandatory(const std::string& trigger_mandatory)
        { trigger_mandatory_ = trigger_mandatory; }
    
        void set_in_var(const std::string& in_var)
        { in_var_ = in_var; }

        void set_out_var(const std::string& out_var)
        { out_var_ = out_var; }

        void set_dest_var(const std::string& dest_var)
        { dest_var_ = dest_var; }

        void set_dest_key(const std::string& dest_key)
        { dest_key_ = dest_key; }
    
        void set_size(unsigned size)
        { size_ = size; }

        template<typename T> void set_size(const T& t)
        { set_size(boost::lexical_cast<unsigned>(t)); }

        void set_delta_encode(bool b)
        { delta_encode_ = b; }
        
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
        std::string dest_var() const          { return dest_var_; }
        std::string dest_key() const          { return dest_key_; }

        MessageVar& last_message_var()        { return layout_.back(); }
        Publish& last_publish()               { return publishes_.back(); }        
        
        std::vector<MessageVar>& layout()     { return layout_; }
        std::vector<Publish>& publishes()     { return publishes_; }

        std::set<std::string> encode_vars();
        std::set<std::string> decode_vars();
        std::set<std::string> src_vars();
        std::set<std::string> all_vars();

        //visual display of required inputs
        std::string input_summary();        
        
        //other
        std::string get_display() const;
        std::string get_short_display() const;
        std::map<std::string, std::string> message_var_names() const;
        void preprocess();
            
        void encode(std::string& body,
                    std::string& head,
                    std::map<std::string, MessageVal>& in);

        void decode(std::string& body,
                    std::string& head,
                    std::map<std::string, MessageVal>& out);

        // increment, means increase trigger number
        // prefix ++Message
        Message& operator++()
        { ++trigger_number_; return(*this); }
        // postfix Message++
        const Message operator++(int)
        { Message tmp(*this); ++(*this); return tmp;}
        
        void add_destination(modem::Message& out_message,
                             const std::map<std::string, dccl::MessageVal>& in);
        
      private:

        unsigned bytes_head() const
        { return acomms_util::NUM_HEADER_BYTES; }
        unsigned bits_head() const
        { return bytes2bits(acomms_util::NUM_HEADER_BYTES); }

        // more efficient way to do ceil(total_bits / 8)
        // to get the number of bytes rounded up.
        unsigned used_bytes_body() const
        {
            return (body_bits_& acomms_util::BYTE_MASK) ?
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
        { return size_ - acomms_util::NUM_HEADER_BYTES; }

        unsigned requested_bytes_total() const
        { return size_; }

        unsigned requested_bits_body() const
        { return bytes2bits(size_ - acomms_util::NUM_HEADER_BYTES); } 

        unsigned requested_bits_total() const
        { return bytes2bits(size_); }        
    
        
      private:
        // total request size of message, e.g. 32 bytes
        unsigned size_;
        
        unsigned trigger_number_;
        // actual used bits in body part of message (not including header bits)
        unsigned body_bits_;

        unsigned id_;
        double trigger_time_;        
        bool delta_encode_;
        std::string name_;
        std::string trigger_type_;
        std::string trigger_var_;
        std::string trigger_mandatory_;
        std::string in_var_;
        std::string out_var_;
        std::string dest_var_;
        std::string dest_key_;

        std::vector<MessageVar> layout_;
        std::vector<Publish> publishes_;

    };
    
    std::ostream& operator<< (std::ostream& out, const Message& message);
}

#endif
