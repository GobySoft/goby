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


#include "goby/acomms/dccl.h"
#include "test.pb.h"
#include "goby/util/string.h"
#include "goby/util/time.h"

using goby::acomms::operator<<;

int main()
{
    goby::acomms::DCCLCodec dccl;
    dccl.set_log(&std::cerr);    
    goby::acomms::protobuf::DCCLConfig cfg;
//    cfg.set_crypto_passphrase("my_passphrase!");
    dccl.set_cfg(cfg);

    BasicTestMsg msg_in1, msg_out1;    

    msg_in1.set_double_default(14);
//    msg_in1.mutable_head()->set_time(goby::util::as<std::string>(goby::util::goby_time));
    
    std::cout << "Message 1 in:\n" << msg_in1 << std::endl;

    assert(dccl.validate(msg_in1.descriptor()));

    std::cout << "Try encode..." << std::endl;
    std::string bytes;
    dccl.encode(&bytes, msg_in1);
    std::cout << "... got bytes (hex): " << goby::acomms::hex_encode(bytes) << std::endl;

    std::cout << "Try decode..." << std::endl;
    dccl.decode(bytes, &msg_out1);
    std::cout << "... got Message 1 out:\n" << msg_out1 << std::endl;

    
    std::cout << "all tests passed" << std::endl;

    return 0;
    
    TestMsg msg_in2;
    int i = 0;
    msg_in2.set_double_default(++i + 0.1);
    msg_in2.set_float_default(++i + 0.2);

    msg_in2.set_int32_default(++i);
    msg_in2.set_int64_default(-++i);
    msg_in2.set_uint32_default(++i);
    msg_in2.set_uint64_default(++i);
    msg_in2.set_sint32_default(-++i);
    msg_in2.set_sint64_default(++i);
    msg_in2.set_fixed32_default(++i);
    msg_in2.set_fixed64_default(++i);
    msg_in2.set_sfixed32_default(++i);
    msg_in2.set_sfixed64_default(-++i);

    msg_in2.set_bool_default(true);

    msg_in2.set_string_default("abc123");
    msg_in2.set_bytes_default(goby::acomms::hex_decode("aabbcc1234"));
    
    msg_in2.set_enum_default(ENUM_C);
    msg_in2.mutable_msg_default()->set_val(++i + 0.3);
    msg_in2.mutable_msg_default()->mutable_msg()->set_val(++i);

    for(int j = 0; j < 2; ++j)
    {
        msg_in2.add_double_default_repeat(++i + 0.1);
        msg_in2.add_float_default_repeat(++i + 0.2);
        
        msg_in2.add_int32_default_repeat(++i);
        msg_in2.add_int64_default_repeat(-++i);
        msg_in2.add_uint32_default_repeat(++i);
        msg_in2.add_uint64_default_repeat(++i);
        msg_in2.add_sint32_default_repeat(-++i);
        msg_in2.add_sint64_default_repeat(++i);
        msg_in2.add_fixed32_default_repeat(++i);
        msg_in2.add_fixed64_default_repeat(++i);
        msg_in2.add_sfixed32_default_repeat(++i);
        msg_in2.add_sfixed64_default_repeat(-++i);
        
        msg_in2.add_bool_default_repeat(true);
        
        msg_in2.add_string_default_repeat("abc123");
        msg_in2.add_bytes_default_repeat(goby::acomms::hex_decode("aabbcc1234"));
        
        msg_in2.add_enum_default_repeat(static_cast<Enum1>((++i % 3) + 1));
        EmbeddedMsg1* em_msg = msg_in2.add_msg_default_repeat();
        em_msg->set_val(++i + 0.3);
        em_msg->mutable_msg()->set_val(++i);
    }
}

