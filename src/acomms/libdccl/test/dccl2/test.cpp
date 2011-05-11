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


// tests custom message codec
// tests cryptography

#include "goby/acomms/dccl.h"
#include "test.pb.h"
#include "goby/util/string.h"
#include "goby/util/time.h"
#include "goby/util/binary.h"

using goby::acomms::operator<<;
using goby::acomms::operator+;

class CustomCodec : public goby::acomms::DCCLTypedFixedFieldCodec<CustomMsg>
{
private:
    unsigned size()
        {
            return (part() == HEAD) ? 0 : A_SIZE + B_SIZE;                
        }
    Bitset encode()
        {
            return Bitset(size());
        }
    
    Bitset encode(const CustomMsg& msg)
        {
            if(part() == HEAD)
            {
                return encode();
            }
            else
            {
                Bitset a(A_SIZE, static_cast<unsigned long>(msg.a()));
                Bitset b(B_SIZE, static_cast<unsigned long>(msg.b()));
                
                std::cout << "a: " << a << std::endl;
                std::cout << "b: " << b  << std::endl;
                
                return a + b;
            }
        }
    
    
    CustomMsg decode(Bitset* bits)
        {
            if(part() == HEAD)
            {
                throw(goby::acomms::DCCLNullValueException());
            }
            else
            {
                Bitset a = *bits;
                a.resize(A_SIZE);
                Bitset b = *bits;
                b >>= A_SIZE;
                b.resize(B_SIZE);
                
                CustomMsg msg;
                msg.set_a(a.to_ulong());
                msg.set_b(b.to_ulong());
                return msg;
            }
        }
    
    
    void validate()
        {
        }

    enum { A_SIZE = 32 };
    enum { B_SIZE = 1 };
};    


int main()
{    
    goby::acomms::DCCLCommon::set_log(&std::cerr);
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
    assert(codec->validate(msg_in1.GetDescriptor()));
    std::cout << "Try encode..." << std::endl;
    std::string bytes1 = codec->encode(msg_in1);
    std::cout << "... got bytes (hex): " << goby::util::hex_encode(bytes1) << std::endl;
    std::cout << "Try decode..." << std::endl;
    boost::shared_ptr<google::protobuf::Message> msg_out1 = codec->decode(bytes1);
    std::cout << "... got Message out:\n" << msg_out1->DebugString() << std::endl;
    assert(msg_in1.SerializeAsString() == msg_out1->SerializeAsString());


    CustomMsg2 msg_in2, msg_out2;

    msg_in2.mutable_msg()->set_a(10);
    msg_in2.mutable_msg()->set_b(true);

    codec->info(msg_in2.GetDescriptor(), &std::cout);    
    std::cout << "Message in:\n" << msg_in2.DebugString() << std::endl;
    assert(codec->validate(msg_in2.GetDescriptor()));
    std::cout << "Try encode..." << std::endl;
    std::string bytes2 = codec->encode(msg_in2);
    std::cout << "... got bytes (hex): " << goby::util::hex_encode(bytes2) << std::endl;
    std::cout << "Try decode..." << std::endl;
    msg_out2 = codec->decode<CustomMsg2>(bytes2);
    std::cout << "... got Message out:\n" << msg_out2.DebugString() << std::endl;
    assert(msg_in2.SerializeAsString() == msg_out2.SerializeAsString());
    
    std::cout << "all tests passed" << std::endl;
}

