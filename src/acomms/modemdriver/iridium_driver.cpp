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


#include <algorithm>


#include "iridium_driver.h"

#include "goby/acomms/modemdriver/driver_exception.h"
#include "goby/common/logger.h"
#include "goby/util/binary.h"

using goby::glog;
using namespace goby::common::logger;
using goby::common::goby_time;


goby::acomms::IridiumDriver::IridiumDriver()
    : fsm_(driver_cfg_),
      last_triple_plus_time_(0)
{
//    assert(byte_string_to_uint32(uint32_to_byte_string(16540)) == 16540);
}

goby::acomms::IridiumDriver::~IridiumDriver()
{
}


void goby::acomms::IridiumDriver::startup(const protobuf::DriverConfig& cfg)
{
    driver_cfg_ = cfg;
    
    glog.is(DEBUG1) && glog << group(glog_out_group()) << "Goby Iridium RUDICS/SBD driver starting up." << std::endl;

    driver_cfg_.set_line_delimiter("\r");    

    if(driver_cfg_.HasExtension(IridiumDriverConfig::debug_client_port))
    {
        debug_client_.reset(new goby::util::TCPClient("localhost", driver_cfg_.GetExtension(IridiumDriverConfig::debug_client_port), "\r"));
        debug_client_->start();
    }
    
    modem_init(cfg);
}

void goby::acomms::IridiumDriver::modem_init(const protobuf::DriverConfig& cfg)
{
        
    modem_start(driver_cfg_);
    
    fsm_.initiate();


    int i = 0;
    while(fsm_.state_cast<const fsm::Ready *>() == 0)
    {
        do_work();

        const int pause_ms = 10;
        usleep(pause_ms*1000);
        ++i;

        const int start_timeout = 10;
        if(i / (1000/pause_ms) > start_timeout)
            glog.is(DIE) && glog << group(glog_out_group()) << "Failed to startup." << std::endl;
    }
}

void goby::acomms::IridiumDriver::shutdown()
{
    fsm_.process_event(fsm::EvHangup());

    while(fsm_.state_cast<const fsm::OnCall *>() != 0)
    {
        do_work();
        usleep(10000);
    }
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
    while(modem().active() && modem_read(&in))
    {
        fsm::EvRxSerial data_event;
        data_event.line = in;
        
        if(debug_client_ && fsm_.state_cast<const fsm::OnCall *>() != 0)
        {    
            debug_client_->write(in);
        }
        
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

    if(debug_client_)
    {    
        std::string line;
        while(debug_client_->readline(&line))
        {
            fsm_.serial_tx_buffer().push_back(line);
            fsm_.process_event(fsm::EvDial());
        }
        
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
        double now = goby_time<double>();
        if(last_triple_plus_time_ + TRIPLE_PLUS_WAIT > now)
            return;
    
        const std::string& line = fsm_.serial_tx_buffer().front();

        
        glog.is(DEBUG1) && glog << group(glog_out_group())
                                << (boost::algorithm::all(line, boost::is_print() ||
                                                          boost::is_any_of("\r\n")) ? line :
                                    goby::util::hex_encode(line)) << std::endl;
        
        modem_write(line);

        // this is safe as all other messages we use are \r terminated
        if(line == "+++") last_triple_plus_time_ = now;
        
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
