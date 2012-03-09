

// tests usage of a custom DCCL ID codec

#include "goby/acomms/dccl.h"
#include "goby/acomms/dccl/dccl_field_codec.h"
#include "test.pb.h"

using goby::acomms::operator<<;

class MicroModemMiniPacketDCCLIDCodec : public goby::acomms::DCCLTypedFixedFieldCodec<goby::uint32>
{
private:
    goby::acomms::Bitset encode(const goby::uint32& wire_value);
    
    goby::acomms::Bitset encode()
        { return encode(MINI_ID_OFFSET); }
    
    goby::uint32 decode(const goby::acomms::Bitset& bits)
        { return bits.to_ulong() + MINI_ID_OFFSET; }
    
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
    
    std::cout << "all tests passed" << std::endl;
}

