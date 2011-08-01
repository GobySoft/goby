#include "goby/acomms/modem_driver.h"
#include "goby/protobuf/perls_modem_mini.pb.h"

int main(int argc, char* argv[])
{
    return 0;
}


// void handle_initiate_mini_transmission(ModemMiniTransmission* mini_msg)
// {
//     /*********************************************
//      mini packet has range 0x0000 - 0x1FFF (13 bits)
//      we define the packet header (3bits) as:
//         | msg_type (3bits) |

//      so that a typical packet is of the form
//         | header (3bits) | user (10bits) |

//      furthermore, if msg_type = MINI_OWTT we steal 6 more bits for TOD;
//         | header (3bits) | clock mode (2bits) | tod (4bits) | user (4bits) |
//     *********************************************/
 
//     // add message header bits 
//     if (!mini_msg->has_clk_mode())
//         mini_msg->set_clk_mode((protobuf::ClockMode) clk_mode_);

//     int type = mini_msg->type();
//     int clk_mode = mini_msg->clk_mode();

//     // get top of second for owtt packet -- we add one because the packet will
//     // not be sent until the next second (section 2.3, Sync Navigation with
//     // MM, Revision D)
//     boost::posix_time::ptime p = goby_time();
//     int tod = int(p.time_of_day().seconds()+1) % 10;

//     // pack mini packet with header and user information
//     int packet = 0, user = 0;
//     switch (type) 
//     {
//         case protobuf::MINI_OWTT:
//         {
//             // make sure user data is only 4bits
//             user = mini_msg->data() & 0x0F;
//             packet = (type<<10) + (clk_mode<<8) + (tod<<4) + user;
//             break;
//         }
//         case protobuf::MINI_ABORT:
//         case protobuf::MINI_USER:
//         default:
//         {
//             // make sure user data is only 10bits
//             user   = mini_msg->data() & 0x3FF;
//             packet = (type<<10) + user;
//         }
//     }

//     char packet_bytes[16];
//     snprintf (packet_bytes, sizeof packet_bytes, "%04x", packet);

//     //$CCMUC,SRC,DEST,HHHH*CS
//     NMEASentence nmea("$CCMUC", NMEASentence::IGNORE);
//     nmea.push_back(mini_msg->base().src()); // ADR1
//     nmea.push_back(mini_msg->base().dest()); // ADR2
//     nmea.push_back(packet_bytes); //HHHH    

//     append_to_write_queue(nmea, mini_msg->mutable_base());
// }



// void goby::acomms::MMDriver::mua(const NMEASentence& nmea, protobuf::ModemMiniTransmission* m)
// {
//     /*********************************************
//      mini packet has range 0x0000 - 0x1FFF (13 bits)
//      we define the packet header (3bits) as:
//         | msg_type (3bits) |

//      so that a typical packet is of the form
//         | header (3bits) | user (10bits) |

//      furthermore, if msg_type = MINI_OWTT we steal 6 more bits for TOD;
//         | header (3bits) | clock mode (2bits) | tod (4bits) | user (4bits) |
//     *********************************************/
 
//     m->mutable_base()->set_time(as<std::string>(goby_time()));
//     m->mutable_base()->set_src(as<uint32>(nmea[1]));
//     m->mutable_base()->set_dest(as<uint32>(nmea[2]));

//     // get user and header bits
//     int data; 
//     sscanf ((char*) nmea[3].c_str(), "%04x", &data);
//     int decode_user   = data & 0x3FF;
//     int decode_type   = data>>10; 

//     if (protobuf::MiniType_IsValid (decode_type))
//         m->set_type((protobuf::MiniType) decode_type);

//     // decode user data -- need to do something with time of launch
//     int user = 0;
//     switch (decode_type) 
//     {
//         case protobuf::MINI_OWTT:
//         {
//             user = decode_user & 0x0F;
//             m->set_time_of_depart((decode_user>>4) & 0x0F);
//             int decode_clk_mode = (decode_user>>8) & 0x03; 
//             if (protobuf::ClockMode_IsValid (decode_clk_mode))
//                 m->set_clk_mode((protobuf::ClockMode) decode_clk_mode);
//             break;
//         }
//         case protobuf::MINI_ABORT:
//         case protobuf::MINI_USER:
//         default:
//         {
//             user = decode_user;
//         }
//     }
//     m->set_data(user);

//     signal_mini_receive(*m);
// }
