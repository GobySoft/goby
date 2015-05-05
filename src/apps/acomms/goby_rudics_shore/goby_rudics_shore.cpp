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
using goby::acomms::protobuf::DirectIPMTHeader;
using goby::acomms::protobuf::DirectIPMTPayload;
 
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
            std::string create_sbd_mt_data_message(const std::string& payload, const std::string& imei);
            void send_sbd_mt(const std::string& bytes, const std::string& imei);

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

            enum { RATE_RUDICS = 1, RATE_SBD = 0 };    

            typedef std::string IMEI;
            std::map<ModemId, IMEI> modem_id_to_imei_;
            
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


    for(int i = 0, n = cfg_.modem_id_to_imei_size(); i < n; ++i)
        modem_id_to_imei_[cfg_.modem_id_to_imei(i).modem_id()] = cfg_.modem_id_to_imei(i).imei();
    
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
            async_rx_msg.set_modem_id(modem_msg.src());
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
        if((*it)->message().data_ready())
        {
            protobuf::ModemTransmission modem_msg;
            
            glog.is(DEBUG1) && glog << "Rx SBD PreHeader: " << (*it)->message().pre_header().DebugString() << std::endl;
            glog.is(DEBUG1) && glog << "Rx SBD Header: " << (*it)->message().header().DebugString()  << std::endl;
            glog.is(DEBUG1) && glog << "Rx SBD Payload: " << (*it)->message().body().DebugString() << std::endl;

            std::string bytes;
            try
            {
                parse_rudics_packet(&bytes, (*it)->message().body().payload());

                modem_msg.ParseFromString(bytes);
            
                glog.is(DEBUG1) && glog << "Rx ModemTransmission: " << modem_msg.ShortDebugString() << std::endl;
            
                protobuf::MODataAsyncReceive async_rx_msg;
                async_rx_msg.set_modem_id(modem_msg.dest());
                *async_rx_msg.mutable_inbox() = modem_msg;
                send(async_rx_msg, cfg_.publish_socket().socket_id());            
                mo_sbd_server_.connections().erase(it++);
            }
            catch(RudicsPacketException& e)
            {
                glog.is(DEBUG1) && glog <<  warn << "Could not decode packet: " << e.what() << std::endl;
            }
        }
        else if((*it)->connect_time() > 0 && (goby::common::goby_time<double>() > ((*it)->connect_time() + timeout)))
        {
	  glog.is(DEBUG1) && glog << "Removing connection that has timed out:" << (*it)->socket().remote_endpoint() << std::endl;
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

    // send any SBD packets that weren't send
    if(!response.mobile_node_connected())
    {
        for(int i = 0, n = request.outbox_size(); i < n; ++i)
        {
            if(request.outbox(i).rate() == RATE_SBD)
            {
                std::string bytes;
                request.outbox(i).SerializeToString(&bytes);

                std::string rudics_packet;
                serialize_rudics_packet(bytes, &rudics_packet);

                if(modem_id_to_imei_.count(request.outbox(i).dest()))
                    send_sbd_mt(rudics_packet, modem_id_to_imei_[request.outbox(i).dest()]);
                else
                    glog.is(WARN) && glog << "No IMEI configured for destination address " << request.outbox(i).dest() << " so unabled to send SBD message." << std::endl;
            }
            
        }

    }
    
    
    send(response, cfg_.reply_socket().socket_id());
    response.Clear();
}


void goby::acomms::GobyRudicsShore::send_sbd_mt(const std::string& bytes, const std::string& imei)
{
    try
    {
        using boost::asio::ip::tcp;

        boost::asio::io_service io_service;

        tcp::resolver resolver(io_service);
        tcp::resolver::query query(cfg_.mt_sbd_server_address(), goby::util::as<std::string>(cfg_.mt_sbd_server_port()));
        tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
        tcp::resolver::iterator end;

        tcp::socket socket(io_service);
        boost::system::error_code error = boost::asio::error::host_not_found;
        while (error && endpoint_iterator != end)
        {
            socket.close();
            socket.connect(*endpoint_iterator++, error);
        }
        if (error)
            throw boost::system::system_error(error);

        boost::asio::write(socket, boost::asio::buffer(create_sbd_mt_data_message(bytes, imei)));

        SBDMTConfirmationMessageReader message(socket);
        boost::asio::async_read(socket,
                                boost::asio::buffer(message.data()),
                                boost::asio::transfer_at_least(SBDMessageReader::PRE_HEADER_SIZE),
                                boost::bind(&SBDMessageReader::pre_header_handler, &message, _1, _2));

        double start_time = goby::common::goby_time<double>();
        const int timeout = 5;
        
        while(!message.data_ready() && (start_time + timeout > goby::common::goby_time<double>()))
            io_service.poll();

        if(message.data_ready())
        {
            glog.is(DEBUG1) && glog << "Tx SBD Confirmation: " << message.confirm().DebugString() << std::endl;
        }
        else
        {
            glog.is(WARN) && glog << "Timeout waiting for confirmation message from DirectIP server" << std::endl;
        }
    }
    catch (std::exception& e)
    {
        glog.is(WARN) && glog << "Could not sent MT SBD message: " << e.what() << std::endl;
    }
}

std::string goby::acomms::GobyRudicsShore::create_sbd_mt_data_message(const std::string& bytes, const std::string& imei)
{
    enum { PRE_HEADER_SIZE = 3, BITS_PER_BYTE = 8, IEI_SIZE = 3, HEADER_SIZE = 21 };

    enum { IEI_MT_HEADER = 0x41, IEI_MT_PAYLOAD = 0x42 };
    
    
    static int i = 0;
    DirectIPMTHeader header;
    header.set_iei(IEI_MT_HEADER);
    header.set_length(HEADER_SIZE);
    header.set_client_id(i++);
    header.set_imei(imei);
    
    enum { DISP_FLAG_FLUSH_MT_QUEUE = 0x01,
	   DISP_FLAG_SEND_RING_ALERT_NO_MTM = 0x02,
	   DISP_FLAG_UPDATE_SSD_LOCATION = 0x08, 
	   DISP_FLAG_HIGH_PRIORITY_MESSAGE = 0x10, 
	   DISP_FLAG_ASSIGN_MTMSN = 0x20 };

    header.set_disposition_flags(DISP_FLAG_FLUSH_MT_QUEUE);

    std::string header_bytes(IEI_SIZE+HEADER_SIZE, '\0');

    std::string::size_type pos = 0;
    enum  { HEADER_IEI = 1, HEADER_LENGTH = 2, HEADER_CLIENT_ID = 3, HEADER_IMEI = 4, HEADER_DISPOSITION_FLAGS = 5 };
    
    for(int field = HEADER_IEI; field <= HEADER_DISPOSITION_FLAGS; ++field)
    {
        switch(field)
        {
            case HEADER_IEI:
                header_bytes[pos++] = header.iei() & 0xff;
                break;
                
            case HEADER_LENGTH:
                header_bytes[pos++] = (header.length() >> BITS_PER_BYTE) & 0xff;
                header_bytes[pos++] = (header.length()) & 0xff;
                break;
        
            case HEADER_CLIENT_ID:
                header_bytes[pos++] = (header.client_id() >> 3*BITS_PER_BYTE) & 0xff;
                header_bytes[pos++] = (header.client_id() >> 2*BITS_PER_BYTE) & 0xff;
                header_bytes[pos++] = (header.client_id() >> BITS_PER_BYTE) & 0xff;
                header_bytes[pos++] = (header.client_id()) & 0xff;
                break;

            case HEADER_IMEI:
                header_bytes.replace(pos, 15, header.imei());
                pos += 15;
                break;

            case HEADER_DISPOSITION_FLAGS:
                header_bytes[pos++] = (header.disposition_flags() >> BITS_PER_BYTE) & 0xff;
                header_bytes[pos++] = (header.disposition_flags()) & 0xff;
                break;
        }
        
    }
    
    
    DirectIPMTPayload payload;
    payload.set_iei(IEI_MT_PAYLOAD);
    payload.set_length(bytes.size());
    payload.set_payload(bytes);

    std::string payload_bytes(IEI_SIZE + bytes.size(), '\0');    
    payload_bytes[0] = payload.iei();
    payload_bytes[1] = (payload.length() >> BITS_PER_BYTE) & 0xff;
    payload_bytes[2] = (payload.length()) & 0xff;
    payload_bytes.replace(3, payload.payload().size(),payload.payload());

    // Protocol Revision Number (1 byte) == 1
    // Overall Message Length (2 bytes)
    int overall_length = header_bytes.size() + payload_bytes.size();
    std::string pre_header_bytes(PRE_HEADER_SIZE, '\0');
    pre_header_bytes[0] = 1;
    pre_header_bytes[1] = (overall_length >> BITS_PER_BYTE) & 0xff;
    pre_header_bytes[2] = (overall_length) & 0xff;

    
    glog.is(DEBUG1) && glog << "Tx SBD PreHeader: " << goby::util::hex_encode(pre_header_bytes) << std::endl;
    glog.is(DEBUG1) && glog << "Tx SBD Header: " << header.DebugString()  << std::endl;
    glog.is(DEBUG1) && glog << "Tx SBD Payload: " << payload.DebugString() << std::endl;
    
    return pre_header_bytes + header_bytes + payload_bytes;
}
