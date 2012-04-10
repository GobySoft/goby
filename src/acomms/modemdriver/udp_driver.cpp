// Copyright 2009-2012 Toby Schneider (https://launchpad.net/~tes)
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


#include "udp_driver.h"

#include "goby/acomms/modemdriver/driver_exception.h"
#include "goby/acomms/modemdriver/mm_driver.h"
#include "goby/common/logger.h"
#include "goby/util/binary.h"

using goby::util::hex_encode;
using goby::util::hex_decode;
using goby::glog;
using namespace goby::common::logger;
using goby::common::goby_time;


const size_t UDP_MAX_PACKET_SIZE = 65507; // (16 bit length = 65535 - 8 byte UDP header -20 byte IP)


goby::acomms::UDPDriver::UDPDriver(boost::asio::io_service* io_service)
    : io_service_(io_service),
      socket_(*io_service),
      receive_buffer_(UDP_MAX_PACKET_SIZE)
{
}

void goby::acomms::UDPDriver::startup(const protobuf::DriverConfig& cfg)
{
    driver_cfg_ = cfg;
    
    
    const UDPDriverConfig::EndPoint& local = driver_cfg_.GetExtension(UDPDriverConfig::local);
    socket_.open(boost::asio::ip::udp::v4());
    socket_.bind(boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), local.port()));

    
    const UDPDriverConfig::EndPoint& remote = driver_cfg_.GetExtension(UDPDriverConfig::remote);
    boost::asio::ip::udp::resolver resolver(*io_service_);
    boost::asio::ip::udp::resolver::query query(boost::asio::ip::udp::v4(),
                                                remote.ip(),
                                                goby::util::as<std::string>(remote.port()));
    boost::asio::ip::udp::resolver::iterator endpoint_iterator = resolver.resolve(query);
    receiver_ = *endpoint_iterator;

    glog.is(DEBUG1) && glog << "Receiver endpoint is: " << receiver_.address().to_string()
                            << ":" << receiver_.port() << std::endl;
    
    start_receive();
    io_service_->reset();
}

void goby::acomms::UDPDriver::shutdown()
{ }

void goby::acomms::UDPDriver::handle_initiate_transmission(const protobuf::ModemTransmission& orig_msg)
{
    // buffer the message
    protobuf::ModemTransmission msg = orig_msg;
    signal_modify_transmission(&msg);

    msg.set_max_frame_bytes(driver_cfg_.GetExtension(UDPDriverConfig::max_frame_size));
    signal_data_request(&msg);

    start_send(msg);
}


void goby::acomms::UDPDriver::do_work()
{
    io_service_->poll();
}

void goby::acomms::UDPDriver::receive_message(const protobuf::ModemTransmission& msg)
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
        start_send(ack);
    }
        
    signal_receive(msg);
}

void goby::acomms::UDPDriver::start_send(const google::protobuf::Message& msg)
{
    // send the message
    std::string bytes;
    msg.SerializeToString(&bytes);
    
    socket_.async_send_to(boost::asio::buffer(bytes), receiver_, boost::bind(&UDPDriver::send_complete, this, _1, _2));
}


void goby::acomms::UDPDriver::send_complete(const boost::system::error_code& error, std::size_t bytes_transferred)
{
    if(error)
    {
        glog.is(WARN) && glog << "Send error: " << error.message() << std::endl;
        return;
    }
    
    glog.is(DEBUG1) && glog << "Sent " << bytes_transferred << " bytes." << std::endl;

}


void goby::acomms::UDPDriver::start_receive()
{
    socket_.async_receive_from(boost::asio::buffer(receive_buffer_), sender_, boost::bind(&UDPDriver::receive_complete, this, _1, _2));
    
}

void goby::acomms::UDPDriver::receive_complete(const boost::system::error_code& error, std::size_t bytes_transferred)
{
    if(error)
    {
        glog.is(WARN) && glog << "Receive error: " << error.message() << std::endl;
        start_receive();
        return;
    }
        
    glog.is(DEBUG1) && glog << "Received " << bytes_transferred << " bytes from " << sender_.address().to_string()
                            << ":" << sender_.port() << std::endl;
    
    protobuf::ModemTransmission msg;
    msg.ParseFromArray(&receive_buffer_[0], bytes_transferred);
    receive_message(msg);
            
    start_receive();
}
