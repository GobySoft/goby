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


// tests fixed id header

#include "goby/acomms/dccl.h"
#include "test.pb.h"

using goby::acomms::operator<<;

int main(int argc, char* argv[])
{
    goby::glog.add_stream(goby::util::Logger::DEBUG3, &std::cerr);
    goby::glog.set_name(argv[0]);
    
    goby::acomms::DCCLCodec* codec = goby::acomms::DCCLCodec::get();

    ShortIDMsg short_id_msg;
    codec->validate(short_id_msg.GetDescriptor());
    codec->info(short_id_msg.GetDescriptor(), &goby::glog);
    codec->decode(codec->encode(short_id_msg));
    assert(codec->size(short_id_msg) == 1);

    LongIDMsg long_id_msg;
    codec->validate(long_id_msg.GetDescriptor());
    codec->info(long_id_msg.GetDescriptor(), &goby::glog);
    codec->decode(codec->encode(long_id_msg));
    assert(codec->size(long_id_msg) == 2);
    

    ShortIDEdgeMsg short_id_edge_msg;
    codec->validate(short_id_edge_msg.GetDescriptor());
    codec->info(short_id_edge_msg.GetDescriptor(), &goby::glog);
    codec->decode(codec->encode(short_id_edge_msg));
    assert(codec->size(short_id_edge_msg) == 1);

    LongIDEdgeMsg long_id_edge_msg;
    codec->validate(long_id_edge_msg.GetDescriptor());
    codec->info(long_id_edge_msg.GetDescriptor(), &goby::glog);
    codec->decode(codec->encode(long_id_edge_msg));
    assert(codec->size(long_id_edge_msg) == 2);

    TooLongIDMsg too_long_id_msg;
    // should fail validation
    try
    {
        codec->validate(too_long_id_msg.GetDescriptor());
        assert(false);
    }
    catch(goby::acomms::DCCLException& e)
    { }
    

    ShortIDMsgWithData short_id_msg_with_data;
    codec->validate(short_id_msg_with_data.GetDescriptor());
    codec->info(short_id_msg_with_data.GetDescriptor(), &goby::glog);

    short_id_msg_with_data.set_in_head(42);
    short_id_msg_with_data.set_in_body(37);
    codec->decode(codec->encode(short_id_msg_with_data));

    
    std::cout << "all tests passed" << std::endl;
}

