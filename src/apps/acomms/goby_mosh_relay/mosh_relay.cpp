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

#include <boost/asio.hpp>
#include <boost/array.hpp>

#include "goby/common/logger.h"

#include "goby/pb/application.h"

#include "mosh_relay_config.pb.h"

using namespace goby::common::logger;

using boost::asio::ip::udp;

namespace goby
{
    namespace acomms
    {
        class MoshRelay : public goby::pb::Application
        {
        public:
            MoshRelay(protobuf::MoshRelayConfig* cfg);
            ~MoshRelay();

        private:
            void loop();
            void start_udp_receive();
            void handle_udp_receive(const boost::system::error_code& error, std::size_t);

        private:
            protobuf::MoshRelayConfig& cfg_;
            boost::asio::io_service io_service_;
            udp::socket socket_;

            enum { MOSH_UDP_PAYLOAD_SIZE = 60 };
            udp::endpoint remote_endpoint_;
            boost::array<char, MOSH_UDP_PAYLOAD_SIZE> recv_buffer_;
        };
    }
}


int main(int argc, char* argv[])
{
    goby::acomms::protobuf::MoshRelayConfig cfg;
    goby::run<goby::acomms::MoshRelay>(argc, argv, &cfg);
}


using goby::glog;

goby::acomms::MoshRelay::MoshRelay(protobuf::MoshRelayConfig* cfg)
    : goby::pb::Application(cfg),
      cfg_(*cfg),
      socket_(io_service_, udp::endpoint(boost::asio::ip::address::from_string(cfg_.ip_address()), cfg_.udp_port()))
{
    glog.is(DEBUG1) && glog << cfg_.DebugString() << std::endl;
    start_udp_receive();
}

                                                 
goby::acomms::MoshRelay::~MoshRelay()
{
}


void goby::acomms::MoshRelay::loop()
{
    
}


void goby::acomms::MoshRelay::start_udp_receive()
{
    socket_.async_receive_from(
        boost::asio::buffer(recv_buffer_), remote_endpoint_,
        boost::bind(&MoshRelay::handle_udp_receive, this,
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred));
}

void goby::acomms::MoshRelay::handle_udp_receive(const boost::system::error_code& error,
                                                 std::size_t /*bytes_transferred*/)
{
    if (!error || error == boost::asio::error::message_size)
    {
        for(int i = 0, n = recv_buffer_.size(); i < n; ++i)
            std::cout << std::hex << recv_buffer_[i];
        std::cout << std::endl;
        
        start_udp_receive();
    }
}
