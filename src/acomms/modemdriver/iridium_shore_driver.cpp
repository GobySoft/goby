// Copyright 2009-2018 Toby Schneider (http://gobysoft.org/index.wt/people/toby)
//                     GobySoft, LLC (2013-)
//                     Massachusetts Institute of Technology (2007-2014)
//                     Community contributors (see AUTHORS file)
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

#include <boost/bimap.hpp>

#include "goby/acomms/modemdriver/driver_exception.h"
#include "goby/acomms/modemdriver/rudics_packet.h"
#include "goby/common/io_service.h"
#include "goby/common/logger.h"
#include "goby/common/time.h"
#include "goby/util/binary.h"

#include "iridium_shore_driver.h"
#include "iridium_shore_sbd.h"

using namespace goby::common::logger;
using goby::glog;
using goby::acomms::protobuf::DirectIPMOHeader;
using goby::acomms::protobuf::DirectIPMOPayload;
using goby::acomms::protobuf::DirectIPMOPreHeader;
using goby::acomms::protobuf::DirectIPMTHeader;
using goby::acomms::protobuf::DirectIPMTPayload;
using goby::common::goby_time;
using goby::util::TCPConnection;

goby::acomms::IridiumShoreDriver::IridiumShoreDriver() : next_frame_(0) { init_iridium_dccl(); }

goby::acomms::IridiumShoreDriver::~IridiumShoreDriver() {}

void goby::acomms::IridiumShoreDriver::startup(const protobuf::DriverConfig& cfg)
{
    driver_cfg_ = cfg;

    glog.is(DEBUG1) && glog << group(glog_out_group())
                            << "Goby Shore Iridium RUDICS/SBD driver starting up." << std::endl;

    rudics_mac_msg_.set_src(driver_cfg_.modem_id());
    rudics_mac_msg_.set_type(goby::acomms::protobuf::ModemTransmission::DATA);
    rudics_mac_msg_.set_rate(RATE_RUDICS);

    rudics_server_.reset(new RUDICSServer(
        rudics_io_, driver_cfg_.GetExtension(IridiumShoreDriverConfig::rudics_server_port)));
    mo_sbd_server_.reset(new SBDServer(
        sbd_io_, driver_cfg_.GetExtension(IridiumShoreDriverConfig::mo_sbd_server_port)));

    rudics_server_->connect_signal.connect(
        boost::bind(&IridiumShoreDriver::rudics_connect, this, _1));

    for (int i = 0, n = driver_cfg_.ExtensionSize(IridiumShoreDriverConfig::modem_id_to_imei);
         i < n; ++i)
        modem_id_to_imei_[driver_cfg_.GetExtension(IridiumShoreDriverConfig::modem_id_to_imei, i)
                              .modem_id()] =
            driver_cfg_.GetExtension(IridiumShoreDriverConfig::modem_id_to_imei, i).imei();
}

void goby::acomms::IridiumShoreDriver::shutdown() {}

void goby::acomms::IridiumShoreDriver::handle_initiate_transmission(
    const protobuf::ModemTransmission& orig_msg)
{
    process_transmission(orig_msg);
}

void goby::acomms::IridiumShoreDriver::process_transmission(protobuf::ModemTransmission msg)
{
    signal_modify_transmission(&msg);

    if (!msg.has_frame_start())
        msg.set_frame_start(next_frame_);

    // set the frame size, if not set or if it exceeds the max configured
    if (!msg.has_max_frame_bytes() ||
        msg.max_frame_bytes() > driver_cfg_.GetExtension(IridiumDriverConfig::max_frame_size))
        msg.set_max_frame_bytes(driver_cfg_.GetExtension(IridiumDriverConfig::max_frame_size));

    signal_data_request(&msg);

    next_frame_ += msg.frame_size();

    if (!(msg.frame_size() == 0 || msg.frame(0).empty()))
    {
        send(msg);
    }
}

void goby::acomms::IridiumShoreDriver::do_work()
{
    //   if(glog.is(DEBUG1))
    //    display_state_cfg(&glog);
    double now = goby_time<double>();

    for (std::map<ModemId, RemoteNode>::iterator it = remote_.begin(), end = remote_.end();
         it != end; ++it)
    {
        RemoteNode& remote = it->second;
        boost::shared_ptr<OnCallBase> on_call_base = remote.on_call;
        ModemId id = it->first;

        // if we're on either type of call, see if we need to send the "bye" message or hangup
        if (on_call_base)
        {
            // if we're on a call, keep pushing data at the target rate
            const double send_wait =
                on_call_base->last_bytes_sent() /
                (driver_cfg_.GetExtension(IridiumDriverConfig::target_bit_rate) /
                 static_cast<double>(BITS_IN_BYTE));

            if (now > (on_call_base->last_tx_time() + send_wait))
            {
                if (!on_call_base->bye_sent())
                {
                    rudics_mac_msg_.set_dest(it->first);
                    process_transmission(rudics_mac_msg_);
                }
            }

            if (!on_call_base->bye_sent() &&
                now > (on_call_base->last_tx_time() +
                       driver_cfg_.GetExtension(IridiumDriverConfig::handshake_hangup_seconds)))
            {
                glog.is(DEBUG1) && glog << "Sending bye" << std::endl;
                rudics_send("bye\r", id);
                on_call_base->set_bye_sent(true);
            }

            if ((on_call_base->bye_received() && on_call_base->bye_sent()) ||
                (now > (on_call_base->last_rx_tx_time() +
                        driver_cfg_.GetExtension(IridiumDriverConfig::hangup_seconds_after_empty))))
            {
                glog.is(DEBUG1) && glog << "Hanging up by disconnecting" << std::endl;
                typedef boost::bimap<
                    ModemId, boost::shared_ptr<RUDICSConnection> >::left_map::iterator LeftIt;
                LeftIt client_it = clients_.left.find(id);
                if (client_it != clients_.left.end())
                    rudics_server_->disconnect(client_it->second);
                else
                    glog.is(WARN) && glog << "Failed to find connection from ModemId " << id
                                          << std::endl;
                remote_[id].on_call.reset();
            }
        }
    }

    rudics_io_.poll();
    receive_sbd_mo();
}

void goby::acomms::IridiumShoreDriver::receive(const protobuf::ModemTransmission& msg)
{
    glog.is(DEBUG2) && glog << group(glog_in_group()) << msg.DebugString() << std::endl;

    if (msg.type() == protobuf::ModemTransmission::DATA && msg.ack_requested() &&
        msg.dest() == driver_cfg_.modem_id())
    {
        // make any acks
        protobuf::ModemTransmission ack;
        ack.set_type(goby::acomms::protobuf::ModemTransmission::ACK);
        ack.set_src(msg.dest());
        ack.set_dest(msg.src());
        ack.set_rate(msg.rate());
        for (int i = msg.frame_start(), n = msg.frame_size() + msg.frame_start(); i < n; ++i)
            ack.add_acked_frame(i);
        send(ack);
    }

    signal_receive(msg);
}

void goby::acomms::IridiumShoreDriver::send(const protobuf::ModemTransmission& msg)
{
    glog.is(DEBUG2) && glog << group(glog_out_group()) << msg.DebugString() << std::endl;

    RemoteNode& remote = remote_[msg.dest()];

    if (msg.rate() == RATE_RUDICS || remote.on_call)
    {
        std::string bytes;
        serialize_iridium_modem_message(&bytes, msg);

        // frame message
        std::string rudics_packet;
        serialize_rudics_packet(bytes, &rudics_packet);
        rudics_send(rudics_packet, msg.dest());
        boost::shared_ptr<OnCallBase> on_call_base = remote.on_call;
        on_call_base->set_last_tx_time(goby_time<double>());
        on_call_base->set_last_bytes_sent(rudics_packet.size());
    }
    else if (msg.rate() == RATE_SBD)
    {
        std::string bytes;
        serialize_iridium_modem_message(&bytes, msg);

        std::string sbd_packet;
        serialize_rudics_packet(bytes, &sbd_packet);

        if (modem_id_to_imei_.count(msg.dest()))
            send_sbd_mt(sbd_packet, modem_id_to_imei_[msg.dest()]);
        else
            glog.is(WARN) && glog << "No IMEI configured for destination address " << msg.dest()
                                  << " so unabled to send SBD message." << std::endl;
    }
}

void goby::acomms::IridiumShoreDriver::rudics_send(const std::string& data,
                                                   goby::acomms::IridiumShoreDriver::ModemId id)
{
    typedef boost::bimap<ModemId, boost::shared_ptr<RUDICSConnection> >::left_map::iterator LeftIt;
    LeftIt client_it = clients_.left.find(id);
    if (client_it != clients_.left.end())
    {
        glog.is(DEBUG1) && glog << "RUDICS sending bytes: " << goby::util::hex_encode(data)
                                << std::endl;
        client_it->second->write_start(data);
    }
    else
    {
        glog.is(WARN) && glog << "Failed to find connection from ModemId " << id << std::endl;
    }
}

void goby::acomms::IridiumShoreDriver::rudics_connect(
    boost::shared_ptr<RUDICSConnection> connection)
{
    connection->line_signal.connect(boost::bind(&IridiumShoreDriver::rudics_line, this, _1, _2));
    connection->disconnect_signal.connect(
        boost::bind(&IridiumShoreDriver::rudics_disconnect, this, _1));
}

void goby::acomms::IridiumShoreDriver::rudics_disconnect(
    boost::shared_ptr<RUDICSConnection> connection)
{
    typedef boost::bimap<ModemId, boost::shared_ptr<RUDICSConnection> >::right_map::iterator
        RightIt;

    RightIt client_it = clients_.right.find(connection);
    if (client_it != clients_.right.end())
    {
        ModemId id = client_it->second;
        remote_[id].on_call.reset();
        clients_.right.erase(client_it);
        glog.is(DEBUG1) && glog << "Disconnecting client for modem id: " << id << "; "
                                << clients_.size() << " clients remaining." << std::endl;
    }
    else
    {
        glog.is(WARN) &&
            glog << "Disconnection received from connection we do not have in the clients_ map: "
                 << connection->remote_endpoint_str() << std::endl;
    }
}

void goby::acomms::IridiumShoreDriver::rudics_line(const std::string& data,
                                                   boost::shared_ptr<RUDICSConnection> connection)
{
    glog.is(DEBUG1) && glog << "RUDICS received bytes: " << goby::util::hex_encode(data)
                            << std::endl;

    try
    {
        std::string decoded_line;

        if (data == "goby\r" ||
            data == "\0goby\r") // sometimes Iridium adds a 0x00 to the start of transmission
        {
            glog.is(DEBUG1) && glog << "Detected start of Goby RUDICS connection from "
                                    << connection->remote_endpoint_str() << std::endl;
        }
        else if (data == "bye\r")
        {
            typedef boost::bimap<ModemId, boost::shared_ptr<RUDICSConnection> >::right_map::iterator
                RightIt;

            RightIt client_it = clients_.right.find(connection);
            if (client_it != clients_.right.end())
            {
                ModemId id = client_it->second;
                glog.is(DEBUG1) && glog << "Detected bye from " << connection->remote_endpoint_str()
                                        << " ID: " << id << std::endl;
                remote_[id].on_call->set_bye_received(true);
            }
            else
            {
                glog.is(WARN) &&
                    glog << "Bye detected from connection we do not have in the clients_ map: "
                         << connection->remote_endpoint_str() << std::endl;
            }
        }
        else
        {
            parse_rudics_packet(&decoded_line, data);

            protobuf::ModemTransmission modem_msg;
            parse_iridium_modem_message(decoded_line, &modem_msg);

            glog.is(DEBUG1) && glog << "Received RUDICS message from: " << modem_msg.src()
                                    << " to: " << modem_msg.dest()
                                    << " from endpoint: " << connection->remote_endpoint_str()
                                    << std::endl;
            if (!clients_.left.count(modem_msg.src()))
            {
                clients_.left.insert(std::make_pair(modem_msg.src(), connection));
                remote_[modem_msg.src()].on_call.reset(new OnCallBase);
            }

            remote_[modem_msg.src()].on_call->set_last_rx_time(goby_time<double>());

            receive(modem_msg);
        }
    }
    catch (RudicsPacketException& e)
    {
        glog.is(DEBUG1) && glog << warn << "Could not decode packet: " << e.what() << std::endl;
        connection->add_packet_failure();
    }
}

void goby::acomms::IridiumShoreDriver::receive_sbd_mo()
{
    try
    {
        sbd_io_.poll();
    }
    catch (std::exception& e)
    {
        glog.is(DEBUG1) && glog << warn << "Could not handle SBD receive: " << e.what()
                                << std::endl;
    }

    std::set<boost::shared_ptr<SBDConnection> >::iterator it =
                                                              mo_sbd_server_->connections().begin(),
                                                          end = mo_sbd_server_->connections().end();
    while (it != end)
    {
        const int timeout = 5;
        if ((*it)->message().data_ready())
        {
            protobuf::ModemTransmission modem_msg;

            glog.is(DEBUG1) && glog << "Rx SBD PreHeader: "
                                    << (*it)->message().pre_header().DebugString() << std::endl;
            glog.is(DEBUG1) && glog << "Rx SBD Header: " << (*it)->message().header().DebugString()
                                    << std::endl;
            glog.is(DEBUG1) && glog << "Rx SBD Payload: " << (*it)->message().body().DebugString()
                                    << std::endl;

            std::string bytes;
            try
            {
                parse_rudics_packet(&bytes, (*it)->message().body().payload());
                parse_iridium_modem_message(bytes, &modem_msg);

                glog.is(DEBUG1) && glog << "Rx SBD ModemTransmission: "
                                        << modem_msg.ShortDebugString() << std::endl;

                receive(modem_msg);
            }
            catch (RudicsPacketException& e)
            {
                glog.is(DEBUG1) && glog << warn << "Could not decode SBD packet: " << e.what()
                                        << std::endl;
            }
            mo_sbd_server_->connections().erase(it++);
        }
        else if ((*it)->connect_time() > 0 &&
                 (goby::common::goby_time<double>() > ((*it)->connect_time() + timeout)))
        {
            glog.is(DEBUG1) && glog << "Removing SBD connection that has timed out:"
                                    << (*it)->remote_endpoint_str() << std::endl;
            mo_sbd_server_->connections().erase(it++);
        }
        else
        {
            ++it;
        }
    }
}

void goby::acomms::IridiumShoreDriver::send_sbd_mt(const std::string& bytes,
                                                   const std::string& imei)
{
    try
    {
        using boost::asio::ip::tcp;

        boost::asio::io_service io_service;

        tcp::resolver resolver(io_service);
        tcp::resolver::query query(
            driver_cfg_.GetExtension(IridiumShoreDriverConfig::mt_sbd_server_address),
            goby::util::as<std::string>(
                driver_cfg_.GetExtension(IridiumShoreDriverConfig::mt_sbd_server_port)));
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
        boost::asio::async_read(
            socket, boost::asio::buffer(message.data()),
            boost::asio::transfer_at_least(SBDMessageReader::PRE_HEADER_SIZE),
            boost::bind(&SBDMessageReader::pre_header_handler, &message, _1, _2));

        double start_time = goby::common::goby_time<double>();
        const int timeout = 5;

        while (!message.data_ready() && (start_time + timeout > goby::common::goby_time<double>()))
            io_service.poll();

        if (message.data_ready())
        {
            glog.is(DEBUG1) && glog << "Tx SBD Confirmation: " << message.confirm().DebugString()
                                    << std::endl;
        }
        else
        {
            glog.is(WARN) && glog << "Timeout waiting for confirmation message from DirectIP server"
                                  << std::endl;
        }
    }
    catch (std::exception& e)
    {
        glog.is(WARN) && glog << "Could not sent MT SBD message: " << e.what() << std::endl;
    }
}

std::string goby::acomms::IridiumShoreDriver::create_sbd_mt_data_message(const std::string& bytes,
                                                                         const std::string& imei)
{
    enum
    {
        PRE_HEADER_SIZE = 3,
        BITS_PER_BYTE = 8,
        IEI_SIZE = 3,
        HEADER_SIZE = 21
    };

    enum
    {
        IEI_MT_HEADER = 0x41,
        IEI_MT_PAYLOAD = 0x42
    };

    static int i = 0;
    DirectIPMTHeader header;
    header.set_iei(IEI_MT_HEADER);
    header.set_length(HEADER_SIZE);
    header.set_client_id(i++);
    header.set_imei(imei);

    enum
    {
        DISP_FLAG_FLUSH_MT_QUEUE = 0x01,
        DISP_FLAG_SEND_RING_ALERT_NO_MTM = 0x02,
        DISP_FLAG_UPDATE_SSD_LOCATION = 0x08,
        DISP_FLAG_HIGH_PRIORITY_MESSAGE = 0x10,
        DISP_FLAG_ASSIGN_MTMSN = 0x20
    };

    header.set_disposition_flags(DISP_FLAG_FLUSH_MT_QUEUE);

    std::string header_bytes(IEI_SIZE + HEADER_SIZE, '\0');

    std::string::size_type pos = 0;
    enum
    {
        HEADER_IEI = 1,
        HEADER_LENGTH = 2,
        HEADER_CLIENT_ID = 3,
        HEADER_IMEI = 4,
        HEADER_DISPOSITION_FLAGS = 5
    };

    for (int field = HEADER_IEI; field <= HEADER_DISPOSITION_FLAGS; ++field)
    {
        switch (field)
        {
            case HEADER_IEI: header_bytes[pos++] = header.iei() & 0xff; break;

            case HEADER_LENGTH:
                header_bytes[pos++] = (header.length() >> BITS_PER_BYTE) & 0xff;
                header_bytes[pos++] = (header.length()) & 0xff;
                break;

            case HEADER_CLIENT_ID:
                header_bytes[pos++] = (header.client_id() >> 3 * BITS_PER_BYTE) & 0xff;
                header_bytes[pos++] = (header.client_id() >> 2 * BITS_PER_BYTE) & 0xff;
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
    payload_bytes.replace(3, payload.payload().size(), payload.payload());

    // Protocol Revision Number (1 byte) == 1
    // Overall Message Length (2 bytes)
    int overall_length = header_bytes.size() + payload_bytes.size();
    std::string pre_header_bytes(PRE_HEADER_SIZE, '\0');
    pre_header_bytes[0] = 1;
    pre_header_bytes[1] = (overall_length >> BITS_PER_BYTE) & 0xff;
    pre_header_bytes[2] = (overall_length)&0xff;

    glog.is(DEBUG1) && glog << "Tx SBD PreHeader: " << goby::util::hex_encode(pre_header_bytes)
                            << std::endl;
    glog.is(DEBUG1) && glog << "Tx SBD Header: " << header.DebugString() << std::endl;
    glog.is(DEBUG1) && glog << "Tx SBD Payload: " << payload.DebugString() << std::endl;

    return pre_header_bytes + header_bytes + payload_bytes;
}
