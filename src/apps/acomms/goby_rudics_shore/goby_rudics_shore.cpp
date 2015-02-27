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


#include <boost/bimap.hpp>

#include "goby/common/time.h"
#include "goby/common/zeromq_application_base.h"
#include "goby/pb/protobuf_node.h"
#include "goby_rudics_shore_config.pb.h"
#include "goby/pb/rudics_packet.h"
#include "goby/util/linebasedcomms/tcp_server.h"

#include "goby_rudics_shore.h"

using namespace goby::common::logger;
using goby::glog;
using goby::common::goby_time;
using goby::util::TCPConnection;
using goby::acomms::protobuf::DirectIPMOPreHeader;
using goby::acomms::protobuf::DirectIPMOHeader;
using goby::acomms::protobuf::DirectIPMOPayload;

 
namespace goby
{
    namespace acomms
    {
        class GobyRudicsShore : public goby::common::ZeroMQApplicationBase,
                                public goby::pb::StaticProtobufNode
        {
        public:
            GobyRudicsShore(protobuf::GobyRudicsShoreConfig* cfg);
            ~GobyRudicsShore()
                {   }            
            

        private:
            void handle_request(const protobuf::MTDataRequest& request);
            void loop();

            void decode_mo(DirectIPMOPreHeader* pre_header, DirectIPMOHeader* header, DirectIPMOPayload* body,
                                     const std::string& data);            
            
        private:
            static goby::common::ZeroMQService zeromq_service_;
            protobuf::GobyRudicsShoreConfig& cfg_;

            goby::util::TCPServer rudics_server_;
            boost::asio::io_service sbd_io_;
            SBDServer mo_sbd_server_;
            
            typedef std::string Endpoint;
            typedef unsigned ModemId;
            // maps *destination* modem to *source* TCP endpoint
            boost::bimap<ModemId, Endpoint> clients_;
        };
    }
}

goby::common::ZeroMQService goby::acomms::GobyRudicsShore::zeromq_service_;


int main(int argc, char* argv[])
{
    goby::acomms::protobuf::GobyRudicsShoreConfig cfg;
    goby::run<goby::acomms::GobyRudicsShore>(argc, argv, &cfg);
}

goby::acomms::GobyRudicsShore::GobyRudicsShore(protobuf::GobyRudicsShoreConfig* cfg)
    : ZeroMQApplicationBase(&zeromq_service_, cfg),
      StaticProtobufNode(&zeromq_service_),
      cfg_(*cfg),
      rudics_server_(cfg_.rudics_server_port(), "\r"),
      mo_sbd_server_(sbd_io_, cfg_.mo_sbd_server_port())
{
    rudics_server_.start();
    
    // set up receiving requests    
    on_receipt<protobuf::MTDataRequest>(
        cfg_.reply_socket().socket_id(), &GobyRudicsShore::handle_request, this);

    // start zeromqservice
    common::protobuf::ZeroMQServiceConfig service_cfg;
    service_cfg.add_socket()->CopyFrom(cfg_.publish_socket());
    service_cfg.add_socket()->CopyFrom(cfg_.reply_socket());
    zeromq_service_.set_cfg(service_cfg);
}

void goby::acomms::GobyRudicsShore::loop()
{
    util::protobuf::Datagram msg;
    while(rudics_server_.readline(&msg))
    {
	glog.is(DEBUG1) && glog << "RUDICS received bytes: " << goby::util::hex_encode(msg.data()) << std::endl;

        std::string bytes;
        try
        {
            parse_rudics_packet(&bytes, msg.data());
            
            protobuf::ModemTransmission modem_msg;
            modem_msg.ParseFromString(bytes);

            glog.is(DEBUG1) && glog << "Received RUDICS message to: " << modem_msg.dest() << " from endpoint: " << msg.src() << std::endl;
            clients_.left.insert(std::make_pair(modem_msg.dest(), msg.src()));

            protobuf::MODataAsyncReceive async_rx_msg;
            async_rx_msg.set_modem_id(modem_msg.dest());
            *async_rx_msg.mutable_inbox() = modem_msg;
            send(async_rx_msg, cfg_.publish_socket().socket_id());
        }
        catch(RudicsPacketException& e)
        {
            glog.is(DEBUG1) && glog <<  warn << "Could not decode packet: " << e.what() << std::endl;
        }
    }    

    try
    {
        sbd_io_.poll();
    }
    catch(std::exception& e)
    {
        glog.is(DEBUG1) && glog <<  warn << "Could not handle SBD receive: " << e.what() << std::endl;
    }

    std::set<boost::shared_ptr<SBDConnection> >::iterator it = mo_sbd_server_.connections().begin(),
        end = mo_sbd_server_.connections().end();
    while(it != end)
    {
        const int timeout = 5;
        if((*it)->data_ready())
        {
            protobuf::ModemTransmission modem_msg;

            glog.is(DEBUG1) && glog << "SBD PreHeader: " << (*it)->pre_header().DebugString() << std::endl;
            glog.is(DEBUG1) && glog << "SBD Header: " << (*it)->header().DebugString()  << std::endl;
            glog.is(DEBUG1) && glog << "SBD Payload: " << (*it)->body().DebugString() << std::endl;

            modem_msg.ParseFromString((*it)->body().payload());
            
            glog.is(DEBUG1) && glog << "ModemTransmission: " << modem_msg.ShortDebugString() << std::endl;
            
            protobuf::MODataAsyncReceive async_rx_msg;
            async_rx_msg.set_modem_id(modem_msg.dest());
            *async_rx_msg.mutable_inbox() = modem_msg;
            send(async_rx_msg, cfg_.publish_socket().socket_id());            
            mo_sbd_server_.connections().erase(it++);
        }
        else if((*it)->connect_time() > 0 && (goby::common::goby_time<double>() > ((*it)->connect_time() + timeout)))
        {
            glog.is(DEBUG1) && glog << "Removing connection that has timed out." << std::endl;
            mo_sbd_server_.connections().erase(it++);
        }
        else
        {
            ++it;
        }
    }
}

void goby::acomms::GobyRudicsShore::handle_request(const protobuf::MTDataRequest& request)
{
    protobuf::MTDataResponse response;
    response.set_request_id(request.request_id());
    response.set_modem_id(request.modem_id());

    // check if we have a connection for this modem id
    typedef boost::bimap<ModemId, Endpoint>::left_map::iterator It;

    It map_it = clients_.left.find(request.modem_id());
    if(map_it == clients_.left.end())
    {
        response.set_mobile_node_connected(false);
    }
    else
    {
        // check that we're still connected   
        const std::set< boost::shared_ptr<TCPConnection> >& connections =
            rudics_server_.connections();

        bool still_connected = false;
        for(std::set< boost::shared_ptr<TCPConnection> >::const_iterator it = connections.begin(), end = connections.end(); it != end; ++it)
        {
            if(map_it->second == (*it)->remote_endpoint())
                still_connected = true;    
        }

        response.set_mobile_node_connected(still_connected);
        if(still_connected)
        {
            util::protobuf::Datagram tcp_msg;
            tcp_msg.set_dest(map_it->second);
            for(int i = 0, n = request.outbox_size(); i < n; ++i)
            {
                std::string bytes;
                request.outbox(i).SerializeToString(&bytes);
            
                // frame message
                std::string rudics_packet;
                serialize_rudics_packet(bytes, &rudics_packet);

                rudics_server_.write(rudics_packet);
            }
        }
        else
        {
            clients_.left.erase(map_it);
        }
    }
    
    send(response, cfg_.reply_socket().socket_id());
    response.Clear();
}
