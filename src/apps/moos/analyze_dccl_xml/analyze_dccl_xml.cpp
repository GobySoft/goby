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

#include "goby/moos/libtransitional/dccl_transitional.h"
#include <iostream>

using goby::acomms::operator<<;

int main(int argc, char* argv[])
{
    std::string xml_file;
    switch(argc)
    {
        case 2:
            xml_file = argv[1];
            break;

        default:
            std::cout << "usage: analyze_dccl_xml message_xml_file.xml" << std::endl;
            exit(EXIT_FAILURE);
    }

    std::cout << "creating DCCLTransitionalCodec using xml file: [" << xml_file << "]" << std::endl;
    
    goby::transitional::DCCLTransitionalCodec dccl;    
    goby::transitional::protobuf::DCCLTransitionalConfig cfg;
    cfg.add_message_file()->set_path(xml_file);
    dccl.set_cfg(cfg);

    std::cout << "parsed file ok!" << std::endl;
    
    std::cout << std::string(30, '#') << std::endl
              << "detailed message summary:" << std::endl
              << std::string(30, '#') << std::endl
              << dccl;
    
    return 0;
}
