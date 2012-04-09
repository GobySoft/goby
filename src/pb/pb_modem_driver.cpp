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
    query_interval_seconds_(1)
{
    on_receipt<util::protobuf::StoreServerResponse>(0, &PBDriver::handle_response, this);
}

void goby::pb::PBDriver::startup(const acomms::protobuf::DriverConfig& cfg)
{
    driver_cfg_ = cfg;
    request_.set_modem_id(driver_cfg_.modem_id());
    
    common::protobuf::ZeroMQServiceConfig service_cfg;
    service_cfg.add_socket()->CopyFrom(
        driver_cfg_.GetExtension(goby::pb::protobuf::Config::request_socket));
    zeromq_service_->set_cfg(service_cfg);

    request_socket_id_ =
        driver_cfg_.GetExtension(goby::pb::protobuf::Config::request_socket).socket_id();

    query_interval_seconds_ =
        driver_cfg_.GetExtension(goby::pb::protobuf::Config::query_interval_seconds);
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

    msg->set_max_num_frames(acomms::MMDriver::packet_frame_count(msg->rate()));
    msg->set_max_frame_bytes(acomms::MMDriver::packet_size(msg->rate()));
    signal_data_request(msg);
}


void goby::pb::PBDriver::do_work()
{
    while(zeromq_service_->poll(0))
    { }

    // call in with our outbox
    if(request_.IsInitialized() && goby_time<uint64>() > last_send_time_ + 1e6*query_interval_seconds_)
    {
        glog.is(DEBUG2) && glog << "Sending outbox" << std::endl;
        send(request_, request_socket_id_);
        request_.clear_outbox();
        last_send_time_ = goby_time<uint64>();
    }
}

void goby::pb::PBDriver::handle_response(const util::protobuf::StoreServerResponse& response)
{
    glog.is(DEBUG1) && glog << (size_t)this <<  ": Response: " << response.DebugString() << std::endl;

    for(int i = 0, n = response.inbox_size(); i < n; ++i)
    {
        signal_receive(response.inbox(i));
    }
}
