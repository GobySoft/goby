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


#include "pb_modem_driver.h"

#include "goby/acomms/modemdriver/driver_exception.h"
#include "goby/acomms/modemdriver/mm_driver.h"
#include "goby/common/logger.h"
#include "goby/util/binary.h"

using goby::util::hex_encode;
using goby::util::hex_decode;
using goby::glog;
using namespace goby::common::logger;
using goby::common::goby_time;


goby::pb::PBDriver::PBDriver(goby::common::ZeroMQService* zeromq_service) :
    StaticProtobufNode(zeromq_service),
    zeromq_service_(zeromq_service),
    last_send_time_(goby_time<uint64>()),
    request_socket_id_(0),
    query_interval_seconds_(1),
    reset_interval_seconds_(120),
    waiting_for_reply_(false)
{
    on_receipt<acomms::protobuf::StoreServerResponse>(0, &PBDriver::handle_response, this);
}

void goby::pb::PBDriver::startup(const acomms::protobuf::DriverConfig& cfg)
{
    driver_cfg_ = cfg;
    request_.set_modem_id(driver_cfg_.modem_id());
    
    service_cfg_.add_socket()->CopyFrom(
        driver_cfg_.GetExtension(PBDriverConfig::request_socket));
    zeromq_service_->set_cfg(service_cfg_);

    request_socket_id_ =
        driver_cfg_.GetExtension(PBDriverConfig::request_socket).socket_id();

    query_interval_seconds_ =
        driver_cfg_.GetExtension(PBDriverConfig::query_interval_seconds);

    reset_interval_seconds_ =
        driver_cfg_.GetExtension(PBDriverConfig::reset_interval_seconds);

}

void goby::pb::PBDriver::shutdown()
{
    
}

void goby::pb::PBDriver::handle_initiate_transmission(const acomms::protobuf::ModemTransmission& orig_msg)
{
    // buffer the message
    acomms::protobuf::ModemTransmission* msg = request_.add_outbox();
    msg->CopyFrom(orig_msg);
    signal_modify_transmission(msg);

    msg->set_max_frame_bytes(driver_cfg_.GetExtension(PBDriverConfig::max_frame_size));
    signal_data_request(msg);

    if(msg->frame_size() == 0 || msg->frame(0).empty())
        request_.mutable_outbox()->RemoveLast();
}


void goby::pb::PBDriver::do_work()
{
    while(zeromq_service_->poll(0))
    { }

    // call in with our outbox
    if(!waiting_for_reply_ &&
       request_.IsInitialized() &&
       goby_time<uint64>() > last_send_time_ + 1e6*query_interval_seconds_)
    {

        glog.is(DEBUG1) && glog << group(glog_out_group()) << "Sending to server." << std::endl;
        glog.is(DEBUG2) && glog << group(glog_out_group()) << "Outbox: " << request_.DebugString() << std::flush;
        send(request_, request_socket_id_);
        last_send_time_ = goby_time<uint64>();
        waiting_for_reply_ = true;
    }
    else if(waiting_for_reply_ &&
	    goby_time<uint64>() > last_send_time_ + 1e6*reset_interval_seconds_)
    {
        glog.is(DEBUG1) && glog << group(glog_out_group()) << warn 
				<< "No response in " << reset_interval_seconds_ << " seconds, resetting socket." << std::endl;
	zeromq_service_->close_all();
        zeromq_service_->set_cfg(service_cfg_);
	waiting_for_reply_ = false;
    }
}

void goby::pb::PBDriver::handle_response(const acomms::protobuf::StoreServerResponse& response)
{
    glog.is(DEBUG1) && glog << group(glog_in_group()) << "Received response in " << (goby_time<uint64_t>() - last_send_time_)/1.0e6 << " seconds." << std::endl;

    glog.is(DEBUG2) && glog << group(glog_in_group()) << "Inbox: " << response.DebugString() << std::flush;

    for(int i = 0, n = response.inbox_size(); i < n; ++i)
    {
        // ack any packets
        const acomms::protobuf::ModemTransmission& msg = response.inbox(i);
        if(msg.dest() == driver_cfg_.modem_id() &&
           msg.type() == acomms::protobuf::ModemTransmission::DATA &&
           msg.ack_requested())
        {
            acomms::protobuf::ModemTransmission& ack = *request_.add_outbox();
            ack.set_type(goby::acomms::protobuf::ModemTransmission::ACK);
            ack.set_src(msg.dest());
            ack.set_dest(msg.src());
            for(int i = 0, n = msg.frame_size(); i < n; ++i)
                ack.add_acked_frame(i);
            
        }
        
        signal_receive(msg);
    }

    request_.clear_outbox();
    waiting_for_reply_ = false;
}
