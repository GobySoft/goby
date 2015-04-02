// Copyright 2009-2014 Toby Schneider (https://launchpad.net/~tes)
//                     GobySoft, LLC (2013-)
//                     Massachusetts Institute of Technology (2007-2014)
//                     Goby Developers Team (https://launchpad.net/~goby-dev)
// 
//
// This file is part of the Goby Underwater Autonomy Project Libraries
// ("The Goby Libraries").
//
// The Goby Libraries are free software: you can redistribute them and/or modify
// them under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 2.1 of the License, or
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
#include "goby/pb/rudics_packet.h"


using goby::glog;
using namespace goby::common::logger;
using goby::common::goby_time;


goby::acomms::IridiumDriver::IridiumDriver(goby::common::ZeroMQService* zeromq_service)
    :     StaticProtobufNode(zeromq_service),
          fsm_(driver_cfg_),
          last_triple_plus_time_(0),
          using_zmq_(false),
          zeromq_service_(zeromq_service),
          request_socket_id_(0),
          subscribe_socket_id_(1),
          last_zmq_request_time_(0),
          query_interval_seconds_(1),
          waiting_for_reply_(false),
          last_send_time_(0),
          serial_fd_(-1),
          next_frame_(0)
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


    if(!driver_cfg_.HasExtension(IridiumDriverConfig::use_dtr) && 
        driver_cfg_.connection_type() == protobuf::DriverConfig::CONNECTION_SERIAL)
    {
        driver_cfg_.SetExtension(IridiumDriverConfig::use_dtr, true);
    }
    
    // dtr low hangs up
    if(driver_cfg_.GetExtension(IridiumDriverConfig::use_dtr))
      driver_cfg_.AddExtension(IridiumDriverConfig::config, "&D2"); 


    using_zmq_ = driver_cfg_.HasExtension(IridiumDriverConfig::request_socket);
    
    glog.is(DEBUG1) && glog << "Using ZMQ? " << (using_zmq_ ? "Yes" : "No") << std::endl;

    if(using_zmq_)
    {
        if(driver_cfg_.GetExtension(IridiumDriverConfig::request_socket).has_socket_id())
            request_socket_id_ = driver_cfg_.GetExtension(IridiumDriverConfig::request_socket).socket_id();
        else
            driver_cfg_.MutableExtension(IridiumDriverConfig::request_socket)->set_socket_id(request_socket_id_);
        
        if(driver_cfg_.GetExtension(IridiumDriverConfig::subscribe_socket).has_socket_id())
            subscribe_socket_id_ = driver_cfg_.GetExtension(IridiumDriverConfig::subscribe_socket).socket_id();
        else
            driver_cfg_.MutableExtension(IridiumDriverConfig::subscribe_socket)->set_socket_id(subscribe_socket_id_);

        request_.set_modem_id(driver_cfg_.modem_id());
        
        service_cfg_.add_socket()->CopyFrom(
            driver_cfg_.GetExtension(IridiumDriverConfig::request_socket));
        
        service_cfg_.add_socket()->CopyFrom(
            driver_cfg_.GetExtension(IridiumDriverConfig::subscribe_socket));
        
        zeromq_service_->set_cfg(service_cfg_);

        
        on_receipt<acomms::protobuf::MTDataResponse>(request_socket_id_, &IridiumDriver::handle_mt_response, this);
        subscribe<acomms::protobuf::MODataAsyncReceive>(subscribe_socket_id_, &IridiumDriver::handle_mo_async_receive, this);
    }
    
    rudics_mac_msg_.set_src(driver_cfg_.modem_id()); 
    rudics_mac_msg_.set_type(goby::acomms::protobuf::ModemTransmission::DATA);
    rudics_mac_msg_.set_rate(RATE_RUDICS);

    modem_init();
}

void goby::acomms::IridiumDriver::modem_init()
{
        
    modem_start(driver_cfg_);
    

    fsm_.initiate();

       
    int i = 0;
    bool dtr_set = false;
    while(fsm_.state_cast<const fsm::Ready *>() == 0)
    {
        do_work();

        if(driver_cfg_.GetExtension(IridiumDriverConfig::use_dtr) &&
           modem().active() &&
           !dtr_set)
        {
            serial_fd_ = dynamic_cast<util::SerialClient&>(modem()).socket().native();
            set_dtr(true);
            glog.is(DEBUG1) && glog << group(glog_out_group()) << "DTR is: " << query_dtr() << std::endl;
            dtr_set = true;

        }
        
        const int pause_ms = 10;
        usleep(pause_ms*1000);
        ++i;

        const int start_timeout = driver_cfg_.GetExtension(IridiumDriverConfig::start_timeout);
        if(i / (1000/pause_ms) > start_timeout)
            throw(ModemDriverException("Failed to startup.",protobuf::ModemDriverStatus::STARTUP_FAILED));
    }
}

void goby::acomms::IridiumDriver::set_dtr(bool state)
{
    int status;
    if(ioctl(serial_fd_, TIOCMGET, &status) == -1)
    {
        glog.is(DEBUG1) && glog << group(glog_out_group()) << warn << "IOCTL failed: " << strerror(errno) << std::endl;            
    }
    
    glog.is(DEBUG1) && glog << group(glog_out_group()) << "Setting DTR to " << (state ? "high" : "low" ) << std::endl;
    if(state) status |= TIOCM_DTR;
    else status &= ~TIOCM_DTR;
    if(ioctl(serial_fd_, TIOCMSET, &status) == -1)
    {
        glog.is(DEBUG1) && glog << group(glog_out_group()) << warn << "IOCTL failed: " << strerror(errno) << std::endl;            
    }
}

bool goby::acomms::IridiumDriver::query_dtr()
{
    int status;
    if(ioctl(serial_fd_, TIOCMGET, &status) == -1)
    {
        glog.is(DEBUG1) && glog << group(glog_out_group()) << warn << "IOCTL failed: " << strerror(errno) << std::endl;            
    }    
    return (status & TIOCM_DTR);
}

void goby::acomms::IridiumDriver::hangup()
{
  if(driver_cfg_.GetExtension(IridiumDriverConfig::use_dtr))
    {
        set_dtr(false);
        sleep(1);
        set_dtr(true);
        // phone doesn't give "NO CARRIER" message after DTR disconnect
        fsm_.process_event(fsm::EvNoCarrier());
    }
    else
    {
        fsm_.process_event(fsm::EvHangup());
    }
}

void goby::acomms::IridiumDriver::shutdown()
{
    hangup();

    while(fsm_.state_cast<const fsm::OnCall *>() != 0)
    {
        do_work();
        usleep(10000);
    }

    if(driver_cfg_.GetExtension(IridiumDriverConfig::use_dtr))
        set_dtr(false);
    
    modem_close();
}

void goby::acomms::IridiumDriver::handle_initiate_transmission(const protobuf::ModemTransmission& orig_msg)
{
    process_transmission(orig_msg, true);
}

void goby::acomms::IridiumDriver::process_transmission(protobuf::ModemTransmission msg, bool dial)
{
    signal_modify_transmission(&msg);

    if(next_frame_ >= FRAME_COUNT_ROLLOVER)
        next_frame_ = 0;
    if(!msg.has_frame_start())
        msg.set_frame_start(next_frame_++);

    // set the frame size, if not set or if it exceeds the max configured
    if(!msg.has_max_frame_bytes() || msg.max_frame_bytes() > driver_cfg_.GetExtension(IridiumDriverConfig::max_frame_size))
        msg.set_max_frame_bytes(driver_cfg_.GetExtension(IridiumDriverConfig::max_frame_size));
    
    signal_data_request(&msg);

    if(!(msg.frame_size() == 0 || msg.frame(0).empty()))
    {
        if(dial && msg.rate() == RATE_RUDICS)
            fsm_.process_event(fsm::EvDial());
        send(msg);
    }
    else if(msg.rate() == RATE_SBD && !using_zmq_)
    {
        fsm_.process_event(fsm::EvSBDBeginData()); // mailbox check
    }
}



void goby::acomms::IridiumDriver::do_work()
{
    //   if(glog.is(DEBUG1))
    //    display_state_cfg(&glog);
    double now = goby_time<double>();
    
    // if we're on a call, keep pushing data at the target rate
    const double send_interval =
        driver_cfg_.GetExtension(IridiumDriverConfig::max_frame_size) /
        (driver_cfg_.GetExtension(IridiumDriverConfig::target_bit_rate) / static_cast<double>(BITS_IN_BYTE));

    
    const fsm::OnCall* on_call = fsm_.state_cast<const fsm::OnCall*>();
    const fsm::OnZMQCall* on_zmq_call = fsm_.state_cast<const fsm::OnZMQCall*>();
    const fsm::OnCallBase* on_call_base = (on_call) ?
        static_cast<const fsm::OnCallBase*>(on_call) :
        static_cast<const fsm::OnCallBase*>(on_zmq_call);
    
    if(fsm_.data_out().empty() &&
       (now > (last_send_time_ + send_interval)))
    {
        if(on_call_base && !on_call_base->bye_sent())
        {        
            process_transmission(rudics_mac_msg_, false);
            last_send_time_ = now;
        }
    }

    // if we're on either type of call, see if we need to send the "bye" message or hangup
    if(on_call_base)
    {
        glog.is(DEBUG2) && glog << on_call_base->last_tx_time() << std::endl;
        if(!on_call_base->bye_sent() &&
           now > (on_call_base->last_tx_time() + driver_cfg_.GetExtension(IridiumDriverConfig::handshake_hangup_seconds)))
        {
            glog.is(DEBUG2) && glog << "Sending bye" << std::endl;
            fsm_.process_event(fsm::EvSendBye());
            if(on_zmq_call)
                request_.set_command(goby::acomms::protobuf::MTDataRequest::SEND_BYE);
        }


        if((on_call_base->bye_received() && on_call_base->bye_sent()) ||
           (now > (on_call_base->last_rx_tx_time() + driver_cfg_.GetExtension(IridiumDriverConfig::hangup_seconds_after_empty))))
        {
            if(on_zmq_call)
                request_.set_command(goby::acomms::protobuf::MTDataRequest::HANGUP);
            else
                hangup();
        }
    }

    if(using_zmq_)
        do_zmq_work(now, on_zmq_call);    
    
    try_serial_tx();
    
    std::string in;
    while(modem().active() && modem_read(&in))
    {
        fsm::EvRxSerial data_event;
        data_event.line = in;

        
        glog.is(DEBUG1) && glog << group(glog_in_group())
                                << (boost::algorithm::all(in, boost::is_print() ||
                                                          boost::is_any_of("\r\n")) ?
                                    boost::trim_copy(in) :
                                    goby::util::hex_encode(in)) << std::endl;
        
        
        if(debug_client_ && fsm_.state_cast<const fsm::OnCall *>() != 0)
        {
            debug_client_->write(in);
        }
        
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

void goby::acomms::IridiumDriver::do_zmq_work(double now, const fsm::OnZMQCall* on_zmq_call)
{
    while(zeromq_service_->poll(0))
    { }
        
    if(!waiting_for_reply_ &&
       request_.IsInitialized())
    {
        static int request_id = 0;
        request_.set_request_id(request_id++);
            
        while(!fsm_.data_out().empty())
        {
            if(on_zmq_call || fsm_.data_out().front().rate() == RATE_SBD)
            {
                (*request_.add_outbox()) = fsm_.data_out().front();
                fsm_.data_out().pop_front();
            }
            else
            {
                break; // we're not on a call and the next packet isn't SBD
            }
        }

        if(request_.outbox_size() || request_.has_command())
        {            
            glog.is(DEBUG2) && glog << group(goby::common::ZeroMQService::glog_out_group()) << "Sending request to server." << std::endl;
            glog.is(DEBUG2) && glog << group(goby::common::ZeroMQService::glog_out_group()) << "Outbox: " << request_.DebugString() << std::flush;
            StaticProtobufNode::send(request_, request_socket_id_);
            last_zmq_request_time_ = now;
            request_.clear_outbox();
            request_.clear_command();
            waiting_for_reply_ = true;
        }
    }
}


void goby::acomms::IridiumDriver::receive(const protobuf::ModemTransmission& msg)
{
    glog.is(DEBUG2) && glog << group(glog_in_group()) << msg << std::endl;

    if(msg.type() == protobuf::ModemTransmission::DATA &&
       msg.ack_requested() && msg.dest() == driver_cfg_.modem_id())
    {
        // make any acks
        protobuf::ModemTransmission ack;
        ack.set_type(goby::acomms::protobuf::ModemTransmission::ACK);
        ack.set_src(msg.dest());
        ack.set_dest(msg.src());
        ack.set_rate(msg.rate());
        for(int i = msg.frame_start(), n = msg.frame_size() + msg.frame_start(); i < n; ++i)
            ack.add_acked_frame(i);
        send(ack);
    }
    
    signal_receive(msg);
}

void goby::acomms::IridiumDriver::send(const protobuf::ModemTransmission& msg)
{
    glog.is(DEBUG2) && glog << group(glog_out_group()) << msg << std::endl;

    if(msg.rate() == RATE_RUDICS)
        fsm_.buffer_data_out(msg);
    else if(msg.rate() == RATE_SBD)
    {
        bool on_call = (fsm_.state_cast<const fsm::OnCall *>() != 0);
        if(using_zmq_ || on_call) // if we're on a call send it via the call
            fsm_.buffer_data_out(msg);
        else
        {
            std::string rudics_packet;
            serialize_rudics_packet(msg.SerializeAsString(), &rudics_packet);
            fsm_.process_event(fsm::EvSBDBeginData(rudics_packet));
        }
        
    }
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
                                                          boost::is_any_of("\r\n")) ?
                                    boost::trim_copy(line) :
                                    goby::util::hex_encode(line)) << std::endl;
        
        modem_write(line);

        // this is safe as all other messages we use are \r terminated
        if(line == "+++") last_triple_plus_time_ = now;
        
        fsm_.serial_tx_buffer().pop_front();
    }
}

void goby::acomms::IridiumDriver::handle_mt_response(const acomms::protobuf::MTDataResponse& response)
{
    glog.is(DEBUG2) && glog << group(goby::common::ZeroMQService::glog_in_group()) << "Received response from shore server:" <<  response.DebugString() << std::endl;
    
    if(response.mobile_node_connected())
        fsm_.process_event(fsm::EvZMQConnect());
    else
        fsm_.process_event(fsm::EvZMQDisconnect());

    
    waiting_for_reply_ = false;  
}

void goby::acomms::IridiumDriver::handle_mo_async_receive(const acomms::protobuf::MODataAsyncReceive& rx)
{
    glog.is(DEBUG2) && glog << group(goby::common::ZeroMQService::glog_in_group()) << "Received async data from shore server:" <<  rx.DebugString() << std::endl;
    
    receive(rx.inbox());
}



void goby::acomms::IridiumDriver::display_state_cfg(std::ostream* os)
{
  char orthogonalRegion = 'a';
  
  for ( fsm::IridiumDriverFSM::state_iterator pLeafState = fsm_.state_begin();
    pLeafState != fsm_.state_end(); ++pLeafState )
  {
      *os << "Orthogonal region " << orthogonalRegion << ": ";
      
      const fsm::IridiumDriverFSM::state_base_type * pState = &*pLeafState;
      
      while ( pState != 0 )
      {
          if ( pState != &*pLeafState )
          {
              *os << " -> ";
          }

          std::string name = typeid( *pState ).name();
          std::string prefix = "N4goby6acomms3fsm";
          std::string suffix = "E";

          if(name.find(prefix) != std::string::npos && name.find(suffix) != std::string::npos)
              name = name.substr(prefix.size() + 1, name.size()-prefix.size()-suffix.size()-1);
          
          
          *os << name;
          
          pState = pState->outer_state_ptr();
      }
      
      *os << "\n";
      ++orthogonalRegion;
  }
  
  *os << std::endl;
}
