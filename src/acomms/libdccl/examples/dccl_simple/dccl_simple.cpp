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


// encodes/decodes a string using the DCCL codec library
// assumes prior knowledge of the message format (required fields)

#include "goby/acomms/dccl.h"
#include <iostream>

using goby::acomms::operator<<;

int main()
{
    // initialize input contents to encoder. since
    // simple.xml only has a <string/> message var
    // we only need one map.
    std::map<std::string, goby::acomms::DCCLMessageVal> val_map;

    //  initialize output bytes
    std::string bytes;
    
    std::cout << "loading xml file: xml/simple.xml" << std::endl;

    // instantiate the parser with a single xml file (simple.xml).
    // also pass the schema, relative to simple.xml, for XML validity
    // checking (syntax).
    goby::acomms::DCCLCodec dccl;    
    goby::acomms::protobuf::DCCLConfig cfg;
    cfg.set_schema("../../message_schema.xsd");
    cfg.add_message_file()->set_path(DCCL_EXAMPLES_DIR "/dccl_simple/simple.xml");
    dccl.set_cfg(cfg);
    
    // read message content (in this case from the command line)
    std::string input;
    std::cout << "input a string to send: " << std::endl;
    getline(std::cin, input);
    
    // key is <name/> child for a given message_var
    // (i.e. child of <int/>, <string/>, etc)
    val_map["s_key"] = input;    
    
    std::cout << "passing values to encoder:\n" << val_map;

    // we are encoding for message id 1 - given in simple.xml
    unsigned id = 1;
    dccl.encode(id, bytes, val_map);
    
    std::cout << "received hexadecimal string: " << goby::acomms::hex_encode(bytes) << std::endl;
    
    val_map.clear();
    
    // input contents right back to decoder
    std::cout << "passed hexadecimal string to decoder: " << goby::acomms::hex_encode(bytes) << std::endl;

    dccl.decode(bytes, val_map);
    
    std::cout << "received values:" << std::endl 
              << val_map;

    // now you can use it...
    std::string output = val_map["s_key"];
    
    return 0;
}

