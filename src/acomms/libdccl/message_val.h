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

#ifndef MESSAGE_VAL20091211H
#define MESSAGE_VAL20091211H

#include <iostream>

#include <boost/numeric/conversion/cast.hpp>
#include <boost/lexical_cast.hpp> 

#include "util/tes_utils.h"
#include "dccl_constants.h"

namespace dccl
{
    
/// defines a DCCL value
    class MessageVal
    {
      public:
        enum { MAX_DBL_PRECISION = 15 };
        
        /// \name Constructors/Destructor
        //@{         
        /// empty
        MessageVal();
        
        /// construct with string value
        MessageVal(const std::string& s);
        
        /// construct with char* value
        MessageVal(const char* s);
        
        /// construct with double value, optionally giving the precision of the double (number of decimal places) which is used if a cast to std::string is required in the future.
        MessageVal(double d, int p = MAX_DBL_PRECISION);
        
        /// construct with long value
        MessageVal(long l);
        
        /// construct with int value
        MessageVal(int i);
        
        /// construct with float value
        MessageVal(float f);
        
        /// construct with bool value
        MessageVal(bool b);
        
        //@}

        /// \name Setters
        //@{  
        /// set the value with a string (overwrites previous value regardless of type)
        void set(std::string sval);
        /// \brief set the value with a double (overwrites previous value regardless of type)
        /// \param dval values to set
        /// \param precision decimal places of precision to preserve if this is cast to a string
        void set(double dval, int precision = MAX_DBL_PRECISION);
        /// set the value with a long (overwrites previous value regardless of type)
        void set(long lval);
        /// set the value with a bool (overwrites previous value regardless of type)
        void set(bool bval);

       //@}
        
        /// \name Getters
        //@{       
        /// \brief extract as std::string (all reasonable casts are done)
        /// \param s std::string to store value in
        /// \return successfully extracted (and if necessary successfully cast to this type)
        bool get(std::string& s) const;
        /// \brief extract as bool (all reasonable casts are done)
        /// \param b bool to store value in
        /// \return successfully extracted (and if necessary successfully cast to this type)
        bool get(bool& b) const;
        /// \brief extract as long (all reasonable casts are done)
        /// \param t long to store value in
        /// \return successfully extracted (and if necessary successfully cast to this type)
        bool get(long& t) const;        
        /// \brief extract as double (all reasonable casts are done)
        /// \param d double to store value in
        /// \return successfully extracted (and if necessary successfully cast to this type)
        bool get(double& d) const;

        ///
        /// allows statements of the form \code double d = MessageVal("3.23"); \endcode
        operator double() const;

        ///
        /// allows statements of the form \code bool b = MessageVal("1"); \endcode
        operator bool() const;

        ///
        /// allows statements of the form \code std::string s = MessageVal(3); \endcode
        operator std::string() const;

        ///
        /// allows statements of the form \code long l = MessageVal(5); \endcode
        operator long() const;

        ///
        /// allows statements of the form \code int i = MessageVal(2); \endcode
        operator int() const;

        ///
        /// allows statements of the form \code unsigned u = MessageVal(2); \endcode
        operator unsigned() const;

        ///
        /// allows statements of the form \code float f = MessageVal("3.5"); \endcode
        operator float() const;

        

        /// what type is the original type of this MessageVal?
        DCCLCppType type() const { return type_; }
        
        /// was this just constructed with MessageVal() ? 
        bool empty() const { return type_ == cpp_notype; }        
//@}


        /// \name Comparison
        //@{       
        
        bool operator==(const std::string& s);
        bool operator==(double d);
        bool operator==(long l);
        bool operator==(bool b);
        
        // @}
        
      private:
        friend std::ostream& operator<<(std::ostream& os, const MessageVal& mv);
        
      private:
        std::string sval_;
        double dval_;
        long lval_;
        bool bval_;

        unsigned precision_;

        DCCLCppType type_;
    };

    std::ostream& operator<<(std::ostream& os, const dccl::MessageVal& mv);
}



#endif
