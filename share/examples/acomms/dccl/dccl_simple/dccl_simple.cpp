

// encodes/decodes a string using the DCCL codec library
// assumes prior knowledge of the message format (required fields)

#include "goby/acomms/dccl.h" // for DCCLCodec
#include "simple.pb.h" // for `Simple` protobuf message defined in simple.proto
#include "goby/util/binary.h" // for goby::util::hex_encode

using goby::acomms::operator<<;

int main()
{
    goby::acomms::DCCLCodec* dccl = goby::acomms::DCCLCodec::get();

    // validate the Simple protobuf message type as a valid DCCL message type
    dccl->validate<Simple>();
    
    // read message content (in this case from the command line)
    Simple message;
    std::cout << "input a string to send up to 10 characters: " << std::endl;
    getline(std::cin, *message.mutable_telegram());

    std::cout << "passing message to encoder:\n" << message << std::endl;

    // encode the message to a byte string
    std::string bytes;
    dccl->encode(&bytes, message);
    
    std::cout << "received hexadecimal string: " << goby::util::hex_encode(bytes) << std::endl;

    message.Clear();
    // input contents right back to decoder
    std::cout << "passed hexadecimal string to decoder: " << goby::util::hex_encode(bytes) << std::endl;

    dccl->decode(bytes, &message);
    
    std::cout << "received message:" << message << std::endl;

    
    return 0;
}

