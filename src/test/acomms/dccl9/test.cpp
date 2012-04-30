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


// tests usage of a custom DCCL ID codec

#include "goby/acomms/dccl.h"
#include "goby/acomms/dccl/dccl_field_codec.h"
#include "goby/acomms/protobuf/ccl.pb.h"
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


bool double_cmp(double a, double b, int precision)
{
    int a_whole = a;
    int b_whole = b;

    int a_part = (a-a_whole)*pow(10.0, precision);
    int b_part = (b-b_whole)*pow(10.0, precision);
    
    return (a_whole == b_whole) && (a_part == b_part);
}

goby::acomms::Bitset MicroModemMiniPacketDCCLIDCodec::encode(const goby::uint32& wire_value)
{
    // 16 bits, only 13 are useable, so
    // 3 "blank bits" + 3 bits for us
    return goby::acomms::Bitset(MINI_ID_SIZE, wire_value - MINI_ID_OFFSET);
}


int main(int argc, char* argv[])
{
    goby::glog.add_stream(goby::common::logger::DEBUG3, &std::cerr);
    goby::glog.set_name(argv[0]);    
    
    goby::acomms::DCCLCodec* codec = goby::acomms::DCCLCodec::get();
    goby::acomms::protobuf::DCCLConfig cfg;
    cfg.set_crypto_passphrase("309ldskjfla39");
    codec->set_cfg(cfg);

    
    codec->add_id_codec<MicroModemMiniPacketDCCLIDCodec>("mini_id_codec");
    codec->set_id_codec("mini_id_codec");    

    codec->validate<MiniUser>();
    codec->info<MiniUser>(&goby::glog);    

    MiniUser mini_user_msg_in, mini_user_msg_out;
    mini_user_msg_in.set_user(876);
    std::string encoded;
    codec->encode(&encoded, mini_user_msg_in);
    codec->decode(encoded, &mini_user_msg_out);
    assert(mini_user_msg_out.SerializeAsString() == mini_user_msg_in.SerializeAsString());

    codec->validate<MiniOWTT>();
    codec->info<MiniOWTT>(&goby::glog);

    MiniOWTT mini_owtt_in, mini_owtt_out; 
    mini_owtt_in.set_clock_mode(3);
    mini_owtt_in.set_tod(12);
    mini_owtt_in.set_user(13);

    codec->encode(&encoded, mini_owtt_in);
    std::cout << "OWTT as hex: " << goby::util::hex_encode(encoded) << std::endl;
    
    codec->decode(encoded, &mini_owtt_out);
    assert(mini_owtt_out.SerializeAsString() == mini_owtt_in.SerializeAsString());
    
    codec->validate<MiniAbort>();
    codec->info<MiniAbort>(&goby::glog);

    MiniAbort mini_abort_in, mini_abort_out; 
    mini_abort_in.set_user(130);
    codec->encode(&encoded, mini_abort_in);
    codec->decode(encoded, &mini_abort_out);
    assert(mini_abort_out.SerializeAsString() == mini_abort_in.SerializeAsString());

    cfg.set_ccl_compatible(true);
    cfg.clear_crypto_passphrase();
    codec->set_cfg(cfg);

    codec->validate<NormalDCCL>();
    codec->info<NormalDCCL>(&goby::glog);
    NormalDCCL normal_msg, normal_msg_out;
    normal_msg.set_a(123);
    normal_msg.set_b(321);

    codec->encode(&encoded, normal_msg);
    std::cout << goby::util::hex_encode(encoded) << std::endl;
    assert(goby::util::hex_encode(encoded).substr(0, 2) == "20");
    codec->decode(encoded, &normal_msg_out);
    
    assert(normal_msg.SerializeAsString() == normal_msg_out.SerializeAsString());

    codec->validate<goby::acomms::protobuf::CCLMDATState>();
    codec->info<goby::acomms::protobuf::CCLMDATState>(&goby::glog);

    goby::acomms::protobuf::CCLMDATState state_in, state_out;
    codec->decode(goby::util::hex_decode("0e86fa11ad20c9011b4432bf47d10000002401042f0e7d87fa111620c95a200a"), &state_out);
    state_in.set_latitude(25.282416667);
    state_in.set_longitude(-77.164266667);
    state_in.set_fix_age(4);
    
    boost::posix_time::ptime time_date(
        boost::gregorian::date(boost::gregorian::day_clock::universal_day().year(),
                               boost::date_time::Mar, 04), 
        boost::posix_time::time_duration(17,1,44));

    state_in.set_time_date(goby::util::as<goby::uint64>(time_date));
    state_in.set_heading(270);
    state_in.set_depth(2323);
    state_in.set_mission_mode(goby::acomms::protobuf::CCLMDATState::NORMAL);
    
    std::cout << "in:" << state_in << std::endl;
    std::cout << "out:" << state_out << std::endl;

    assert(double_cmp(state_in.latitude(), state_out.latitude(), 4));
    assert(double_cmp(state_in.longitude(), state_out.longitude(), 4));
    assert(state_in.fix_age() == state_out.fix_age());
    assert(state_in.time_date() == state_out.time_date());
    assert(goby::util::unbiased_round(state_in.heading(),0) ==
           goby::util::unbiased_round(state_out.heading(),0));
    assert(double_cmp(state_in.depth(), state_out.depth(), 1));
    assert(state_in.mission_mode() == state_out.mission_mode());
    
    std::cout << goby::util::hex_encode(state_out.faults()) << std::endl;
    std::cout << goby::util::hex_encode(state_out.faults_2()) << std::endl;
                                        
    
    std::cout << "all tests passed" << std::endl;
}

