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

#include "goby/acomms/dccl.h"
#include <iostream>

using goby::acomms::operator<<;

int main(int argc, char* argv[])
{
    std::string xml_file, xml_schema;
    switch(argc)
    {
        case 3:
            xml_schema = argv[2];
            // no break intentional
        case 2:
            xml_file = argv[1];
            break;

        default:
            std::cout << "usage: analyze_dccl_xml message_xml_file.xml [schema_path]" << std::endl;
            exit(EXIT_FAILURE);
    }

    std::cout << "creating DCCLCodec using xml file: [" << xml_file << "] and schema: [" << xml_schema << "]" << std::endl;
    std::cout << "schema must be specified with an absolute path or a relative path to the xml file location (not pwd!)" << std::endl;
    
    goby::acomms::DCCLCodec dccl(xml_file, xml_schema);

    std::cout << "parsed file ok!" << std::endl;
    
    std::cout << std::string(30, '#') << std::endl
              << "detailed message summary:" << std::endl
              << std::string(30, '#') << std::endl
              << dccl;
    
    return 0;
}
