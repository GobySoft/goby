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


// tests all protobuf types with _default codecs, repeat and non repeat

#include <google/protobuf/descriptor.pb.h>

#include "goby/acomms/dccl.h"
#include "test.pb.h"
#include "goby/util/as.h"
#include "goby/common/time.h"
#include "goby/util/binary.h"

using goby::acomms::operator<<;

int main(int argc, char* argv[])
{
    goby::glog.add_stream(goby::common::logger::DEBUG3, &std::cerr);
    goby::glog.set_name(argv[0]);
    
    goby::acomms::protobuf::DCCLConfig cfg;
    goby::acomms::DCCLCodec* codec = goby::acomms::DCCLCodec::get();
    codec->set_cfg(cfg);

    TestMsg msg_in;
    int i = 0;
    msg_in.set_double_default_optional(++i + 0.1);
    msg_in.set_float_default_optional(++i + 0.2);

    msg_in.set_int32_default_optional(++i);
    msg_in.set_int64_default_optional(-++i);
    msg_in.set_uint32_default_optional(++i);
    msg_in.set_uint64_default_optional(++i);
    msg_in.set_sint32_default_optional(-++i);
    msg_in.set_sint64_default_optional(++i);
    msg_in.set_fixed32_default_optional(++i);
    msg_in.set_fixed64_default_optional(++i);
    msg_in.set_sfixed32_default_optional(++i);
    msg_in.set_sfixed64_default_optional(-++i);

    msg_in.set_bool_default_optional(true);

    msg_in.set_string_default_optional("abc123");
    msg_in.set_bytes_default_optional(goby::util::hex_decode("00112233aabbcc1234"));
    
    msg_in.set_enum_default_optional(ENUM_C);
    msg_in.mutable_msg_default_optional()->set_val(++i + 0.3);
    msg_in.mutable_msg_default_optional()->mutable_msg()->set_val(++i);

    msg_in.set_double_default_required(++i + 0.1);
    msg_in.set_float_default_required(++i + 0.2);

    msg_in.set_int32_default_required(++i);
    msg_in.set_int64_default_required(-++i);
    msg_in.set_uint32_default_required(++i);
    msg_in.set_uint64_default_required(++i);
    msg_in.set_sint32_default_required(-++i);
    msg_in.set_sint64_default_required(++i);
    msg_in.set_fixed32_default_required(++i);
    msg_in.set_fixed64_default_required(++i);
    msg_in.set_sfixed32_default_required(++i);
    msg_in.set_sfixed64_default_required(-++i);

    msg_in.set_bool_default_required(true);

    msg_in.set_string_default_required("abc123");
    msg_in.set_bytes_default_required(goby::util::hex_decode("00112233aabbcc1234"));
    
    msg_in.set_enum_default_required(ENUM_C);
    msg_in.mutable_msg_default_required()->set_val(++i + 0.3);
    msg_in.mutable_msg_default_required()->mutable_msg()->set_val(++i);

    
    for(int j = 0; j < 2; ++j)
    {
        msg_in.add_double_default_repeat(++i + 0.1);
        msg_in.add_float_default_repeat(++i + 0.2);
        
        msg_in.add_int32_default_repeat(++i);
        msg_in.add_int64_default_repeat(-++i);
        msg_in.add_uint32_default_repeat(++i);
        msg_in.add_uint64_default_repeat(++i);
        msg_in.add_sint32_default_repeat(-++i);
        msg_in.add_sint64_default_repeat(++i);
        msg_in.add_fixed32_default_repeat(++i);
        msg_in.add_fixed64_default_repeat(++i);
        msg_in.add_sfixed32_default_repeat(++i);
        msg_in.add_sfixed64_default_repeat(-++i);
        
        msg_in.add_bool_default_repeat(true);
        
        msg_in.add_string_default_repeat("abc123");

        if(j)
            msg_in.add_bytes_default_repeat(goby::util::hex_decode("00aabbcc"));
        else
            msg_in.add_bytes_default_repeat(goby::util::hex_decode("ffeedd12"));
        
        msg_in.add_enum_default_repeat(static_cast<Enum1>((++i % 3) + 1));
        EmbeddedMsg1* em_msg = msg_in.add_msg_default_repeat();
        em_msg->set_val(++i + 0.3);
        em_msg->mutable_msg()->set_val(++i);
    }
    
    codec->info(msg_in.GetDescriptor(), &std::cout);    

    std::ofstream fout("/tmp/testmessage.pb");
    msg_in.SerializeToOstream(&fout);
    
    
    std::cout << "Message in:\n" << msg_in.DebugString() << std::endl;
     
    codec->validate(msg_in.GetDescriptor());

    std::cout << "Try encode..." << std::endl;
    std::string bytes;
    codec->encode(&bytes, msg_in);
    std::cout << "... got bytes (hex): " << goby::util::hex_encode(bytes) << std::endl;

    std::cout << "Try decode..." << std::endl;

    TestMsg msg_out;
    codec->decode(bytes, &msg_out);
    
    std::cout << "... got Message out:\n" << msg_out.DebugString() << std::endl;


    // truncate to "max_length" as codec should do
    msg_in.set_string_default_repeat(0,"abc1");
    msg_in.set_string_default_repeat(1,"abc1");

    
    assert(msg_in.SerializeAsString() == msg_out.SerializeAsString());
    
    std::cout << "all tests passed" << std::endl;
}

