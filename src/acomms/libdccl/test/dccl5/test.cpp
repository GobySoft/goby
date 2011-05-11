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


// tests callback hooks on (using wire values generated at pre-encode)

#include "goby/acomms/dccl.h"
#include "goby/acomms/libdccl/dccl_field_codec_default.h"
#include "goby/acomms/libdccl/dccl_field_codec.h"
#include "test.pb.h"
#include "goby/protobuf/queue_option_extensions.pb.h"
#include "goby/util/string.h"
#include "goby/util/time.h"
#include "goby/util/binary.h"

using goby::acomms::operator<<;
using goby::acomms::int32;

bool found_dest = false;

void dest(const boost::any& wire_value, const boost::any& extension_value)
{
    found_dest = true;
    std::cout << "dest: type: `" << wire_value.type().name() << "`, wire_value: " << boost::any_cast<int32>(wire_value) << std::endl;
    
    assert(boost::any_cast<bool>(extension_value) == true);
    assert(boost::any_cast<int32>(wire_value) == 3);    
}


// template< ::google::protobuf::internal::ExtensionIdentifier ex>
// void number()
// {
// //    Extension ex;
// //    std::cout << "!!" << ex.number() << std::endl;
// }


int main()
{
//    goby::acomms::DCCLFieldCodecBase::register_wire_value_hook<queue::is_dest>(&dest);
//    number<queue::is_dest>();

    std::cout <<     queue::is_dest.number()<< std::endl;
    
    
    goby::acomms::DCCLCommon::set_log(&std::cerr);

    goby::acomms::DCCLModemIdConverterCodec::add("unicorn", 3);
    goby::acomms::DCCLModemIdConverterCodec::add("topside", 1);
    
    
    goby::acomms::DCCLCodec* codec = goby::acomms::DCCLCodec::get();
    goby::acomms::protobuf::DCCLConfig cfg;
    codec->set_cfg(cfg);

    GobyMessage msg_in1, msg_out1;

    msg_in1.set_telegram("hello!");
    msg_in1.mutable_header()->set_time(
        goby::util::as<std::string>(boost::posix_time::second_clock::universal_time()));
    msg_in1.mutable_header()->set_source_platform("topside");
    msg_in1.mutable_header()->set_dest_platform("unicorn");
    msg_in1.mutable_header()->set_dest_type(Header::PUBLISH_OTHER);

    codec->run_hooks(&msg_in1);
    assert(found_dest);

    
    codec->info(msg_in1.GetDescriptor(), &std::cout);    
    std::cout << "Message in:\n" << msg_in1.DebugString() << std::endl;
    assert(codec->validate(msg_in1.GetDescriptor()));

    // try callbacks
    
    std::cout << "Try encode..." << std::endl;
    std::string bytes1 = codec->encode(msg_in1);
    std::cout << "... got bytes (hex): " << goby::util::hex_encode(bytes1) << std::endl;
    std::cout << "Try decode..." << std::endl;
    msg_out1 = codec->decode<GobyMessage>(bytes1);
    std::cout << "... got Message out:\n" << msg_out1.DebugString() << std::endl;
    assert(msg_in1.SerializeAsString() == msg_out1.SerializeAsString());

    
    std::cout << "all tests passed" << std::endl;
}

