// Copyright 2009-2013 Toby Schneider (https://launchpad.net/~tes)
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

#include <boost/bimap.hpp>

#include "goby/common/time.h"
#include "goby/common/zeromq_application_base.h"
#include "goby/pb/protobuf_node.h"
#include "goby_rudics_shore_config.pb.h"
#include "goby/acomms/protobuf/rudics_shore.pb.h"
#include "goby/acomms/modemdriver/rudics_packet.h"
#include "goby/util/linebasedcomms/tcp_server.h"

using namespace goby::common::logger;
using goby::glog;
using goby::common::goby_time;
using goby::util::TCPConnection;


namespace goby
{
    namespace acomms
    {
        class GobyRudicsShore : public goby::common::ZeroMQApplicationBase,
                                public goby::pb::StaticProtobufNode
        {
        public:
            GobyRudicsShore();
            ~GobyRudicsShore()
                {   }            
            

        private:
            void handle_request(const protobuf::MTDataRequest& request);
            void loop();

        private:
            static goby::common::ZeroMQService zeromq_service_;
            static protobuf::GobyRudicsShoreConfig cfg_;

            goby::util::TCPServer rudics_server_;

            typedef std::string Endpoint;
            typedef unsigned ModemId;
            boost::bimap<ModemId, Endpoint> clients_;
            
                         
        };
    }
}

goby::common::ZeroMQService goby::acomms::GobyRudicsShore::zeromq_service_;
goby::acomms::protobuf::GobyRudicsShoreConfig goby::acomms::GobyRudicsShore::cfg_;


int main(int argc, char* argv[])
{
    goby::run<goby::acomms::GobyRudicsShore>(argc, argv);
}

goby::acomms::GobyRudicsShore::GobyRudicsShore()
    : ZeroMQApplicationBase(&zeromq_service_, &cfg_),
      StaticProtobufNode(&zeromq_service_),
      rudics_server_(cfg_.rudics_server_port(), "\r")
{
    rudics_server_.start();

    // set up receiving requests    
    on_receipt<protobuf::MTDataRequest>(
        cfg_.reply_socket().socket_id(), &GobyRudicsShore::handle_request, this);

    // start zeromqservice
    common::protobuf::ZeroMQServiceConfig service_cfg;
    service_cfg.add_socket()->CopyFrom(cfg_.reply_socket());
    zeromq_service_.set_cfg(service_cfg);
}

void goby::acomms::GobyRudicsShore::loop()
{
    util::protobuf::Datagram msg;
    while(rudics_server_.readline(&msg))
    {
        std::string bytes;
        try
        {
            parse_rudics_packet(&bytes, msg.data());
            
            protobuf::ModemTransmission modem_msg;
            modem_msg.ParseFromString(bytes);

            glog.is(DEBUG1) && glog << "Received message from: " << modem_msg.src() << " from endpoint: " << msg.src() << std::endl;
        }
        catch(RudicsPacketException& e)
        {
            glog.is(DEBUG1) && glog <<  warn << "Could not decode packet: " << e.what() << std::endl;
        }
    }    

    
    const std::set< boost::shared_ptr<TCPConnection> >& connections =
        rudics_server_.connections();

    int j = 0;
    for(std::set< boost::shared_ptr<TCPConnection> >::const_iterator it = connections.begin(), end = connections.end(); it != end; ++it)
    {
        glog.is(DEBUG1) && glog << "Remote " << ++j << ": " << (*it)->remote_endpoint() << std::endl;
    }
}

void goby::acomms::GobyRudicsShore::handle_request(const protobuf::MTDataRequest& request)
{
    protobuf::MTDataResponse response;
    
    send(response, cfg_.reply_socket().socket_id());
}

