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

#ifndef PUBLISH20091211H
#define PUBLISH20091211H

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
            type_(cpp_notype),
            ap_(AlgorithmPerformer::getInstance())
            { }        

        //set
    
        void set_var(std::string var) {var_=var;}
        void set_format(std::string format) {format_=format; format_set_ = true;}
        void set_use_all_names(bool use_all_names) {use_all_names_ = use_all_names;}
        void set_type(DCCLCppType type) {type_ = type;}
    
        void add_name(const std::string& name) {names_.push_back(name);}
        void add_algorithms(const std::vector<std::string> algorithms) {algorithms_.push_back(algorithms);}
        void add_id(size_t id) {ids_.push_back(id);}
    
        //get
//    std::string var() const {return var_;}
//    std::string format() const {return format_;}
//    bool format_set()  const {return format_set_;}
//    bool use_all_names() const {return use_all_names_;}
    
        DCCLCppType type() {return type_;}
//    std::vector<std::string> const& names() {return names_;}
//    std::vector<std::vector<std::string> > const& algorithms(){return algorithms_;}
    
        
        std::string get_display() const;

        void write_publish(const std::map<std::string,MessageVal>& vals,
                           std::vector<MessageVar>& layout,
                           std::multimap<std::string,MessageVal>& pubsub_vals);
        
        
        

        void initialize(std::vector<MessageVar>& layout);

      private:
        void fill_format(const std::map<std::string,MessageVal>& vals,
                         const std::vector<MessageVar>& layout,
                         std::string& key,
                         std::string& value);
            
      private:
        std::string var_;
        std::string format_;
        bool format_set_;
        bool use_all_names_;
        DCCLCppType type_;
        std::vector<std::string> names_;
        std::vector< std::vector<std::string> > algorithms_;
        std::vector<size_t> ids_;    
        AlgorithmPerformer* ap_;
    };

    std::ostream& operator<< (std::ostream& out, const Publish& publish);
}

#endif
