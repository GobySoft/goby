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
bool found_source = false;
bool found_time = false;

boost::posix_time::ptime current_time = boost::posix_time::second_clock::universal_time();

void dest(const boost::any& wire_value, const boost::any& extension_value)
{
    std::cout << "dest: type: `" << wire_value.type().name() << "`, wire_value: " << boost::any_cast<int32>(wire_value) << std::endl;
    
    assert(boost::any_cast<bool>(extension_value) == true);
    assert(boost::any_cast<int32>(wire_value) == 3);    
    found_dest = true;
}


void source(const boost::any& wire_value, const boost::any& extension_value)
{
    std::cout << "source: type: `" << wire_value.type().name() << "`, wire_value: " << boost::any_cast<int32>(wire_value) << std::endl;
    
    assert(boost::any_cast<bool>(extension_value) == true);
    assert(boost::any_cast<int32>(wire_value) == 1);    
    found_source = true;
}

void tm(const boost::any& wire_value, const boost::any& extension_value)
{
    std::cout << "time: type: `" << wire_value.type().name() << "`, wire_value: " << boost::any_cast<std::string>(wire_value) << std::endl;
    
    assert(boost::any_cast<bool>(extension_value) == true);
    assert(boost::any_cast<std::string>(wire_value) == goby::util::as<std::string>(current_time));
    found_time = true;
}

int main()
{
    goby::acomms::protobuf::HookKey dest_key;
    dest_key.set_field_option_extension_number(queue::is_dest.number());
    goby::acomms::DCCLFieldCodecBase::register_wire_value_hook(dest_key, &dest);

    goby::acomms::protobuf::HookKey source_key;
    source_key.set_field_option_extension_number(queue::is_src.number());
    goby::acomms::DCCLFieldCodecBase::register_wire_value_hook(source_key, &source);

    goby::acomms::DCCLFieldCodecBase::register_wire_value_hook(
        make_hook_key(queue::is_time.number(), goby::acomms::protobuf::HookKey::FIELD_VALUE),
        &tm);
    
    goby::acomms::DCCLCommon::set_log(&std::cerr);

    goby::acomms::DCCLModemIdConverterCodec::add("unicorn", 3);
    goby::acomms::DCCLModemIdConverterCodec::add("topside", 1);
    
    
    goby::acomms::DCCLCodec* codec = goby::acomms::DCCLCodec::get();
    goby::acomms::protobuf::DCCLConfig cfg;
    codec->set_cfg(cfg);

    GobyMessage msg_in1, msg_out1;

    msg_in1.set_telegram("hello!");
    msg_in1.mutable_header()->set_time(
        goby::util::as<std::string>(current_time));
    msg_in1.mutable_header()->set_source_platform("topside");
    msg_in1.mutable_header()->set_dest_platform("unicorn");
    msg_in1.mutable_header()->set_dest_type(Header::PUBLISH_OTHER);

    codec->run_hooks(&msg_in1);
    assert(found_dest);
    assert(found_source);
    assert(found_time);
    
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

