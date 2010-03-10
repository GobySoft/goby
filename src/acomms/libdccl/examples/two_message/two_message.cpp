// t. schneider tes@mit.edu 3.31.09
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


// encodes/decodes the message given in the pGeneralCodec documentation
// also includes the simple.xml file to show example of DCCLCodec instantiation
// with multiple files
#include "acomms/dccl.h"
#include <exception>
#include <iostream>

using dccl::operator<<;


int main()
{
    std::cout << "loading xml files: xml/simple.xml, xml/two_message.xml" << std::endl;

    std::set<std::string> xml_files;
    xml_files.insert(DCCL_EXAMPLES_DIR "/dccl_simple/simple.xml");
    xml_files.insert(DCCL_EXAMPLES_DIR "/two_message/two_message.xml");
    
    dccl::DCCLCodec dccl(xml_files, "../../message_schema.xsd");

    // show some useful information about all the loaded messages
    std::cout << std::string(30, '#') << std::endl
              << "number of messages loaded: " << dccl.message_count() << std::endl
              << std::string(30, '#') << std::endl;

    std::cout << std::string(30, '#') << std::endl
              << "detailed message summary:" << std::endl
              << std::string(30, '#') << std::endl
              << dccl;

    std::cout << std::string(30, '#') << std::endl
              << "brief message summary:" << std::endl
              << std::string(30, '#') << std::endl
              << dccl.brief_summary();
    
    std::cout << std::string(30, '#') << std::endl
              << "all loaded ids:" << std::endl
              << std::string(30, '#') << std::endl
              << dccl.all_message_ids();
    
    std::cout << std::string(30, '#') << std::endl
              << "all loaded names:" << std::endl
              << std::string(30, '#') << std::endl
              << dccl.all_message_names();    

    
    std::cout << std::string(30, '#') << std::endl
              << "all required message var names:" << std::endl
              << std::string(30, '#') << std::endl;
    
     
    std::set<unsigned> s = dccl.all_message_ids();
    for (std::set<unsigned>::const_iterator it = s.begin(), n = s.end(); it != n; ++it)
        std::cout << (*it) << ": " << dccl.message_var_names(*it) << std::endl;

    std::cout << std::string(30, '#') << std::endl
              << "ENCODE / DECODE example:" << std::endl
              << std::string(30, '#') << std::endl;

    // initialize input contents to encoder
    std::map<std::string, dccl::MessageVal> vals;
    
    // initialize output hexadecimal
    std::string hex2, hex3;

    // id = 2, name = GoToCommand
    vals["goto_x"] = 423;
    vals["goto_y"] = 523;
    vals["lights_on"] = true;
    vals["new_instructions"] = "make_toast";
    vals["goto_speed"] = 2.3456;
    
    // id = 3, name = VehicleStatus
    vals["nav_x"] = 234.5;
    vals["nav_y"] = 451.3;
    vals["health"] = "abort";

    std::cout << "passing values to encoder:" << std::endl  
              << vals;

    dccl.encode(2, hex2, vals);
    dccl.encode(3, hex3, vals);
    
    std::cout << "received hexadecimal string for message 2 (GoToCommand): "
              << hex2
              << std::endl;

    std::cout << "received hexadecimal string for message 3 (VehicleStatus): "
              << hex3
              << std::endl;

    vals.clear();
    
    std::cout << "passed hexadecimal string for message 2 to decoder: " 
              << hex2
              << std::endl;
    
    std::cout << "passed hexadecimal string for message 3 to decoder: " 
              << hex3
              << std::endl;


    dccl.decode(2, hex2, vals);
    dccl.decode(3, hex3, vals);
    
    std::cout << "received values:" << std::endl
              << vals;
    
    return 0;
}

