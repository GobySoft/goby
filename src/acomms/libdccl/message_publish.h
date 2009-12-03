// copyright 2008, 2009 t. schneider tes@mit.edu
// ocean engineering graudate student - mit / whoi joint program
// massachusetts institute of technology (mit)
// laboratory for autonomous marine sensing systems (lamss)
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

#ifndef PUBLISH_H
#define PUBLISH_H

#include <iostream>
#include <vector>
#include <sstream>

#include <boost/format.hpp>

#include "message_var.h"
#include "message_val.h"
#include "message_algorithms.h"
#include "dccl_constants.h"

namespace dccl
{
    
// defines (a single) thing to do with the decoded message
// that is, where do we publish it and what should we include in the
// published message
    class Publish
    {
      public:
      Publish() : var_(""),
            format_(""),
            format_set_(false),
            use_all_names_(false),
            is_string_(false),
            is_double_(false),
            ap_(AlgorithmPerformer::getInstance())
            { }        

        //set
    
        void set_var(std::string var) {var_=var;}
        void set_format(std::string format) {format_=format;}
        void set_format_set(bool format_set) {format_set_=format_set;}
        void set_use_all_names(bool use_all_names) {use_all_names_ = use_all_names;}
        void set_is_string(bool is_string) {is_string_ = is_string;}
        void set_is_double(bool is_double) {is_double_ = is_double;}
    
        void add_name(const std::string& name) {names_.push_back(name);}
        void add_algorithms(const std::vector<std::string> algorithms) {algorithms_.push_back(algorithms);}
        void add_id(size_t id) {ids_.push_back(id);}
    
        //get
//    std::string var() const {return var_;}
//    std::string format() const {return format_;}
//    bool format_set()  const {return format_set_;}
//    bool use_all_names() const {return use_all_names_;}
    
        bool is_string() {return is_string_;}
        bool is_double() {return is_double_;}
//    std::vector<std::string> const& names() {return names_;}
//    std::vector<std::vector<std::string> > const& algorithms(){return algorithms_;}
    
        
        std::string get_display() const;

        template <typename Map1, typename Map2>
            void write_publish(std::map<std::string,MessageVal>& vals, std::vector<MessageVar>& layout, Map1& out_str, Map2& out_dbl);
        
        void initialize(std::vector<MessageVar>& layout);

      private:
        void fill_format(std::map<std::string,MessageVal>& vals, std::vector<MessageVar>& layout, std::string& key, std::string& value);
            
      private:
        std::string var_;
        std::string format_;
        bool format_set_;
        bool use_all_names_;
        bool is_string_;
        bool is_double_;
        std::vector<std::string> names_;
        std::vector< std::vector<std::string> > algorithms_;
        std::vector<size_t> ids_;    
        AlgorithmPerformer* ap_;
    };

    template <typename Map1, typename Map2>
        inline void Publish::write_publish(std::map<std::string,MessageVal>& vals, std::vector<MessageVar>& layout, Map1& out_str, Map2& out_dbl)
    {
        std::string out_var, out_val;
        fill_format(vals, layout, out_var, out_val);
        
        // pass through a MessageVal to do the type conversions
        MessageVal mv;
        mv.set(out_val);
        
        double out_dval = acomms_util::NaN;
        mv.val(out_dval);
        
        // see if our value is a double
        bool is_dbl = true;
        try { boost::lexical_cast<double>(out_val); }
        catch (boost::bad_lexical_cast &) { is_dbl = false; }

        // user sets to string
        if(is_string() || !(is_double() || is_dbl))
            out_str.insert(std::pair<std::string, std::string>(out_var, out_val));
        else // user sets to double or we catch it as a double
            out_dbl.insert(std::pair<std::string, double>(out_var, out_dval));
    }
    
    
    std::ostream& operator<< (std::ostream& out, const Publish& publish);
}

#endif
