// copyright 2011 t. schneider tes@mit.edu
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


// tests required versus optional encoding of fields using a presence bit

#include "goby/acomms/dccl.h"
#include "test.pb.h"

using goby::acomms::operator<<;

int main(int argc, char* argv[])
{
    goby::glog.add_stream(goby::util::logger::DEBUG3, &std::cerr);
    goby::glog.set_name(argv[0]);
    
    goby::acomms::DCCLCodec* codec = goby::acomms::DCCLCodec::get();
    
    codec->validate<BytesMsg>();
    codec->info<BytesMsg>(&goby::glog);

    BytesMsg msg_in;

    msg_in.set_req_bytes(goby::util::hex_decode("88abcd1122338754"));
    msg_in.set_opt_bytes(goby::util::hex_decode("102030adef2cb79d"));

    std::string encoded;
    codec->encode(&encoded, msg_in);
    
    BytesMsg msg_out;
    codec->decode(encoded, &msg_out);

    assert(msg_in.SerializeAsString() == msg_out.SerializeAsString());
    
    
    std::cout << "all tests passed" << std::endl;
}

