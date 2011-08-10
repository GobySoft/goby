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

#include "goby/acomms/dccl.h" // for DCCLCodec
#include "simple.pb.h" // for `Simple` protobuf message defined in simple.proto
#include "goby/util/binary.h" // for goby::util::hex_encode

using goby::acomms::operator<<;

int main()
{
    std::cout << "loading xml file: xml/simple.xml" << std::endl;
    
    // instantiate the parser with a single xml file (simple.xml).
    // also pass the schema, relative to simple.xml, for XML validity
    // checking (syntax).
    goby::acomms::DCCLCodec* dccl = goby::acomms::DCCLCodec::get();

    // validate the Simple protobuf message type as a valid DCCL message type
    dccl->validate<Simple>();
    
    // read message content (in this case from the command line)
    Simple message;
    std::cout << "input a string to send up to 10 characters: " << std::endl;
    getline(std::cin, *message.mutable_telegram());

    std::cout << "passing message to encoder:\n" << message << std::endl;

    // encode the message to a byte string
    std::string bytes = dccl->encode(message);
    
    std::cout << "received hexadecimal string: " << goby::util::hex_encode(bytes) << std::endl;

    message.Clear();
    // input contents right back to decoder
    std::cout << "passed hexadecimal string to decoder: " << goby::util::hex_encode(bytes) << std::endl;

    message = dccl->decode<Simple>(bytes);
    
    std::cout << "received message:" << message << std::endl;

    
    return 0;
}

