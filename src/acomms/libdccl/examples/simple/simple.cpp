// t. schneider tes@mit.edu 3.31.09
// ocean engineering graduate student - mit / whoi joint program
// massachusetts institute of technology (mit)
// laboratory for autonomous marine sensing systems (lamss)
// 
// this is simple.cpp
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


// encodes/decodes a string using the DCCL codec library
// assumes prior knowledge of the message format (required fields)

#include "../../dccl.h"
#include <iostream>

using dccl::operator<<;

int main()
{
    // initialize input contents to encoder
    // simple.xml only has a <string/> message var
    // so we only need one map
    std::map<std::string, std::string> strings;

    // initialize output hexadecimal (in WHOI micromodem format
    // with CCL header in most significant byte)
    std::string hex;
    
    std::cout << "loading xml file: xml/simple.xml" << std::endl;

    // instantiate the parser with a single xml file
    dccl::DCCLCodec dccl("../src/acomms/lib_dccl/examples/simple/simple.xml", "../../message_schema.xsd");

    // read message content (in this case from terminal window)
    std::string input;
    std::cout << "input a string to send: " << std::endl;
    getline(std::cin, input);
    
    // key is <name/> child for a given message_var
    // (i.e. child of <int/>, <string/>, etc)
    strings["value"] = input;

    std::cout << "passing values to encoder:\n" << strings;

    // we are encoding for message id 1 - given in simple.xml
    // we pass null pointers (0) for the remaining maps (double, long, bool)
    // since we don't have any message vars that require these C types
    // don't forget to pass the pointer(s) for the map! (&)
    dccl.encode(1, hex, &strings, 0, 0, 0);
    
    std::cout << "received hexadecimal string: " << hex << std::endl;
    
    strings.clear();
    
    // input contents right back to encoder
    std::cout << "passed hexadecimal string to decoder: " << hex << std::endl;


    std::map<std::string, double> doubles;
    
    dccl.decode(1, hex, &strings, 0, 0, 0);
    
    std::cout << "received values:" << std::endl 
              << strings;
    
    return 0;
}
