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


#include "iridium_driver.h"

#include "goby/acomms/modemdriver/driver_exception.h"
#include "goby/common/logger.h"
#include "goby/util/binary.h"

using goby::glog;
using goby::common::logger_lock::lock;
using namespace goby::common::logger;
using goby::common::goby_time;


goby::acomms::IridiumDriver::IridiumDriver()
{
}

goby::acomms::IridiumDriver::~IridiumDriver()
{
}


void goby::acomms::IridiumDriver::startup(const protobuf::DriverConfig& cfg)
{
    driver_cfg_ = cfg;
    
    glog.is(DEBUG1, lock) && glog << group(glog_out_group()) << "Goby Iridium RUDICS/SBD driver starting up." << std::endl << unlock;

    modem_start(driver_cfg_);
   
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

}

