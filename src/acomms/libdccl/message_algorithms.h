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

#ifndef MESSAGE_ALGORITHMSH
#define MESSAGE_ALGORITHMSH

#include <map>
#include <string>
#include <vector>
#include <iostream>
#include <cctype>

#include <boost/algorithm/string.hpp>
#include <boost/function.hpp>

namespace dccl
{
    class MessageVal;
    class AlgorithmPerformer
    {
      public:
        static AlgorithmPerformer* getInstance();

        void algorithm(MessageVal& in, const std::string& algorithm, const std::map<std::string,MessageVal>& vals);


        typedef boost::function1<void, std::string&> StrAlgFunction1;
        typedef boost::function1<void, double& > DblAlgFunction1;
        typedef boost::function1<void, long& > LongAlgFunction1;
        typedef boost::function1<void, bool& > BoolAlgFunction1;
        typedef boost::function1<void, MessageVal&> AdvAlgFunction1;
        typedef boost::function3<void, MessageVal&, const std::vector<std::string>&, const std::map<std::string,MessageVal>&> AdvAlgFunction3;

        void add_str_algorithm(const std::string& name, StrAlgFunction1 func)
        { str_map1_[name] = func; }        
        void add_dbl_algorithm(const std::string& name, DblAlgFunction1 func)
        { dbl_map1_[name] = func; }        
        void add_long_algorithm(const std::string& name, LongAlgFunction1 func)
        { long_map1_[name] = func; }        
        void add_bool_algorithm(const std::string& name, BoolAlgFunction1 func)
        { bool_map1_[name] = func; }

        void add_generic_algorithm(const std::string& name, AdvAlgFunction1 func)
        { adv_map1_[name] = func; }        

        void add_adv_algorithm(const std::string& name, AdvAlgFunction3 func)
        { adv_map3_[name] = func; }        
        
      private:
        static AlgorithmPerformer* inst_;
        std::map<std::string, StrAlgFunction1> str_map1_;
        std::map<std::string, DblAlgFunction1> dbl_map1_;
        std::map<std::string, LongAlgFunction1> long_map1_;
        std::map<std::string, BoolAlgFunction1> bool_map1_;

        std::map<std::string, AdvAlgFunction1> adv_map1_;
        std::map<std::string, AdvAlgFunction3> adv_map3_;
        
        AlgorithmPerformer()
        {}
        
        AlgorithmPerformer(const AlgorithmPerformer&);
        AlgorithmPerformer& operator = (const AlgorithmPerformer&);
      
    };
}

#endif
