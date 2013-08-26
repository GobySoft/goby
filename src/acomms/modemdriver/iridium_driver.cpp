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
using namespace goby::common::logger;
using goby::common::goby_time;


goby::acomms::IridiumDriver::IridiumDriver()
    : fsm_(driver_cfg_)
{
    assert(byte_string_to_uint32(uint32_to_byte_string(16540)) == 16540);
    
    fsm_.initiate();
}

goby::acomms::IridiumDriver::~IridiumDriver()
{
}


void goby::acomms::IridiumDriver::startup(const protobuf::DriverConfig& cfg)
{
    driver_cfg_ = cfg;
    
    glog.is(DEBUG1) && glog << group(glog_out_group()) << "Goby Iridium RUDICS/SBD driver starting up." << std::endl;

    driver_cfg_.set_line_delimiter("\r");
    
    modem_start(driver_cfg_);

    fsm_.process_event(fsm::EvConfigured());
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
//    DisplayStateConfiguration();
        
    try_serial_tx();
    
    std::string in;
    while(modem_read(&in))
    {
        fsm::EvRxSerial data_event;
        data_event.line = in;


        glog.is(DEBUG1) && glog << group(glog_in_group())
                                << (boost::algorithm::all(in, boost::is_print() ||
                                                          boost::is_any_of("\r\n")) ? in :
                                    goby::util::hex_encode(in)) << std::endl;
        
        fsm_.process_event(data_event);
    }

    while(!fsm_.received().empty())
    {
        receive(fsm_.received().front());
        fsm_.received().pop_front();
    }

    // try sending again at the end to push newly generated messages before we wait
    try_serial_tx();
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

void goby::acomms::IridiumDriver::send(const protobuf::ModemTransmission& msg)
{
    fsm_.buffer_data_out(msg);
}


void goby::acomms::IridiumDriver::try_serial_tx()
{    
    fsm_.process_event(fsm::EvTxSerial());

    while(!fsm_.serial_tx_buffer().empty())
    {
        const std::string& line = fsm_.serial_tx_buffer().front();

        
        glog.is(DEBUG1) && glog << group(glog_out_group())
                                << (boost::algorithm::all(line, boost::is_print() ||
                                                          boost::is_any_of("\r\n")) ? line :
                                    goby::util::hex_encode(line)) << std::endl;
        
        modem_write(line);

        fsm_.serial_tx_buffer().pop_front();
    }
}

void goby::acomms::IridiumDriver::DisplayStateConfiguration()
{
  char orthogonalRegion = 'a';

  for ( fsm::IridiumDriverFSM::state_iterator pLeafState = fsm_.state_begin();
    pLeafState != fsm_.state_end(); ++pLeafState )
  {
    std::cout << "Orthogonal region " << orthogonalRegion << ": ";

    const fsm::IridiumDriverFSM::state_base_type * pState = &*pLeafState;

    while ( pState != 0 )
    {
      if ( pState != &*pLeafState )
      {
        std::cout << " -> ";
      }

      std::cout << std::setw( 15 ) << typeid( *pState ).name();
 
      pState = pState->outer_state_ptr();
    }

    std::cout << std::endl;
    ++orthogonalRegion;
  }

  std::cout << std::endl;
}


void goby::acomms::serialize_rudics_packet(std::string bytes, std::string* rudics_pkt)
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

void goby::acomms::parse_rudics_packet(std::string* bytes, std::string rudics_pkt)
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

std::string goby::acomms::uint32_to_byte_string(uint32_t i)
{
    union u_t { uint32_t i; char c[4]; } u;
    u.i = htonl(i);
    return std::string(u.c, 4);
}

uint32_t goby::acomms::byte_string_to_uint32(std::string s)
{
    union u_t { uint32_t i; char c[4]; } u;
    memcpy(u.c, s.c_str(), 4);
    return ntohl(u.i);
}

