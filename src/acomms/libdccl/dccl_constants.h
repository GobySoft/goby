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

#ifndef DCCLConstants20091211H
#define DCCLConstants20091211H

#include <string>
#include <bitset>
#include <limits>

#include "acomms/acomms_constants.h"

namespace dccl
{

/// Enumeration of DCCL types used for sending messages. dccl_enum and dccl_string primarily map to cpp_string, dccl_bool to cpp_bool, dccl_int to cpp_long, dccl_float to cpp_double
    enum DCCLType { dccl_static, /*!<  \ref tag_static */
                    dccl_bool, /*!< \ref tag_bool */
                    dccl_int, /*!< \ref tag_int */
                    dccl_float, /*!< \ref tag_float */
                    dccl_enum, /*!< \ref tag_enum */
                    dccl_string, /*!< \ref tag_string */
                    dccl_hex  /*!< \ref tag_hex */
    };
/// Enumeration of C++ types used in DCCL.
    enum DCCLCppType { cpp_notype,/*!< not one of the C++ types used in DCCL */
                       cpp_bool,/*!< C++ bool */
                       cpp_string,/*!< C++ std::string */
                       cpp_long,/*!< C++ long */
                       cpp_double/*!< C++ double */
    };


    // 2^^3 = 8
    enum { POWER2_BITS_IN_BYTE = 3 };
    inline unsigned bits2bytes(unsigned bits) { return bits >> POWER2_BITS_IN_BYTE; }
    inline unsigned bytes2bits(unsigned bytes) { return bytes << POWER2_BITS_IN_BYTE; }
    
    // 2^^1 = 2
    enum { POWER2_NIBS_IN_BYTE = 1 };
    inline unsigned bytes2nibs(unsigned bytes) { return bytes << POWER2_NIBS_IN_BYTE; }
    inline unsigned nibs2bytes(unsigned nibs) { return nibs >> POWER2_NIBS_IN_BYTE; }


    inline std::string type_to_string(DCCLType type)
    {
        switch(type)
        {
            case dccl_int:    return "int";
            case dccl_hex:    return "hex";
            case dccl_bool:   return "bool";
            case dccl_string: return "string";
            case dccl_float:  return "float";
            case dccl_static: return "static";
            case dccl_enum:   return "enum";
        }
        return "notype";
    }

    inline std::string type_to_string(DCCLCppType type)
    {
        switch(type)
        {
            case cpp_long:   return "long";
            case cpp_double: return "double";
            case cpp_string: return "string";
            case cpp_bool:   return "bool";
            case cpp_notype: return "no_type";
        }
        return "notype";
    }

}

#endif
