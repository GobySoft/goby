// Copyright 2009-2013 Toby Schneider (https://launchpad.net/~tes)
//                     Massachusetts Institute of Technology (2007-)
//                     Woods Hole Oceanographic Institution (2007-)
//                     Goby Developers Team (https://launchpad.net/~goby-dev)
// 
//
// This file is part of the Goby Underwater Autonomy Project Binaries
// ("The Goby Binaries").
//
// The Goby Binaries are free software: you can redistribute them and/or modify
// them under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// The Goby Binaries are distributed in the hope that they will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Goby.  If not, see <http://www.gnu.org/licenses/>.


// tests callback hooks on (using wire values generated at pre-encode)

#include "goby/acomms/dccl.h"
#include "goby/acomms/dccl/dccl_field_codec_default.h"
#include "goby/acomms/dccl/dccl_field_codec.h"
#include "test.pb.h"
#include "goby/util/as.h"
#include "goby/common/time.h"
#include "goby/util/binary.h"

using goby::acomms::operator<<;
using goby::uint32;

bool found_dest = false;
bool found_source = false;
bool found_time = false;

boost::posix_time::ptime current_time = boost::posix_time::second_clock::universal_time();

const std::string time_field = "Header.time";
const std::string src_field = "Header.source_platform";
const std::string dest_field = "Header.dest_platform";

void process_queue_field(const google::protobuf::FieldDescriptor* field,
                         const boost::any& field_value,
                         const boost::any& wire_value)
{
    std::cout << "Field: " << field->DebugString() << std::flush;
    
    if(field->full_name() == dest_field)
    {
        std::cout << "dest: type: `" << wire_value.type().name() << "`, wire_value: " << boost::any_cast<uint32>(wire_value) << std::endl;
        assert(boost::any_cast<uint32>(wire_value) == 3);    
        found_dest = true;
    }
    else if(field->full_name() == src_field)
    {
        std::cout << "source: type: `" << wire_value.type().name() << "`, wire_value: " << boost::any_cast<uint32>(wire_value) << std::endl;
        
        assert(boost::any_cast<uint32>(wire_value) == 1);    
        found_source = true;
    }
    else if(field->full_name() == time_field)
    {
        std::cout << "time: type: `" << field_value.type().name() << "`, field_value: " << boost::any_cast<std::string>(field_value) << std::endl;        
        assert(boost::any_cast<std::string>(field_value) == goby::util::as<std::string>(current_time));
        found_time = true;
    }
}

int main(int argc, char* argv[])
{
    goby::glog.add_stream(goby::common::logger::DEBUG3, &std::cerr);
    goby::glog.set_name(argv[0]);

    goby::acomms::DCCLCodec* codec = goby::acomms::DCCLCodec::get();

    goby::acomms::DCCLFieldCodecBase::register_wire_value_hook(codec->id<GobyMessage>(),
                                                               &process_queue_field);    
    
    goby::acomms::protobuf::DCCLConfig cfg;
    codec->set_cfg(cfg);

    GobyMessage msg_in1, msg_out1;

    msg_in1.set_telegram("hello!");
    msg_in1.mutable_header()->set_time(
        goby::util::as<std::string>(current_time));
    msg_in1.mutable_header()->set_source_platform(1);
    msg_in1.mutable_header()->set_dest_platform(3);
    msg_in1.mutable_header()->set_dest_type(Header::PUBLISH_OTHER);

    codec->run_hooks(msg_in1);
    assert(found_dest);
    assert(found_source);
    assert(found_time);
    
    codec->info(msg_in1.GetDescriptor(), &std::cout);    
    std::cout << "Message in:\n" << msg_in1.DebugString() << std::endl;
    codec->validate(msg_in1.GetDescriptor());

    // try callbacks
    
    std::cout << "Try encode..." << std::endl;
    std::string bytes1;
    codec->encode(&bytes1, msg_in1);
    std::cout << "... got bytes (hex): " << goby::util::hex_encode(bytes1) << std::endl;
    std::cout << "Try decode..." << std::endl;
    codec->decode(bytes1, &msg_out1);
    std::cout << "... got Message out:\n" << msg_out1.DebugString() << std::endl;
    assert(msg_in1.SerializeAsString() == msg_out1.SerializeAsString());

    
    std::cout << "all tests passed" << std::endl;
}

