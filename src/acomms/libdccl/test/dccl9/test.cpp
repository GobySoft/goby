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


// tests usage of a custom DCCL ID codec

#include "goby/acomms/dccl.h"
#include "goby/acomms/libdccl/dccl_field_codec.h"
#include "test.pb.h"

using goby::acomms::operator<<;

class MicroModemMiniPacketDCCLIDCodec : public goby::acomms::DCCLTypedFixedFieldCodec<goby::uint32>
{
private:
    goby::acomms::Bitset encode(const goby::uint32& wire_value);
    
    goby::acomms::Bitset encode()
        { return encode(MINI_ID_OFFSET); }
    
    goby::uint32 decode(goby::acomms::Bitset* bits)
        { return bits->to_ulong() + MINI_ID_OFFSET; }
    
    unsigned size()
        { return MINI_ID_SIZE; }
    
    void validate()
        { }
    

    // Add this value when decoding to put us safely in our own namespace
    // from the normal default DCCL Codec
    enum { MINI_ID_OFFSET = 1000000 };    
    enum { MINI_ID_SIZE = 6 };
};


goby::acomms::Bitset MicroModemMiniPacketDCCLIDCodec::encode(const goby::uint32& wire_value)
{
    // 16 bits, only 13 are useable, so
    // 3 "blank bits" + 3 bits for us
    return goby::acomms::Bitset(MINI_ID_SIZE, wire_value - MINI_ID_OFFSET);
}


int main(int argc, char* argv[])
{
    goby::glog.add_stream(goby::util::Logger::DEBUG3, &std::cerr);
    goby::glog.set_name(argv[0]);    
    
    goby::acomms::DCCLCodec* codec = goby::acomms::DCCLCodec::get();

    codec->add_id_codec<MicroModemMiniPacketDCCLIDCodec>("mini_id_codec");
    codec->set_id_codec("mini_id_codec");    

    codec->validate<MiniUser>();
    codec->info<MiniUser>(&goby::glog);    

    MiniUser mini_user_msg;
    mini_user_msg.set_user(876);
    assert(codec->decode(codec->encode(mini_user_msg))->SerializeAsString() == mini_user_msg.SerializeAsString());

    codec->validate<MiniOWTT>();
    codec->info<MiniOWTT>(&goby::glog);

    MiniOWTT mini_owtt; 
    mini_owtt.set_clock_mode(3);
    mini_owtt.set_tod(12);
    mini_owtt.set_user(13);
    std::string encoded = codec->encode(mini_owtt);
    std::cout << "OWTT as hex: " << goby::util::hex_encode(encoded) << std::endl;
    
    assert(codec->decode(encoded)->SerializeAsString() == mini_owtt.SerializeAsString());
    
    codec->validate<MiniAbort>();
    codec->info<MiniAbort>(&goby::glog);

    MiniAbort mini_abort; 
    mini_abort.set_user(130);
    assert(codec->decode(codec->encode(mini_abort))->SerializeAsString() == mini_abort.SerializeAsString());
    
    std::cout << "all tests passed" << std::endl;
}

