#include "goby/acomms/modem_driver.h"
#include "goby/acomms/connect.h"
#include "perls_modem_mini.pb.h"
#include "goby/util/binary.h" // for hex encode / decode

const int PERLS_MODEM_ID = 8;
const std::string PERLS_SERIAL_PORT = "/dev/ttyS0";

void handle_range_reply(const goby::acomms::protobuf::ModemRangingReply& message);
//void handle_data_request(const goby::acomms::protobuf::ModemDataRequest& msg_request, goby::acomms::protobuf::ModemDataTransmission* msg_data);
void handle_receive (const goby::acomms::protobuf::ModemDataTransmission& message);

void initiate_mini_transmission(perls::protobuf::ModemMiniTransmission& mini_msg);
void decode_mini_packet(const goby::acomms::protobuf::ModemDataTransmission data_msg,
                        perls::protobuf::ModemMiniTransmission* mini_msg);



goby::acomms::MMDriver mm_driver(&std::clog);

int main(int argc, char* argv[])
{
    goby::acomms::connect(&mm_driver.signal_receive, &handle_receive);
    goby::acomms::connect(&mm_driver.signal_range_reply, &handle_range_reply);
    
    goby::acomms::protobuf::DriverConfig cfg;
    cfg.set_modem_id(PERLS_MODEM_ID);
    cfg.set_serial_port(PERLS_SERIAL_PORT);
    mm_driver.startup(cfg);
    
    return 0;
}


//
// Goby ModemDriver Handlers
//
void handle_range_reply(const goby::acomms::protobuf::ModemRangingReply& message)
{
    std::cout << "Got ranging msg: " << message.DebugString() << std::endl;
}

// void handle_data_request(const goby::acomms::protobuf::ModemDataRequest& msg_request, goby::acomms::protobuf::ModemDataTransmission* msg_data)
// {

// }

void handle_receive (const goby::acomms::protobuf::ModemDataTransmission& message)
{
    switch(message.GetExtension(MicroModem::packet_type))
    {
        case MicroModem::PACKET_MINI:
        {
            perls::protobuf::ModemMiniTransmission mini_msg;
            decode_mini_packet(message, &mini_msg);
            std::cout << "Got mini msg: " << mini_msg.DebugString() << std::endl;
        }
        break;

        case MicroModem::PACKET_DATA:
        {
            std::cout << "Got data msg: " << message.DebugString() << std::endl;
        }
        break;
    }
}


//
// PERLS Specific Mini Packet Functions
//
void initiate_mini_transmission(perls::protobuf::ModemMiniTransmission& mini_msg)
{
    /*********************************************
     mini packet has range 0x0000 - 0x1FFF (13 bits)
     we define the packet header (3bits) as:
        | msg_type (3bits) |

     so that a typical packet is of the form
        | header (3bits) | user (10bits) |

     furthermore, if msg_type = MINI_OWTT we steal 6 more bits for TOD;
        | header (3bits) | clock mode (2bits) | tod (4bits) | user (4bits) |
    *********************************************/
 
    // add message header bits 
    if (!mini_msg.has_clk_mode())
        mini_msg.set_clk_mode((goby::acomms::protobuf::ClockMode) mm_driver.clk_mode());

    int type = mini_msg.type();
    int clk_mode = mini_msg.clk_mode();

    // get top of second for owtt packet -- we add one because the packet will
    // not be sent until the next second (section 2.3, Sync Navigation with
    // MM, Revision D)
    boost::posix_time::ptime p = goby::util::goby_time();
    int tod = int(p.time_of_day().seconds()+1) % 10;

    // pack mini packet with header and user information
    int packet = 0, user = 0;
    switch (type) 
    {
        case perls::protobuf::MINI_OWTT:
        {
            // make sure user data is only 4bits
            user = mini_msg.data() & 0x0F;
            packet = (type<<10) + (clk_mode<<8) + (tod<<4) + user;
            break;
        }
        case perls::protobuf::MINI_ABORT:
        case perls::protobuf::MINI_USER:
        default:
        {
            // make sure user data is only 10bits
            user   = mini_msg.data() & 0x3FF;
            packet = (type<<10) + user;
        }
    }

    char packet_bytes[16];
    snprintf (packet_bytes, sizeof packet_bytes, "%04x", packet);

    goby::acomms::protobuf::ModemDataInit init_msg;
    init_msg.MutableExtension(MicroModem::init_slot)->set_type(goby::acomms::protobuf::SLOT_MINI);
    
    goby::acomms::protobuf::ModemDataTransmission* frame = init_msg.add_frame();
    frame->mutable_base()->CopyFrom(mini_msg);
    frame->set_data(goby::util::hex_decode(packet_bytes));
    
    mm_driver.handle_initiate_transmission(&init_msg);
}



void decode_mini_packet(const goby::acomms::protobuf::ModemDataTransmission data_msg,
                        perls::protobuf::ModemMiniTransmission* mini_msg)
{
    /*********************************************
     mini packet has range 0x0000 - 0x1FFF (13 bits)
     we define the packet header (3bits) as:
        | msg_type (3bits) |

     so that a typical packet is of the form
        | header (3bits) | user (10bits) |

     furthermore, if msg_type = MINI_OWTT we steal 6 more bits for TOD;
        | header (3bits) | clock mode (2bits) | tod (4bits) | user (4bits) |
    *********************************************/
    mini_msg->mutable_base()->CopyFrom(data_msg.base());

    // get user and header bits
    int data;
    std::string hex_data = goby::util::hex_encode(data_msg.data());
    sscanf ((char*) hex_data.c_str(), "%04x", &data);
    int decode_user   = data & 0x3FF;
    int decode_type   = data>>10; 

    if (perls::protobuf::MiniType_IsValid (decode_type))
        mini_msg->set_type((perls::protobuf::MiniType) decode_type);

    // decode user data -- need to do something with time of launch
    int user = 0;
    switch (decode_type) 
    {
        case perls::protobuf::MINI_OWTT:
        {
            user = decode_user & 0x0F;
            mini_msg->set_time_of_depart((decode_user>>4) & 0x0F);
            int decode_clk_mode = (decode_user>>8) & 0x03; 
            if (goby::acomms::protobuf::ClockMode_IsValid (decode_clk_mode))
                mini_msg->set_clk_mode((goby::acomms::protobuf::ClockMode) decode_clk_mode);
            break;
        }
        case perls::protobuf::MINI_ABORT:
        case perls::protobuf::MINI_USER:
        default:
        {
            user = decode_user;
        }
    }
    mini_msg->set_data(user);
}
