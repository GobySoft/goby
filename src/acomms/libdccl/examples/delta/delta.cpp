// copyright 2009 t. schneider tes@mit.edu
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


// encodes/decodes several fields using delta encoding


#include "acomms/dccl.h"
#include <iostream>

using dccl::operator<<;

int main()
{
    std::vector< std::map<std::string, dccl::MessageVal> > vals_vect;
    std::map<std::string, dccl::MessageVal> vals;
    
    //  initialize output hexadecimal
    std::string hex;
    
    std::cout << "loading xml file: delta.xml" << std::endl;

    dccl::DCCLCodec dccl(DCCL_EXAMPLES_DIR "/delta/delta.xml",
                         "../../message_schema.xsd");

    std::cout << dccl << std::endl;
    
    
    vals["time"] = 1250000000;
    vals["temp"] = 15.23;
    vals["sal"] = 37.9;
    vals["depth"] = 10;

    vals_vect.push_back(vals);

    std::cout << "passing key frame values to encoder:\n" << vals;
    
    vals["time"] = 1250000010;
    vals["temp"] = 15.4;
    vals["sal"] = 37.8;
    vals["depth"] = 14;

    vals_vect.push_back(vals);

    std::cout << "passing delta frame values to encoder:\n" << vals;
    
    vals["time"] = 1250000020;
    vals["temp"] = 15.7;
    vals["sal"] = 37.7;
    vals["depth"] = 19;

    vals_vect.push_back(vals);

    std::cout << "passing delta frame values to encoder:\n" << vals;

    unsigned id = 4;
//    dccl.delta_encode(id, hex, 0, &vals_vect, 0, 0);
    
    std::cout << "received hexadecimal string: " << hex << std::endl;
    
    vals_vect.clear();
    
    // input contents right back to decoder
    std::cout << "passed hexadecimal string to decoder: " << hex << std::endl;

//    dccl.delta_decode(1, hex, 0, &vals_vect, 0, 0);
    
    for(std::vector< std::map<std::string, dccl::MessageVal> >::iterator
            it = vals_vect.begin(),
            n = vals_vect.end();
        it != n;
        ++it)
    {
        std::cout << "received values:" << std::endl 
                  << *it;
    }
    
    return 0;
}

