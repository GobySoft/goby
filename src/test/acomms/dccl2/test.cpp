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


// tests custom message codec
// tests cryptography

#include "goby/acomms/dccl.h"
#include "test.pb.h"
#include "goby/util/as.h"
#include "goby/common/time.h"
#include "goby/util/binary.h"

using goby::acomms::operator<<;
using goby::acomms::operator+;
using goby::acomms::Bitset;

class CustomCodec : public goby::acomms::DCCLTypedFixedFieldCodec<CustomMsg>
{
private:
    unsigned size() { return (part() == HEAD) ? 0 : A_SIZE + B_SIZE; }
    Bitset encode() { return Bitset(size()); }
    
    Bitset encode(const CustomMsg& msg)
        {
            if(part() == HEAD)
            { return encode(); }
            else
            {
                Bitset a(A_SIZE, static_cast<unsigned long>(msg.a()));
                Bitset b(B_SIZE, static_cast<unsigned long>(msg.b()));
                
                std::cout << "a: " << a << std::endl;
                std::cout << "b: " << b  << std::endl;
                
                return a + b;
            }
        }    
    
    CustomMsg decode(const Bitset& bits)
        {
            if(part() == HEAD)
            { throw(goby::acomms::DCCLNullValueException()); }
            else
            {
                Bitset a = bits;
                a.resize(A_SIZE);
                Bitset b = bits;
                b >>= A_SIZE;
                b.resize(B_SIZE);
                
                CustomMsg msg;
                msg.set_a(a.to_ulong());
                msg.set_b(b.to_ulong());
                return msg;
            }
        }
    
    
    void validate() { }

    enum { A_SIZE = 32 };
    enum { B_SIZE = 1 };
};    

int main(int argc, char* argv[])
{
    goby::glog.add_stream(goby::common::logger::DEBUG3, &std::cerr);
    goby::glog.set_name(argv[0]);

    goby::acomms::DCCLCodec* codec = goby::acomms::DCCLCodec::get();
    goby::acomms::DCCLFieldCodecManager::add<CustomCodec>("custom_codec");

    goby::acomms::protobuf::DCCLConfig cfg;
    cfg.set_crypto_passphrase("my_passphrase!");
    codec->set_cfg(cfg);

    CustomMsg msg_in1;

    msg_in1.set_a(10);
    msg_in1.set_b(true);
    codec->info(msg_in1.GetDescriptor(), &std::cout);    
    std::cout << "Message in:\n" << msg_in1.DebugString() << std::endl;
    codec->validate(msg_in1.GetDescriptor());
    std::cout << "Try encode..." << std::endl;
    std::string bytes1;
    codec->encode(&bytes1, msg_in1);
    std::cout << "... got bytes (hex): " << goby::util::hex_encode(bytes1) << std::endl;
    std::cout << "Try decode..." << std::endl;
    boost::shared_ptr<google::protobuf::Message> msg_out1 = codec->decode<boost::shared_ptr<google::protobuf::Message> >(bytes1);
    std::cout << "... got Message out:\n" << msg_out1->DebugString() << std::endl;
    assert(msg_in1.SerializeAsString() == msg_out1->SerializeAsString());


    CustomMsg2 msg_in2, msg_out2;

    msg_in2.mutable_msg()->set_a(10);
    msg_in2.mutable_msg()->set_b(true);

    codec->info(msg_in2.GetDescriptor(), &std::cout);    
    std::cout << "Message in:\n" << msg_in2.DebugString() << std::endl;
    codec->validate(msg_in2.GetDescriptor());
    std::cout << "Try encode..." << std::endl;
    std::string bytes2;
    codec->encode(&bytes2, msg_in2);
    std::cout << "... got bytes (hex): " << goby::util::hex_encode(bytes2) << std::endl;
    std::cout << "Try decode..." << std::endl;
    codec->decode(bytes2, &msg_out2);
    std::cout << "... got Message out:\n" << msg_out2.DebugString() << std::endl;
    assert(msg_in2.SerializeAsString() == msg_out2.SerializeAsString());
    
    std::cout << "all tests passed" << std::endl;
}

