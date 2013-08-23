// Copyright 2009-2013 Toby Schneider (https://launchpad.net/~tes)
//                     Massachusetts Institute of Technology (2007-)
//                     Woods Hole Oceanographic Institution (2007-)
//                     Goby Developers Team (https://launchpad.net/~goby-dev)
// 
//
// This file is part of the Goby Underwater Autonomy Project Libraries
// ("The Goby Libraries").
//
// The Goby Libraries are free software: you can redistribute them and/or modify
// them under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// The Goby Libraries are distributed in the hope that they will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with Goby.  If not, see <http://www.gnu.org/licenses/>.

#include <netinet/in.h>

#include <algorithm>

#include <boost/algorithm/string/classification.hpp>
#include <boost/crc.hpp>     

#include "iridium_driver.h"

#include "goby/util/base_convert.h"
#include "goby/acomms/modemdriver/driver_exception.h"
#include "goby/common/logger.h"
#include "goby/util/binary.h"

using goby::glog;
using goby::common::logger_lock::lock;
using namespace goby::common::logger;
using goby::common::goby_time;


goby::acomms::IridiumDriver::IridiumDriver()
    : call_state_(NOT_IN_CALL),
      command_state_(READY_TO_COMMAND),
      last_command_time_(0)
{
    assert(byte_string_to_uint32(uint32_to_byte_string(16540)) == 16540);
}

goby::acomms::IridiumDriver::~IridiumDriver()
{
}


void goby::acomms::IridiumDriver::startup(const protobuf::DriverConfig& cfg)
{
    driver_cfg_ = cfg;
    
    glog.is(DEBUG1, lock) && glog << group(glog_out_group()) << "Goby Iridium RUDICS/SBD driver starting up." << std::endl << unlock;

    driver_cfg_.set_line_delimiter("\r");
    
    modem_start(driver_cfg_);
    at_push_out("");
    at_push_out("E");
    for(int i = 0, n = driver_cfg_.ExtensionSize(IridiumDriverConfig::config); i < n; ++i)
        at_push_out(driver_cfg_.GetExtension(IridiumDriverConfig::config, i));

}

void goby::acomms::IridiumDriver::shutdown()
{
    modem_close();
}

void goby::acomms::IridiumDriver::handle_initiate_transmission(const protobuf::ModemTransmission& orig_msg)
{
    // buffer the message
    protobuf::ModemTransmission msg = orig_msg;
    signal_modify_transmission(&msg);

    msg.set_max_frame_bytes(driver_cfg_.GetExtension(IridiumDriverConfig::max_frame_size));
    signal_data_request(&msg);

    if(!(msg.frame_size() == 0 || msg.frame(0).empty()))
        send(msg);
}


void goby::acomms::IridiumDriver::do_work()
{
    try_at_write();
    try_data_write();
    
    std::string in;
    std::string blank;
    while(modem_read(&in))
    {
        if(command_state_ != IN_CALL_DATA)
        {
            boost::trim_if(in, !boost::algorithm::is_alnum());
            glog.is(DEBUG1) && glog << group(glog_in_group()) << in << std::endl;

        
            switch(command_state_)
            {
                case READY_TO_COMMAND:
                    boost::trim_if(in, !boost::algorithm::is_alnum());
                    if(in == "RING")
                    {
                        at_push_out("A");
                        command_state_ = WAITING_FOR_RESPONSE;
                    }
                    else
                        glog.is(DEBUG1) && glog << group(glog_in_group()) << "^ Unexpected message from modem." << std::endl;
                    break;

                case WAITING_FOR_RESPONSE:
                {
                    if(at_out_.empty())
                        break;

                    bool response_valid = false;
                
                    if(in == "OK")
                    {
                        response_valid = true;
                        command_state_ = READY_TO_COMMAND;
                    }
                    else if(in == "NO CARRIER")
                    {
                        response_valid = true;
                        command_state_ = READY_TO_COMMAND;
                    }
                    else if(in.compare(0, 7, "CONNECT") == 0)
                    {
                        response_valid = true;
                        command_state_ = IN_CALL_DATA;
                        call_state_ = IN_CALL;
                        data_out_.push_front("\r");
                    }

                    if(response_valid)
                    {
                        glog.is(DEBUG1) && glog << group(glog_in_group()) << "^ response to " << at_out_.front() << std::endl;
                        at_out_.pop_front();
                        modem_read(&blank);
                    }
                
                }
                break;                

                case IN_CALL_DATA: break;
            }
        }
        else
        {
            glog.is(DEBUG1) && glog << group(glog_in_group()) << goby::util::hex_encode(in) << std::endl;

            std::string bytes;
            try
            {
                parse_rudics_packet(&bytes, in);
                
                protobuf::ModemTransmission msg;
                msg.ParseFromString(bytes);
                receive(msg);
            }
            catch(RudicsPacketException& e)
            {
                glog.is(DEBUG1) && glog << group(glog_in_group()) << warn << "Could not decode packet: " << e.what() << std::endl;
            }
            
        }
    }

}

void goby::acomms::IridiumDriver::receive(const protobuf::ModemTransmission& msg)
{
    if(msg.type() == protobuf::ModemTransmission::DATA &&
       msg.ack_requested() && msg.dest() != BROADCAST_ID)
    {
        // make any acks
        protobuf::ModemTransmission ack;
        ack.set_type(goby::acomms::protobuf::ModemTransmission::ACK);
        ack.set_src(msg.dest());
        ack.set_dest(msg.src());
        for(int i = 0, n = msg.frame_size(); i < n; ++i)
            ack.add_acked_frame(i);
        send(ack);
    }
        
    signal_receive(msg);
}

void goby::acomms::IridiumDriver::send(const google::protobuf::Message& msg)
{
    // send the message
    std::string bytes;
    msg.SerializeToString(&bytes);

    // make call or use SBD?
    if(call_state_ == NOT_IN_CALL)
        dial_remote();

    // frame message
    std::string rudics_packet;
    serialize_rudics_packet(bytes, &rudics_packet);
    data_out_.push_back(rudics_packet);
}

void goby::acomms::IridiumDriver::at_push_out(const std::string& cmd)
{
    at_out_.push_back(cmd);
    try_at_write();
}


void goby::acomms::IridiumDriver::try_at_write()
{
    if(at_out_.empty())
        return;

    double now = goby::common::goby_time<double>();
    
    if(command_state_ == IN_CALL_DATA)
        return;
    else if(command_state_ == WAITING_FOR_RESPONSE)
    {
        if((last_command_time_ + COMMAND_TIMEOUT_SECONDS) > now)
            return;
        else
            glog.is(DEBUG1) && glog << warn << group(glog_out_group()) << "Resending last command..." << std::endl;
    }
    
    
    std::string at_cmd = "AT" + at_out_.front() + "\r";
    glog.is(DEBUG1) && glog << group(glog_out_group()) << at_cmd << std::endl;
    modem_write(at_cmd);

    last_command_time_ = now;

    command_state_ = WAITING_FOR_RESPONSE;
}


void goby::acomms::IridiumDriver::try_data_write()
{
    
    if(data_out_.empty())
    {
        return;
    }
    else if(command_state_ != IN_CALL_DATA)
    {
        return;    
    }
    
    glog.is(DEBUG1) && glog << group(glog_out_group()) << goby::util::hex_encode(data_out_.front()) << std::endl;

    modem_write(data_out_.front());
    data_out_.pop_front();
}


void goby::acomms::IridiumDriver::dial_remote()
{
    at_push_out("D" + driver_cfg_.GetExtension(IridiumDriverConfig::remote).iridium_number());
}


void goby::acomms::IridiumDriver::serialize_rudics_packet(std::string bytes, std::string* rudics_pkt)
{
    // append CRC
    boost::crc_32_type crc;
    crc.process_bytes(bytes.data(), bytes.length());
    bytes += uint32_to_byte_string(crc.checksum());
    
    // convert to base 255
    goby::util::base_convert(bytes, rudics_pkt, 256, 255);

    // turn all '\r' into 0xFF (255) so we can use '\r' as an end-of-line character
    std::replace(rudics_pkt->begin(), rudics_pkt->end(), '\r', static_cast<char>(0xFF));

    *rudics_pkt += "\r";
}

void goby::acomms::IridiumDriver::parse_rudics_packet(std::string* bytes, std::string rudics_pkt)
{
    const unsigned CR_SIZE = 1;    
    if(rudics_pkt.size() < CR_SIZE)
        throw(RudicsPacketException("Packet too short for <CR>"));
    
        
    rudics_pkt = rudics_pkt.substr(0, rudics_pkt.size()-1);
    std::replace(rudics_pkt.begin(), rudics_pkt.end(), static_cast<char>(0xFF), '\r');
    goby::util::base_convert(rudics_pkt, bytes, 255, 256);

    const unsigned CRC_BYTE_SIZE = 4;
    if(bytes->size() < CRC_BYTE_SIZE)
        throw(RudicsPacketException("Packet too short for CRC32"));

    std::string crc_str = bytes->substr(bytes->size()-4, 4);
    uint32 given_crc = byte_string_to_uint32(crc_str);
    *bytes = bytes->substr(0, bytes->size()-4);

    boost::crc_32_type crc;
    crc.process_bytes(bytes->data(), bytes->length());
    uint32_t computed_crc = crc.checksum();

    if(given_crc != computed_crc)
        throw(RudicsPacketException("Bad CRC32"));
    
}

std::string goby::acomms::IridiumDriver::uint32_to_byte_string(uint32_t i)
{
    union u_t { uint32_t i; char c[4]; } u;
    u.i = htonl(i);
    return std::string(u.c, 4);
}

uint32_t goby::acomms::IridiumDriver::byte_string_to_uint32(std::string s)
{
    union u_t { uint32_t i; char c[4]; } u;
    memcpy(u.c, s.c_str(), 4);
    return ntohl(u.i);
}

