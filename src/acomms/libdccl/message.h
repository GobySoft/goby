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

        void set_id(unsigned id) {id_ = id;}
        template<typename T> void set_id(const T& t)
        { set_id(boost::lexical_cast<unsigned>(t)); }
        
        void set_trigger(const std::string& trigger_type) {trigger_type_ = trigger_type; }
        void set_trigger_var(const std::string& trigger_var){trigger_var_ = trigger_var;}

        void set_trigger_time(double trigger_time){trigger_time_ = trigger_time;}
        template<typename T> void set_trigger_time(const T& t)
        { set_trigger_time(boost::lexical_cast<double>(t)); }
        
        
        void set_trigger_mandatory(const std::string& trigger_mandatory){trigger_mandatory_ = trigger_mandatory;}
    
        void set_in_var(const std::string& in_var){in_var_ = in_var;}
        void set_out_var(const std::string& out_var){out_var_ = out_var;}
        void set_dest_var(const std::string& dest_var){dest_var_ = dest_var;}
        void set_dest_key(const std::string& dest_key){dest_key_ = dest_key;}
    
        void set_size(unsigned size){size_ = size; avail_size_ = size - acomms_util::NUM_HEADER_BYTES;}
        template<typename T> void set_size(const T& t)
        { set_size(boost::lexical_cast<unsigned>(t)); }

        void set_delta_encode(bool b) { delta_encode_ = b; }
        
        void add_message_var(const std::string& type);        
        void add_publish();

        //get
        std::string name() const {return name_;}
        unsigned id() const {return id_;}
        std::string trigger_var() const {return trigger_var_;}
        std::string trigger_mandatory() const {return trigger_mandatory_;}
        double trigger_time() const {return trigger_time_;}
        unsigned trigger_number() const {return trigger_number_;}
        std::string trigger_type() const {return trigger_type_;}
        std::string in_var() const {return in_var_;}
        std::string out_var() const {return out_var_;}
        std::string dest_var() const {return dest_var_;}
        std::string dest_key() const {return dest_key_;}

        MessageVar& last_message_var() { return layout_.back(); }
        Publish& last_publish() { return publishes_.back(); }        
        
        std::vector<MessageVar> * get_layout() {return &layout_;}
        std::vector<Publish> * get_publishes() {return &publishes_;}

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

            
        enum { NO_MOOS_VAR = false, FROM_MOOS_VAR = true };
        void encode(modem::Message& out,
                    const std::map<std::string, std::string>* in_str,
                    const std::map<std::string, double>* in_dbl,
                    const std::map<std::string, long>* in_long,
                    const std::map<std::string, bool>* in_bool,
                    bool extract_from_moos_var = NO_MOOS_VAR);
        enum { NO_PUBLISHES = false, DO_PUBLISHES = true };
        template <typename Map1, typename Map2, typename Map3, typename Map4>
            void decode(const modem::Message& in,
                        Map1* out_str,
                        Map2* out_dbl,
                        Map3* out_long,
                        Map4* out_bool,
                        bool write_publishes = NO_PUBLISHES);
        

        // increment, means increase trigger number
        // prefix ++Message
            Message& operator++() {++trigger_number_; return(*this);}
        // postfix Message++
        const Message operator++(int) {Message tmp(*this); ++(*this); return tmp;}
    
        
      private:
        void assemble_hex(modem::Message& out_message,
                          const boost::dynamic_bitset<>& bits);
        void disassemble_hex(const modem::Message& in_message,
                             boost::dynamic_bitset<>& bits);
        void add_destination(modem::Message& out_message,
                             const std::map<std::string, std::string>& in_str,
                             const std::map<std::string, double>& in_dbl);        
        
        
        // more efficient way to do ceil(total_bits / 8) to get the number of bytes rounded up.
        unsigned used_bytes_no_head() const { return (total_bits_& acomms_util::BYTE_MASK) ? bits2bytes(total_bits_) + 1 : bits2bytes(total_bits_); }
        unsigned used_bytes() const { return (total_bits_& acomms_util::BYTE_MASK) ? bits2bytes(total_bits_) + 1 + acomms_util::NUM_HEADER_BYTES : bits2bytes(total_bits_) + acomms_util::NUM_HEADER_BYTES; }

        unsigned used_bits_no_head() const { return total_bits_; } 
        unsigned used_bits() const { return total_bits_ + bytes2bits(acomms_util::NUM_HEADER_BYTES); }

        // more efficient way to do ceil(total_bits / 8) to get the number of bytes rounded up.
        unsigned requested_bytes_no_head() const { return size_ - acomms_util::NUM_HEADER_BYTES; }
        unsigned requested_bytes() const { return size_; }

        unsigned requested_bits_no_head() const { return bytes2bits(size_ - acomms_util::NUM_HEADER_BYTES); } 
        unsigned requested_bits() const { return bytes2bits(size_); }        
    
        
      private:
        // total size of message, e.g. 32 bytes
        unsigned size_;
        // available size of message, e.g. 32-2 = 30 bytes;
        unsigned avail_size_;
        
        unsigned trigger_number_;
        // bits in user part of message (not including header bits)
        unsigned total_bits_;

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

    // decode is a template so put encode in the header too
    inline void Message::encode(modem::Message& out,
                                const std::map<std::string, std::string>* in_str,
                                const std::map<std::string, double>* in_dbl,
                                const std::map<std::string, long>* in_long,
                                const std::map<std::string, bool>* in_bool,                                
                                bool extract_from_moos_var /* = false */)
    {

        boost::dynamic_bitset<> bits(bytes2bits(used_bytes_no_head())); // actual size rounded up to closest byte
        
        // 1. iterate first to build up maps of values
        std::map<std::string,MessageVal> vals;

        if(extract_from_moos_var)
        {
            if(!in_str || !in_dbl)
                throw(std::runtime_error(std::string("DCCL: attempted encode using `extract from moos var` without providing a valid std::map<std::string, std::string> and/or std::map<std::string, double>")));
            else
            {   
                // using <moos_var/> tag, do casts from double, pull strings from key=value,key=value, etc.
                for (std::vector<MessageVar>::iterator it = layout_.begin(); it != layout_.end(); ++it) 
                    it->read_dynamic_vars(vals, in_str, in_dbl, in_long, in_bool);
            }    
        }
        else
        {
            // using <name/> tag, do only casts supported by streams
            if(in_str)
            {
                typedef std::pair<std::string,std::string> P;
                BOOST_FOREACH(const P& p, *in_str)
                    vals[p.first] = MessageVal(p.second);
            }
            if(in_dbl)
            {
                typedef std::pair<std::string,double> P;
                BOOST_FOREACH(const P& p, *in_dbl)
                    vals[p.first] = MessageVal(p.second);
            }
            if(in_long)
            {
                typedef std::pair<std::string,long> P;
                BOOST_FOREACH(const P& p, *in_long)
                    vals[p.first] = MessageVal(p.second);
            }
            if(in_bool)
            {
                typedef std::pair<std::string,bool> P;
                BOOST_FOREACH(const P& p, *in_bool)
                    vals[p.first] = MessageVal(p.second);
            }            
        }
            
        // 2. encode each variable
        for (std::vector<MessageVar>::iterator it = layout_.begin(); it != layout_.end(); ++it)
            it->var_encode(vals, bits);
        
        // 3. convert to hexadecimal string and tack on the header
        assemble_hex(out, bits);

        // 4. deal with the destination
        add_destination(out, *in_str, *in_dbl);
    }
    
    template <typename Map1, typename Map2, typename Map3, typename Map4>
        inline void Message::decode(const modem::Message& in,
                                    Map1* out_str,
                                    Map2* out_dbl,
                                    Map3* out_long,
                                    Map4* out_bool,
                                    bool write_publishes /* = false */)
    {
        
        boost::dynamic_bitset<> bits(bytes2bits(used_bytes_no_head()));
        // 3. convert hexadecimal string to bitset
        disassemble_hex(in, bits);
        
        std::map<std::string,MessageVal> vals;
        // 2. pull the bits off the message in the reverse that they were put on
        for (std::vector<MessageVar>::reverse_iterator it = layout_.rbegin(), n = layout_.rend(); it != n; ++it)
            it->var_decode(vals, bits);
        
        if(write_publishes)
        {
            if(!out_str || !out_dbl)
                throw(std::runtime_error(std::string("DCCL: attempted decode using `write publishes` without providing a valid std::map<std::string, std::string> and/or std::map<std::string, double>")));
            else
            {                
                // 1. now go through all the publishes_ and fill in the format strings
                for (std::vector<Publish>::iterator it = publishes_.begin(), n = publishes_.end(); it != n; ++it)
                    it->write_publish(vals, layout_, out_str, out_dbl, out_long, out_bool);
            }    
        }
        else
        {
            // for non moos users, just return the properly "binned" values (keyed to the <name> tag)
            typedef std::pair<std::string,MessageVal> P;
            BOOST_FOREACH(const P& vp, vals)
            {
                std::string s;
                double d;
                long l;
                bool b;
                const MessageVal& mv = vp.second;
                if(out_bool && (mv.type() == cpp_bool) && mv.val(b))
                    out_bool->insert(std::pair<std::string, bool>(vp.first,b));
                else if(out_long && (mv.type() == cpp_long || mv.type() == cpp_bool) && mv.val(l))
                    out_long->insert(std::pair<std::string, long>(vp.first,l));
                else if(out_dbl && (mv.type() == cpp_double || mv.type() == cpp_long || mv.type() == cpp_bool) && mv.val(d))
                    out_dbl->insert(std::pair<std::string, double>(vp.first,d));
                else if(out_str && mv.val(s))
                    out_str->insert(std::pair<std::string, std::string>(vp.first,s));
                // otherwise we're out of luck
            }
        }
    }
}

#endif
