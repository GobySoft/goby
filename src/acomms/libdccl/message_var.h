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

#ifndef MESSAGE_VAR20091211H
#define MESSAGE_VAR20091211H

#include <bitset>
#include <stdexcept>
#include <vector>
#include <string>
#include <map>
#include <ostream>

#include <boost/dynamic_bitset.hpp>
#include <boost/lexical_cast.hpp>

#include "dccl_constants.h"

namespace dccl
{
    class MessageVal;
    class AlgorithmPerformer;
    
// defines a piece of a Message (an <int>, a <bool>, etc)
    class MessageVar
    {
      public:
        MessageVar();
        
        // set
        void set_name(std::string name) {name_ = name;}
        void set_type(DCCLType type) {type_ = type;}
        void set_source_var(std::string source_var) {source_var_ = source_var;}
        void set_source_key(std::string source_key) {source_key_ = source_key;}

        void set_max(double max) {max_ = max;}
        void set_max(const std::string& s) { set_max(boost::lexical_cast<double>(s)); }
        
        void set_min(double min) {min_ = min;}
        void set_min(const std::string& s) { set_min(boost::lexical_cast<double>(s)); }

        void set_max_length(unsigned max_length) {max_length_ = max_length;}
        void set_max_length(const std::string& s) { set_max_length(boost::lexical_cast<unsigned>(s)); }

        void set_precision(int precision) {precision_ = precision;}
        void set_precision(const std::string& s) { set_precision(boost::lexical_cast<int>(s)); }

        void set_source_set(bool source_set) {source_set_ = source_set;}
        void set_static_val(std::string static_val) {static_val_ = static_val;}
        void set_algorithms(const std::vector<std::string>& algorithm) {algorithms_ = algorithm;}

        void set_expected_delta(double expected_delta) {expected_delta_ = expected_delta;}
        void set_expected_delta(const std::string& s) { set_expected_delta(boost::lexical_cast<double>(s)); }

        void set_delta_var(bool b) { delta_var_ = b; }
        
        
        void add_enum(std::string senum) {enums_.push_back(senum);}
        

        // get
        std::string name() const {return name_;}
        DCCLType type() const {return type_;}
        std::string source_var() const {return source_var_;}
//    std::string const source_key() {return source_key_;}
        double max() const { return (delta_var_) ? expected_delta_ : max_;}
        double min() const {return (delta_var_) ? -expected_delta_ : min_;}
        unsigned max_length() const {return max_length_;}
        int precision() const {return precision_;}
//    bool const source_set() {return source_set_;}
        std::string static_val() const {return static_val_;}
        std::vector<std::string>& enums() {return enums_;}
//    std::vector<std::string> const& algorithms() {return algorithms_;}

        // other
        void initialize(const std::string& message_name, const std::string& trigger_var, bool delta_encode);
        int calc_size() const;
        std::string get_display() const;

        void read_dynamic_vars(std::map<std::string,MessageVal>& vals, const std::map<std::string, std::string>* in_str, const std::map<std::string, double>* in_dbl, const std::map<std::string, long>* in_long, const std::map<std::string, bool>* in_bool);
        std::string parse_string_val(const std::string& sval);
        void var_encode(std::map<std::string,MessageVal>& vals, boost::dynamic_bitset<>& bits);
        void var_decode(std::map<std::string,MessageVal>& vals, boost::dynamic_bitset<>& bits);

      private:
        double max_;
        double min_;
        unsigned max_length_;
        int precision_;
        bool source_set_;
        double expected_delta_;
        bool delta_var_;
        
        AlgorithmPerformer * ap_;
        
        std::string name_;
        DCCLType type_;
        std::string source_var_;
        std::string source_key_;
        std::string static_val_;
        std::vector<std::string> enums_;
        std::vector<std::string> algorithms_;

    };

    std::ostream& operator<< (std::ostream& out, const MessageVar& m);
}

#endif
