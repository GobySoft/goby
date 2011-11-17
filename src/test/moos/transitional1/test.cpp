// t. schneider tes@mit.edu 11.20.09
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


#include "goby/acomms/dccl/dccl.h"
#include "goby/moos/transitional/dccl_transitional.h"

#include <iostream>
#include <cassert>

using namespace goby;
using goby::acomms::operator<<;
using goby::transitional::operator<<;

void plus1(transitional::DCCLMessageVal& mv)
{
    long l = mv;
    ++l;
    mv = l;
}

void times2(transitional::DCCLMessageVal& mv)
{
    double d = mv;
    d *= 2;
    mv = d;
}

void prepend_fat(transitional::DCCLMessageVal& mv)
{
    std::string s = mv;
    s = "fat_" + s;
    mv = s;
}

void invert(transitional::DCCLMessageVal& mv)
{
    bool b = mv;
    b ^= 1;
    mv = b;
}

void algsum(transitional::DCCLMessageVal& mv, const std::vector<transitional::DCCLMessageVal>& ref_vals)
{
    double d = 0;
    // index 0 is the name ("sum"), so start at 1
    for(size_t i = 0, n = ref_vals.size(); i < n; ++i)
    {
        d += double(ref_vals[i]);
    }
    mv = d;
}

int main(int argc, char* argv[])
{
    goby::glog.add_stream(goby::util::logger::DEBUG3, &std::cerr);
    goby::glog.set_name(argv[0]);
    
    std::cout << "loading xml file: " << DCCL_EXAMPLES_DIR "transitional1/test.xml" << std::endl;

    // instantiate the parser with a single xml file
    goby::transitional::DCCLTransitionalCodec transitional_dccl(&std::cerr);    
    goby::transitional::protobuf::DCCLTransitionalConfig cfg;
    cfg.add_message_file()->set_path(DCCL_EXAMPLES_DIR "/transitional1/test.xml");
    transitional_dccl.set_cfg(cfg);

    std::cout << transitional_dccl << std::endl;

    goby::acomms::DCCLCodec* dccl = goby::acomms::DCCLCodec::get();

    const google::protobuf::Descriptor* desc = transitional_dccl.descriptor(4);
    
    // const google::protobuf::Descriptor* desc =
    //     transitional_dccl.convert_to_protobuf_descriptor(4, "/tmp/foo.proto");

    assert(desc != 0);

    std::cout << desc->DebugString() << std::endl;

    dccl->validate(desc);

    
    // load up the algorithms    
    transitional_dccl.add_algorithm("prepend_fat", &prepend_fat);
    transitional_dccl.add_algorithm("+1", &plus1);
    transitional_dccl.add_algorithm("*2", &times2);
    transitional_dccl.add_algorithm("invert", &invert);
    transitional_dccl.add_adv_algorithm("sum", &algsum);

    
    std::map<std::string, std::vector<transitional::DCCLMessageVal> > in;
    
    bool b = true; 
    std::vector<transitional::DCCLMessageVal> e;
    e.push_back("dog");
    e.push_back("cat");
    e.push_back("emu");
    
    std::string s = "raccoon";  
    std::vector<transitional::DCCLMessageVal> i;
    i.push_back(30);
    i.push_back(40);
    std::vector<transitional::DCCLMessageVal> f;
    f.push_back(-12.5);
    f.push_back(1);
    
    std::string h = goby::util::hex_decode("abcd1234"); 
    std::vector<transitional::DCCLMessageVal> sum(2,0);
    
    
    in["B"] = std::vector<transitional::DCCLMessageVal>(1,b);
    in["E"] = e;
    in["S"] = std::vector<transitional::DCCLMessageVal>(1,s);
    in["I"] = i;
    in["F"] = f;
    in["H"] = std::vector<transitional::DCCLMessageVal>(1,h);
    in["SUM"] = sum;

    std::string bytes;
    std::cout << "sent values:" << std::endl 
              << in;

    boost::shared_ptr<google::protobuf::Message> proto_msg;
    transitional_dccl.encode(4, proto_msg, in);
    dccl->encode(&bytes, *proto_msg);

    
    std::cout << "hex out: " << goby::util::hex_encode(bytes) << std::endl;
    bytes.resize(bytes.length() + 20, '0');
    std::cout << "hex in: " << goby::util::hex_encode(bytes) << std::endl;    
    
    std::map<std::string, std::vector<transitional::DCCLMessageVal> > out;

    
    dccl->decode(bytes, proto_msg.get());
    transitional_dccl.decode(*proto_msg, out);
    
    std::cout << "received values:" << std::endl 
              << out;    

    sum[0] = double(i[0]) + double(f[0]);
    sum[1] = double(i[1]) + double(f[1]);
    i[0] = int(i[0]) + 1;
    i[1] = int(i[1]) + 1;
    
    transitional::DCCLMessageVal tmp = b;
    invert(tmp);
    b = tmp;
    
    tmp = s;
    prepend_fat(tmp);
    tmp.get(s);

    tmp = f[0];
    times2(tmp);
    f[0] = tmp;

    tmp = f[1];
    times2(tmp);
    f[1] = tmp;

    assert(out["B"][0] == b);
    assert(out["E"][0] == e[0]);
    assert(out["E"][1] == e[1]);
    assert(out["E"][2] == e[2]);
    assert(out["S"][0] == s);
    assert(out["F"][0] == f[0]);
    assert(out["F"][1] == f[1]);
    assert(out["SUM"][0] == sum[0]);
    assert(out["SUM"][1] == sum[1]);
    assert(out["I"][0] == i[0]);
    assert(out["I"][1] == i[1]);
    assert(out["H"][0] == h);
    
    std::cout << "all tests passed" << std::endl;
}
