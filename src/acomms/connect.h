
// gives functions for connecting the slots and signals of the goby-acomms libraries together

#ifndef CONNECT20110121H
#define CONNECT20110121H

#include <boost/bind.hpp>
#include <boost/signal.hpp>

namespace goby
{
    namespace acomms
    {   

        /// connect a signal to a slot (e.g. function pointer)
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
