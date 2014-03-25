// Copyright 2009-2014 Toby Schneider (https://launchpad.net/~tes)
//                     GobySoft, LLC (2013-)
//                     Massachusetts Institute of Technology (2007-2014)
//                     Goby Developers Team (https://launchpad.net/~goby-dev)
// 
//
// This file is part of the Goby Underwater Autonomy Project Binaries
// ("The Goby Binaries").
//
// The Goby Binaries are free software: you can redistribute them and/or modify
// them under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
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
#include "goby/common/logger/term_color.h"
#include "goby/common/zeromq_service.h"

#include "goby/pb/application.h"

#include "goby/acomms/queue.h"
#include "goby/acomms/route.h"
#include "goby/acomms/amac.h"


#include "goby/acomms/bind.h"
#include "goby/acomms/connect.h"

#include "goby/acomms/protobuf/queue.pb.h"
#include "goby/acomms/protobuf/network_ack.pb.h"

#include "goby/acomms/protobuf/file_transfer.pb.h"

#include "bridge_config.pb.h"

using namespace goby::common::logger;

namespace goby
{
    namespace acomms
    {
        class Bridge : public goby::pb::Application
        {
        public:
            Bridge();
            ~Bridge();

        private:
            void loop();

            void handle_link_ack(const protobuf::ModemTransmission& ack_msg,
                                 const google::protobuf::Message& orig_msg,
                                 QueueManager* from_queue);
            
            void handle_queue_receive(const google::protobuf::Message& msg,
                                      QueueManager* from_queue);

            void handle_modem_receive(const goby::acomms::protobuf::ModemTransmission& message,
                                      QueueManager* in_queue);


            template<typename ProtobufMessage>
            void handle_external_push(const ProtobufMessage& message,
                                      QueueManager* in_queue)
                {
                    try
                    {
                        in_queue->push_message(message);
                    }
                    catch(std::exception& e)
                    {
                        glog.is(WARN) && glog << "Failed to push message: " << e.what() << std::endl;
                    }
                }
            
            
            void handle_initiate_transmission(const protobuf::ModemTransmission& m, int subnet);

            void handle_data_request(const protobuf::ModemTransmission& m, int subnet);
            
        private:
            static protobuf::BridgeConfig cfg_;
            
            std::vector<boost::shared_ptr<QueueManager> > q_managers_;
            std::vector<boost::shared_ptr<MACManager> > mac_managers_;
            
            RouteManager r_manager_;

            typedef int ModemId;
            std::set<ModemId> network_ack_src_ids_;
                
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
    : Application(&cfg_)
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
    
    r_manager_.set_cfg(cfg_.route_cfg());
    q_managers_.resize(cfg_.subnet_size());
    mac_managers_.resize(cfg_.subnet_size());
    for(int i = 0, n = cfg_.subnet_size(); i < n; ++i)
    {
        q_managers_[i].reset(new QueueManager);
        mac_managers_[i].reset(new MACManager);

        goby::acomms::protobuf::QueueManagerConfig qcfg = cfg_.subnet(i).queue_cfg();
        q_managers_[i]->set_cfg(qcfg);

        
        mac_managers_[i]->startup(cfg_.subnet(i).mac_cfg());
        
        goby::acomms::bind(*q_managers_[i], r_manager_);

        goby::acomms::connect(&(q_managers_[i]->signal_ack),
                              boost::bind(&Bridge::handle_link_ack, this, _1, _2, q_managers_[i].get()));

        goby::acomms::connect(&(q_managers_[i]->signal_receive),
                              boost::bind(&Bridge::handle_queue_receive, this, _1, q_managers_[i].get()));
        
        subscribe<goby::acomms::protobuf::ModemTransmission>(
            boost::bind(&Bridge::handle_modem_receive, this, _1, q_managers_[i].get()),
            "Rx" + goby::util::as<std::string>(qcfg.modem_id()));

        subscribe<goby::acomms::protobuf::TransferRequest>(
            boost::bind(&Bridge::handle_external_push<goby::acomms::protobuf::TransferRequest>, this, _1, q_managers_[i].get()),
            "QueuePush" + goby::util::as<std::string>(qcfg.modem_id()));

        subscribe<goby::acomms::protobuf::FileFragment>(
            boost::bind(&Bridge::handle_external_push<goby::acomms::protobuf::FileFragment>, this, _1, q_managers_[i].get()),
            "QueuePush" + goby::util::as<std::string>(qcfg.modem_id()));
        
        subscribe<goby::acomms::protobuf::TransferResponse>(
            boost::bind(&Bridge::handle_external_push<goby::acomms::protobuf::TransferResponse>, this, _1, q_managers_[i].get()),
            "QueuePush" + goby::util::as<std::string>(qcfg.modem_id()));

        subscribe<goby::acomms::protobuf::ModemTransmission>(
            boost::bind(&Bridge::handle_data_request, this, _1, i),
            "DataRequest" + goby::util::as<std::string>(qcfg.modem_id()));

        goby::acomms::connect(&mac_managers_[i]->signal_initiate_transmission,
                              boost::bind(&Bridge::handle_initiate_transmission, this, _1, i));
        
    }    

    for(int i = 0, n = cfg_.make_network_ack_for_src_id_size();
        i < n; ++i)
    {
        glog.is(DEBUG1) && glog << "Generating NetworkAck for messages required ACK from source ID: " << cfg_.make_network_ack_for_src_id(i) << std::endl;

        network_ack_src_ids_.insert(cfg_.make_network_ack_for_src_id(i));
    }
    

    protobuf::NetworkAck ack;

    assert(ack.GetDescriptor() == google::protobuf::DescriptorPool::generated_pool()->FindMessageTypeByName("goby.acomms.protobuf.NetworkAck"));

    assert(ack.GetDescriptor() == goby::util::DynamicProtobufManager::new_protobuf_message("goby.acomms.protobuf.NetworkAck")->GetDescriptor());
    
}


goby::acomms::Bridge::~Bridge()
{
}


void goby::acomms::Bridge::loop()
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
}

void goby::acomms::Bridge::handle_queue_receive(const google::protobuf::Message& msg,
                                                QueueManager* from_queue)
{
    publish(msg, "QueueRx" + goby::util::as<std::string>(from_queue->modem_id()));
}

void goby::acomms::Bridge::handle_link_ack(const protobuf::ModemTransmission& ack_msg,
                                           const google::protobuf::Message& orig_msg,
                                           QueueManager* from_queue)
{
    // publish link ack
    publish(orig_msg, "QueueAckOrig" + goby::util::as<std::string>(from_queue->modem_id()));
    
    if(orig_msg.GetDescriptor()->full_name() == "goby.acomms.protobuf.NetworkAck")
    {
        glog.is(DEBUG1) && glog << "Not generating network ack from NetworkAck to avoid infinite proliferation of ACKS." << std::endl;
        return;
    }

    
    
    protobuf::NetworkAck ack;
    ack.set_ack_src(ack_msg.src());
    ack.set_message_dccl_id(DCCLCodec::get()->id(orig_msg.GetDescriptor()));


    protobuf::QueuedMessageMeta meta = from_queue->meta_from_msg(orig_msg);

    if(!network_ack_src_ids_.count(meta.src()))
    {
        glog.is(DEBUG1) && glog << "Not generating network ack for message from source ID: " << meta.src() << " as we weren't asked to do so." << std::endl;
        return;
    }
    
    ack.set_message_src(meta.src());
    ack.set_message_dest(meta.dest());
    ack.set_message_time(meta.time());

    if(ack_msg.src() == ack.message_src())
    {
        glog.is(DEBUG1) && glog << "Not generating network ack for single hop messages" << std::endl;
        return;
    }

    
    glog.is(VERBOSE) && glog << "Generated network ack: " << ack.DebugString() << "from: " << orig_msg.DebugString() << std::endl;
    
    r_manager_.handle_in(from_queue->meta_from_msg(ack), ack, from_queue->modem_id());
}


void goby::acomms::Bridge::handle_modem_receive(const goby::acomms::protobuf::ModemTransmission& message, QueueManager* in_queue)
{
    try
    {
        in_queue->handle_modem_receive(message);

        if(cfg_.forward_cacst())
        {
            for(int i = 0, n = message.ExtensionSize(micromodem::protobuf::receive_stat);
                i < n; ++i)
            {
                
                micromodem::protobuf::ReceiveStatistics cacst =
                    message.GetExtension(micromodem::protobuf::receive_stat, i);
                
                glog.is(VERBOSE) && glog << "Forwarding statistics message to topside: " << cacst << std::endl;
                r_manager_.handle_in(in_queue->meta_from_msg(cacst),
                                     cacst,
                                     in_queue->modem_id());
            }
        }
        
        if(cfg_.forward_ranging_reply() && message.HasExtension(micromodem::protobuf::ranging_reply))
        {
            micromodem::protobuf::RangingReply ranging =
                message.GetExtension(micromodem::protobuf::ranging_reply);
        
            glog.is(VERBOSE) && glog << "Forwarding ranging message to topside: " << ranging << std::endl;
            r_manager_.handle_in(in_queue->meta_from_msg(ranging),
                                 ranging,
                                 in_queue->modem_id());        
        }
    }
    catch(std::exception& e)
    {
        glog.is(WARN) && glog << "Failed to handle incoming message: " << e.what() << std::endl;
    }    
}


void goby::acomms::Bridge::handle_initiate_transmission(const protobuf::ModemTransmission& m,
                                                        int subnet)
{
    publish(m, "Tx" + goby::util::as<std::string>(cfg_.subnet(subnet).queue_cfg().modem_id()));
}


void goby::acomms::Bridge::handle_data_request(const protobuf::ModemTransmission& orig_msg,
                                               int subnet)
{
    protobuf::ModemTransmission msg = orig_msg;
    q_managers_[subnet]->handle_modem_data_request(&msg);
    publish(msg, "DataResponse" + goby::util::as<std::string>(cfg_.subnet(subnet).queue_cfg().modem_id()));
}
