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

#ifndef MESSAGE_ALGORITHMS20091211H
#define MESSAGE_ALGORITHMS20091211H

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
    
    /// \brief boost::function for a function taking a single dccl::MessageVal reference. Used for algorithm callbacks.
    ///
    /// Think of this as a generalized version of a function pointer (void (*)(dccl::MessageVal&)). See http://www.boost.org/doc/libs/1_34_0/doc/html/function.html for more on boost:function.
    typedef boost::function<void (MessageVal&)> AlgFunction1;
    /// \brief boost::function for a function taking a dccl::MessageVal reference, a std::vector<std::string> (the parameters for the algorithm), and a std::map<std::string,dccl::MessageVal> (map of all the other message variable values in the current message). Used for algorithm callbacks.
    ///
    /// Think of this as a generalized version of a function pointer (void (*)(MessageVal&, const std::vector<std::string>&, const std::map<std::string,dccl::MessageVal>&)). See http://www.boost.org/doc/libs/1_34_0/doc/html/function.html for more on boost:function.
    typedef boost::function<void (MessageVal&, const std::vector<std::string>&, const std::map<std::string,MessageVal>&)> AlgFunction3;

    class AlgorithmPerformer
    {
      public:
        static AlgorithmPerformer* getInstance();

        void algorithm(MessageVal& in, const std::string& algorithm, const std::map<std::string,MessageVal>& vals);
        
        void add_algorithm(const std::string& name, AlgFunction1 func)
        { adv_map1_[name] = func; }

        void add_adv_algorithm(const std::string& name, AlgFunction3 func)
        { adv_map3_[name] = func; }
        
      private:
        static AlgorithmPerformer* inst_;
        std::map<std::string, AlgFunction1> adv_map1_;
        std::map<std::string, AlgFunction3> adv_map3_;
        
        AlgorithmPerformer()
        {}
        
        AlgorithmPerformer(const AlgorithmPerformer&);
        AlgorithmPerformer& operator = (const AlgorithmPerformer&);
      
    };
}

#endif
