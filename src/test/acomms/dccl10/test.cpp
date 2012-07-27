// Copyright 2009-2012 Toby Schneider (https://launchpad.net/~tes)
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


// tests arithmetic encoder

#include <google/protobuf/descriptor.pb.h>

#include "goby/acomms/dccl.h"
#include "goby/acomms/dccl/dccl_field_codec_arithmetic.h"

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

    goby::acomms::protobuf::ArithmeticModel float_model;

    float_model.add_value_bound(100.0);
    float_model.add_frequency(100);
    
    float_model.add_value_bound(100.1);
    float_model.add_frequency(100);
    
    float_model.add_value_bound(100.2);
    float_model.add_frequency(100);
    
    float_model.add_value_bound(100.3);
    float_model.add_frequency(100);
    
    float_model.add_value_bound(100.4);
    float_model.add_frequency(90);
    
    float_model.add_value_bound(100.5);
    float_model.add_frequency(125);
    
    float_model.add_value_bound(100.6);
    float_model.add_frequency(125);
    
    float_model.add_value_bound(100.7);
    float_model.add_frequency(125);

    float_model.add_value_bound(100.8);

    float_model.set_eof_frequency(25);
    float_model.set_out_of_range_frequency(10);

    
    goby::acomms::DCCLArithmeticFieldCodec<double>::set_model("float_model", float_model);
    
    


    ArithmeticTestMsg msg_in;

    msg_in.add_double_arithmetic_repeat(100.5);
    msg_in.add_double_arithmetic_repeat(100.5);
    msg_in.add_double_arithmetic_repeat(100.6);
    msg_in.add_double_arithmetic_repeat(100.7);    
    
    codec->info(msg_in.GetDescriptor(), &std::cout);

    codec->validate(msg_in.GetDescriptor());
    
    std::cout << "Message in:\n" << msg_in.DebugString() << std::endl;

    
    std::cout << "Try encode..." << std::endl;
    std::string bytes;
    codec->encode(&bytes, msg_in);
    std::cout << "... got bytes (hex): " << goby::util::hex_encode(bytes) << std::endl;

    std::cout << "Try decode..." << std::endl;

    ArithmeticTestMsg msg_out;
    codec->decode(bytes, &msg_out);
    
    std::cout << "... got Message out:\n" << msg_out.DebugString() << std::endl;

    
    assert(msg_in.SerializeAsString() == msg_out.SerializeAsString());
    
    std::cout << "all tests passed" << std::endl;
}

