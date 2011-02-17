// copyright 2011 t. schneider tes@mit.edu
// 
// this file is part of goby-acomms, which is a collection of libraries 
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

// gives functions for connecting the slots and signals of the goby-acomms libraries together

#ifndef CONNECT20110121H
#define CONNECT20110121H

#include <boost/bind.hpp>
#include <boost/signal.hpp>

namespace goby
{
    namespace acomms
    {   

        /// connect a signal to a function slot (non-member of a class)
        template<typename Signal, typename Slot>
            void connect(Signal* signal, Slot slot)
        { signal->connect(slot); }

        /// connect a signal to a member function with one argument
        template<typename Signal, typename Obj, typename A1> 
            void connect(Signal* signal, Obj* obj, void(Obj::*mem_func)(A1))
        { connect(signal, boost::bind(mem_func, obj, _1)); }

        /// connect a signal to a member function with two arguments
        template<typename Signal, typename Obj, typename A1, typename A2> 
            void connect(Signal* signal, Obj* obj, void(Obj::*mem_func)(A1, A2))
        { connect(signal, boost::bind(mem_func, obj, _1, _2)); }

        /// connect a signal to a member function with three arguments
        template<typename Signal, typename Obj, typename A1, typename A2, typename A3> 
            void connect(Signal* signal, Obj* obj, void(Obj::*mem_func)(A1, A2, A3))
        { connect(signal, boost::bind(mem_func, obj, _1, _2, _3)); }
    }
}

#endif
