// t. schneider tes@mit.edu 12.23.08
// ocean engineering graduate student - mit / whoi joint program
// massachusetts institute of technology (mit)
// laboratory for autonomous marine sensing systems (lamss)
// 
// this is message_algorithms.h, part of libdccl
//
// see the readme file within this directory for information
// pertaining to usage and purpose of this script.
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
        typedef boost::function3<void, std::string&, const std::vector<std::string>&, const std::map<std::string,MessageVal>&> StrAlgFunction3;
        typedef boost::function3<void, double&, const std::vector<std::string>&, const std::map<std::string,MessageVal>&> DblAlgFunction3;
        typedef boost::function3<void, long&, const std::vector<std::string>&, const std::map<std::string,MessageVal>&> LongAlgFunction3;
        typedef boost::function3<void, bool&, const std::vector<std::string>&, const std::map<std::string,MessageVal>&> BoolAlgFunction3;


        void add_str_algorithm(const std::string& name, StrAlgFunction1 func)
        { str_map1_[name] = func; }        
        void add_dbl_algorithm(const std::string& name, DblAlgFunction1 func)
        { dbl_map1_[name] = func; }        
        void add_long_algorithm(const std::string& name, LongAlgFunction1 func)
        { long_map1_[name] = func; }        
        void add_bool_algorithm(const std::string& name, BoolAlgFunction1 func)
        { bool_map1_[name] = func; }
        
        void add_adv_str_algorithm(const std::string& name, StrAlgFunction3 func)
        { str_map3_[name] = func; }        
        void add_adv_dbl_algorithm(const std::string& name, DblAlgFunction3 func)
        { dbl_map3_[name] = func; }        
        void add_adv_long_algorithm(const std::string& name, LongAlgFunction3 func)
        { long_map3_[name] = func; }        
        void add_adv_bool_algorithm(const std::string& name, BoolAlgFunction3 func)
        { bool_map3_[name] = func; }        
        
      private:
        static AlgorithmPerformer* inst_;
        std::map<std::string, StrAlgFunction1> str_map1_;
        std::map<std::string, DblAlgFunction1> dbl_map1_;
        std::map<std::string, LongAlgFunction1> long_map1_;
        std::map<std::string, BoolAlgFunction1> bool_map1_;

        std::map<std::string, StrAlgFunction3> str_map3_;
        std::map<std::string, DblAlgFunction3> dbl_map3_;
        std::map<std::string, LongAlgFunction3> long_map3_;
        std::map<std::string, BoolAlgFunction3> bool_map3_;
        
      AlgorithmPerformer()
            {}
        
        AlgorithmPerformer(const AlgorithmPerformer&);
        AlgorithmPerformer& operator = (const AlgorithmPerformer&);
        
    };
}

#endif
