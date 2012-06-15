// Copyright 2009-2012 Toby Schneider (https://launchpad.net/~tes)
//                     Massachusetts Institute of Technology (2007-)
//                     Woods Hole Oceanographic Institution (2007-)
//                     Goby Developers Team (https://launchpad.net/~goby-dev)
// 
//
// This file is part of the Goby Underwater Autonomy Project Binaries
// ("The Goby Binaries").
//
// The Goby Binaries are free software: you can redistribute them and/or modify
// them under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// The Goby Binaries are distributed in the hope that they will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Goby.  If not, see <http://www.gnu.org/licenses/>.

#include "goby/common/logger.h"
#include "goby/common/application_base.h"
#include "goby/common/logger/term_color.h"
#include "goby/common/zeromq_service.h"

#include "goby/acomms/queue.h"
#include "goby/acomms/route.h"
#include "goby/acomms/amac.h"
#include "goby/acomms/modem_driver.h"
#include "goby/acomms/modemdriver/udp_driver.h"
#include "goby/pb/pb_modem_driver.h"

#include "goby/acomms/bind.h"
#include "goby/acomms/connect.h"

#include "goby/acomms/protobuf/queue.pb.h"
#include "goby/acomms/protobuf/network_ack.pb.h"


#include "bridge_config.pb.h"

using namespace goby::common::logger;

namespace goby
{
    namespace acomms
    {
        class Bridge : public goby::common::ApplicationBase
        {
        public:
            Bridge();
            ~Bridge();

        private:
            void iterate();

            void handle_link_ack(const protobuf::ModemTransmission& ack_msg,
                                 const google::protobuf::Message& orig_msg,
                                 QueueManager* from_queue);
            
            void handle_micromodem_receive(const goby::acomms::protobuf::ModemTransmission& message, QueueManager* in_queue);
            
        private:
            static protobuf::BridgeConfig cfg_;
            
            // for PBDriver, maps drivers_ index to service
            std::map<size_t, boost::shared_ptr<goby::common::ZeroMQService> > zeromq_services_;
            
            // for UDPDriver, maps drivers_ index to service
            std::map<size_t, boost::shared_ptr<boost::asio::io_service> > asio_services_;
            
            std::vector<boost::shared_ptr<QueueManager> > q_managers_;
            std::vector<boost::shared_ptr<MACManager> > mac_managers_;
            std::vector<boost::shared_ptr<goby::acomms::ModemDriverBase> > drivers_;
            
            RouteManager r_manager_;
        };
    }
}

goby::acomms::protobuf::BridgeConfig goby::acomms::Bridge::cfg_;

int main(int argc, char* argv[])
{
    goby::run<goby::acomms::Bridge>(argc, argv);
}


using goby::glog;

goby::acomms::Bridge::Bridge()
    : ApplicationBase(&cfg_)
{
    glog.is(DEBUG1) && glog << cfg_.DebugString() << std::endl;
    
    // load all shared libraries
    for(int i = 0, n = cfg_.load_shared_library_size(); i < n; ++i)
    {
        glog.is(DEBUG1) &&
            glog << "Loading shared library: " << cfg_.load_shared_library(i) << std::endl;
        
        void* handle = goby::util::DynamicProtobufManager::load_from_shared_lib(cfg_.load_shared_library(i));
        
        if(!handle)
        {
            glog.is(DIE) && glog << "Failed ... check path provided or add to /etc/ld.so.conf "
                         << "or LD_LIBRARY_PATH" << std::endl;
        }

        glog.is(DEBUG1) && glog << "Loading shared library dccl codecs." << std::endl;
        
        goby::acomms::DCCLCodec::get()->load_shared_library_codecs(handle);
    }
    
    // load all .proto files
    goby::util::DynamicProtobufManager::enable_compilation();
    for(int i = 0, n = cfg_.load_proto_file_size(); i < n; ++i)
    {
        glog.is(DEBUG1) &&
            glog << "Loading protobuf file: " << cfg_.load_proto_file(i) << std::endl;
        
        if(!goby::util::DynamicProtobufManager::load_from_proto_file(cfg_.load_proto_file(i)))
            glog.is(DIE) && glog << "Failed to load file." << std::endl;
    }

    // validate all messages
    typedef boost::shared_ptr<google::protobuf::Message> GoogleProtobufMessagePointer;
    
    for(int i = 0, n = cfg_.load_dccl_message_size(); i < n; ++i)
    {
        GoogleProtobufMessagePointer msg = goby::util::DynamicProtobufManager::new_protobuf_message<GoogleProtobufMessagePointer>(cfg_.load_dccl_message(i));
        // validate with DCCL
        goby::acomms::DCCLCodec::get()->validate(msg->GetDescriptor());
    }
    
    r_manager_.set_cfg(cfg_.route_cfg());
    q_managers_.resize(cfg_.subnet_size());
    mac_managers_.resize(cfg_.subnet_size());
    drivers_.resize(cfg_.subnet_size());
    for(int i = 0, n = cfg_.subnet_size(); i < n; ++i)
    {
        q_managers_[i].reset(new QueueManager);
        mac_managers_[i].reset(new MACManager);
        
        q_managers_[i]->set_cfg(cfg_.subnet(i).queue_cfg());
        mac_managers_[i]->startup(cfg_.subnet(i).mac_cfg());

        switch(cfg_.subnet(i).driver_type())
        {
            case goby::acomms::protobuf::DRIVER_WHOI_MICROMODEM:
                drivers_[i].reset(new goby::acomms::MMDriver);
                goby::acomms::connect(&(drivers_[i]->signal_receive),
                                      boost::bind(&Bridge::handle_micromodem_receive,
                                                  this, _1, q_managers_[i].get()));
                break;

            case goby::acomms::protobuf::DRIVER_PB_STORE_SERVER:
                zeromq_services_[i].reset(new goby::common::ZeroMQService);
                drivers_[i].reset(new goby::pb::PBDriver(zeromq_services_[i].get()));
                break;

            case goby::acomms::protobuf::DRIVER_UDP:
                asio_services_[i].reset(new boost::asio::io_service);
                drivers_[i].reset(new goby::acomms::UDPDriver(asio_services_[i].get()));
                break;


            default:
            case goby::acomms::protobuf::DRIVER_NONE:
                throw(goby::Exception("Invalid driver specified"));
                break;
        }

        drivers_[i]->startup(cfg_.subnet(i).driver_cfg());

        goby::acomms::bind(*drivers_[i], *q_managers_[i], *mac_managers_[i]);
        goby::acomms::bind(*q_managers_[i], r_manager_);

        goby::acomms::connect(&(q_managers_[i]->signal_ack),
                              boost::bind(&Bridge::handle_link_ack, this, _1, _2, q_managers_[i].get()));
    }    


    protobuf::NetworkAck ack;

    assert(ack.GetDescriptor() == google::protobuf::DescriptorPool::generated_pool()->FindMessageTypeByName("goby.acomms.protobuf.NetworkAck"));

    assert(ack.GetDescriptor() == goby::util::DynamicProtobufManager::new_protobuf_message("goby.acomms.protobuf.NetworkAck")->GetDescriptor());

}


goby::acomms::Bridge::~Bridge()
{
    for(std::vector<boost::shared_ptr<goby::acomms::ModemDriverBase> >::iterator it = drivers_.begin(),
            end = drivers_.end(); it != end; ++it)
    {
        (*it)->shutdown();
    }
}


void goby::acomms::Bridge::iterate()
{
    for(std::vector<boost::shared_ptr<QueueManager> >::iterator it = q_managers_.begin(),
            end = q_managers_.end(); it != end; ++it)
    {
        (*it)->do_work();
    }
    
    for(std::vector<boost::shared_ptr<MACManager> >::iterator it = mac_managers_.begin(),
            end = mac_managers_.end(); it != end; ++it)
    {
        (*it)->do_work();
    }

    for(std::vector<boost::shared_ptr<goby::acomms::ModemDriverBase> >::iterator it = drivers_.begin(),
            end = drivers_.end(); it != end; ++it)
    {
        (*it)->do_work();
    }

    usleep(100000);
}

void goby::acomms::Bridge::handle_link_ack(const protobuf::ModemTransmission& ack_msg,
                                           const google::protobuf::Message& orig_msg,
                                           QueueManager* from_queue)
{
    if(orig_msg.GetDescriptor()->full_name() == "goby.acomms.protobuf.NetworkAck")
    {
        glog.is(DEBUG1) && glog << "Not generating network ack from NetworkAck to avoid infinite proliferation of ACKS." << std::endl;
        return;
    }
    
    protobuf::NetworkAck ack;
    ack.set_ack_src(ack_msg.src());
    ack.set_message_dccl_id(DCCLCodec::get()->id(orig_msg.GetDescriptor()));


    protobuf::QueuedMessageMeta meta = from_queue->meta_from_msg(orig_msg);
    ack.set_message_src(meta.src());
    ack.set_message_dest(meta.dest());
    ack.set_message_time(meta.time());

    glog.is(VERBOSE) && glog << "Generated network ack: " << ack.DebugString() << "from: " << orig_msg.DebugString() << std::endl;
    
    r_manager_.handle_in(from_queue->meta_from_msg(ack), ack, from_queue->modem_id());
}


void goby::acomms::Bridge::handle_micromodem_receive(const goby::acomms::protobuf::ModemTransmission& message, QueueManager* in_queue)
{
    for(int i = 0, n = message.ExtensionSize(micromodem::protobuf::receive_stat);
        i < n; ++i)
    {

        micromodem::protobuf::ReceiveStatistics cacst =
            message.GetExtension(micromodem::protobuf::receive_stat, i);
        cacst.set_forward_src(in_queue->modem_id());
        cacst.set_forward_dest(cfg_.topside_modem_id());
        
        glog.is(VERBOSE) && glog << "Forwarding statistics message to topside: " << cacst << std::endl;
        r_manager_.handle_in(in_queue->meta_from_msg(cacst),
                             cacst,
                             in_queue->modem_id());
    }
}
