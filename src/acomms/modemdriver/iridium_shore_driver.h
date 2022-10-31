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

#ifndef IridiumShoreDriver20140508H
#define IridiumShoreDriver20140508H

#include <boost/bimap.hpp>
#include <boost/circular_buffer.hpp>

#include "goby/acomms/modemdriver/driver_base.h"
#include "goby/acomms/modemdriver/iridium_driver_common.h"
#include "goby/acomms/modemdriver/iridium_shore_rudics.h"
#include "goby/acomms/modemdriver/iridium_shore_sbd.h"
#include "goby/acomms/protobuf/iridium_driver.pb.h"
#include "goby/acomms/protobuf/iridium_sbd_directip.pb.h"
#include "goby/acomms/protobuf/iridium_shore_driver.pb.h"
#include "goby/common/time.h"
#include "goby/common/io_service.h"

namespace goby
{
namespace acomms
{
class IridiumShoreDriver : public ModemDriverBase
{
  public:
    IridiumShoreDriver();
    ~IridiumShoreDriver();
    void startup(const protobuf::DriverConfig& cfg);

    void modem_init();

    void shutdown();
    void do_work();

    void handle_initiate_transmission(const protobuf::ModemTransmission& m);
    void process_transmission(protobuf::ModemTransmission msg);

  private:
    typedef unsigned ModemId;

    void receive(const protobuf::ModemTransmission& msg);
    void send(const protobuf::ModemTransmission& msg);

    void decode_mo(protobuf::DirectIPMOPreHeader* pre_header, protobuf::DirectIPMOHeader* header,
                   protobuf::DirectIPMOPayload* body, const std::string& data);
    std::string create_sbd_mt_data_message(const std::string& payload, const std::string& imei);
    void receive_sbd_mo();
    void send_sbd_mt(const std::string& bytes, const std::string& imei);

    void rudics_send(const std::string& data, ModemId id);
    void rudics_disconnect(boost::shared_ptr<RUDICSConnection> connection);
    void rudics_line(const std::string& line, boost::shared_ptr<RUDICSConnection> connection);
    void rudics_connect(boost::shared_ptr<RUDICSConnection> connection);

  private:
    protobuf::DriverConfig driver_cfg_;

    protobuf::ModemTransmission rudics_mac_msg_;

    goby::uint32 next_frame_;

    struct RemoteNode
    {
        enum
        {
            DATA_BUFFER_CAPACITY = 30
        };
        RemoteNode() : data_out(DATA_BUFFER_CAPACITY) {}

        boost::shared_ptr<OnCallBase> on_call;
        boost::circular_buffer<protobuf::ModemTransmission> data_out;
    };

    std::map<ModemId, RemoteNode> remote_;

    boost::asio::io_service rudics_io_;
    boost::shared_ptr<RUDICSServer> rudics_server_;
    boost::asio::io_service sbd_io_;
    boost::shared_ptr<SBDServer> mo_sbd_server_;

    // maps remote modem to connection
    boost::bimap<ModemId, boost::shared_ptr<RUDICSConnection> > clients_;

    typedef std::string IMEI;
    std::map<ModemId, IMEI> modem_id_to_imei_;
};
} // namespace acomms
} // namespace goby
#endif
