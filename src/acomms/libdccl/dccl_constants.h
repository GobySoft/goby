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

#ifndef DCCLConstantsH
#define DCCLConstantsH

#include <string>
#include <bitset>
#include <limits>

#include "acomms_constants.h"
namespace dccl
{

/// Enumeration of DCCL types used for sending messages. dccl_enum and dccl_string primarily map to std::string, dccl_bool to bool, dccl_int to long, dccl_float to double
    enum DCCLType { dccl_static, /*!< <static/> */
                    dccl_bool, /*!< <bool/> */
                    dccl_int, /*!< <int/> */
                    dccl_float, /*!< <float/> */
                    dccl_enum, /*!< <enum/> */
                    dccl_string /*!< <string/> */
    };
/// Enumeration of C++ types used in DCCL.
    enum DCCLCppType { cpp_notype,/*!< not one of the C++ types used in DCCL */
                       cpp_bool,/*!< bool */
                       cpp_string,/*!< std::string */
                       cpp_long,/*!< long */
                       cpp_double/*!< double */
    };

}

#endif
