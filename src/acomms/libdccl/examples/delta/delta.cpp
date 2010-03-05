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
    std::vector< std::map<std::string, double> > doubles_vect;
    std::map<std::string, double> doubles;
    
    //  initialize output hexadecimal
    std::string hex;
    
    std::cout << "loading xml file: delta.xml" << std::endl;

    dccl::DCCLCodec dccl(DCCL_EXAMPLES_DIR "/delta/delta.xml",
                         "../../message_schema.xsd");

    std::cout << dccl << std::endl;
    
    
    doubles["time"] = 1250000000;
    doubles["temp"] = 15.23;
    doubles["sal"] = 37.9;
    doubles["depth"] = 10;

    doubles_vect.push_back(doubles);

    std::cout << "passing key frame values to encoder:\n" << doubles;
    
    doubles["time"] = 1250000010;
    doubles["temp"] = 15.4;
    doubles["sal"] = 37.8;
    doubles["depth"] = 14;

    doubles_vect.push_back(doubles);

    std::cout << "passing delta frame values to encoder:\n" << doubles;
    
    doubles["time"] = 1250000020;
    doubles["temp"] = 15.7;
    doubles["sal"] = 37.7;
    doubles["depth"] = 19;

    doubles_vect.push_back(doubles);

    std::cout << "passing delta frame values to encoder:\n" << doubles;

    unsigned id = 4;
//    dccl.delta_encode(id, hex, 0, &doubles_vect, 0, 0);
    
    std::cout << "received hexadecimal string: " << hex << std::endl;
    
    doubles_vect.clear();
    
    // input contents right back to decoder
    std::cout << "passed hexadecimal string to decoder: " << hex << std::endl;

//    dccl.delta_decode(1, hex, 0, &doubles_vect, 0, 0);
    
    for(std::vector< std::map<std::string, double> >::iterator
            it = doubles_vect.begin(),
            n = doubles_vect.end();
        it != n;
        ++it)
    {
        std::cout << "received values:" << std::endl 
                  << *it;
    }
    
    return 0;
}

