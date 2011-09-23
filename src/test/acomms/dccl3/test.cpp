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


// tests proper encoding of standard Goby header

#include "goby/acomms/dccl.h"
#include "goby/acomms/dccl/dccl_field_codec_default.h"
#include "test.pb.h"
#include "goby/util/as.h"
#include "goby/util/time.h"
#include "goby/util/binary.h"

using goby::acomms::operator<<;



int main(int argc, char* argv[])
{
    goby::glog.add_stream(goby::util::logger::DEBUG3, &std::cerr);
    goby::glog.set_name(argv[0]);
    
    goby::acomms::DCCLModemIdConverterCodec::add("unicorn", 3);
    goby::acomms::DCCLModemIdConverterCodec::add("topside", 1);
    
    
    goby::acomms::DCCLCodec* codec = goby::acomms::DCCLCodec::get();
    goby::acomms::protobuf::DCCLConfig cfg;
//    cfg.set_crypto_passphrase("hmmm");
    codec->set_cfg(cfg);

    GobyMessage msg_in1;

    msg_in1.set_telegram("hello!");
    msg_in1.mutable_header()->set_time(
        goby::util::as<std::string>(boost::posix_time::second_clock::universal_time()));
    msg_in1.mutable_header()->set_source_platform("topside");
    msg_in1.mutable_header()->set_dest_platform("unicorn");
    msg_in1.mutable_header()->set_dest_type(Header::PUBLISH_OTHER);
    
    codec->info(msg_in1.GetDescriptor(), &std::cout);    
    std::cout << "Message in:\n" << msg_in1.DebugString() << std::endl;
    codec->validate(msg_in1.GetDescriptor());
    std::cout << "Try encode..." << std::endl;
    std::string bytes1;
    codec->encode(&bytes1, msg_in1);
    std::cout << "... got bytes (hex): " << goby::util::hex_encode(bytes1) << std::endl;

    // test that adding garbage to the end does not affect decoding
    bytes1 += std::string(10, '\0');
    
    std::cout << "Try decode..." << std::endl;
    
    GobyMessage* msg_out1 = codec->decode<GobyMessage*>(bytes1);
    std::cout << "... got Message out:\n" << msg_out1->DebugString() << std::endl;
    assert(msg_in1.SerializeAsString() == msg_out1->SerializeAsString());
    delete msg_out1;
    
    std::cout << "all tests passed" << std::endl;
}

