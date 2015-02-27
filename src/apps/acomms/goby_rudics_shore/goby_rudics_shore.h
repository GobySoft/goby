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

#ifndef GobyRudicsShore20150227H
#define GobyRudicsShore20150227H

#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/enable_shared_from_this.hpp>
#include "goby/pb/protobuf/rudics_shore.pb.h"
#include "goby/util/binary.h"
#include "goby/common/time.h"

namespace goby
{
    namespace acomms
    {
        

        class SBDConnection
            : public boost::enable_shared_from_this<SBDConnection>
        {
          public:
    
            static boost::shared_ptr<SBDConnection> create(boost::asio::io_service& io_service)
            { return boost::shared_ptr<SBDConnection>(new SBDConnection(io_service)); }

            boost::asio::ip::tcp::socket& socket()
            {
                return socket_;
            }

            void start()
            {
                connect_time_ = goby::common::goby_time<double>();
                boost::asio::async_read(socket_,
                                        boost::asio::buffer(data_),
                                        boost::asio::transfer_at_least(PRE_HEADER_SIZE),
                                        boost::bind(&SBDConnection::pre_header_handler, this, _1, _2));
            }

            ~SBDConnection()
            {
            }

            bool data_ready() const { return data_ready_; }
            double connect_time() const { return connect_time_; }
    
            const goby::acomms::protobuf::DirectIPMOPreHeader& pre_header() const { return pre_header_; }
            const goby::acomms::protobuf::DirectIPMOHeader& header() const { return header_; }
            const goby::acomms::protobuf::DirectIPMOPayload& body() const { return body_; }

          private:
          SBDConnection(boost::asio::io_service& io_service)
              : socket_(io_service),
                data_(1 << 16),
                pos_(0),
                data_ready_(false),
                connect_time_(-1)
                {
                }

    
            void pre_header_handler(const boost::system::error_code& error, std::size_t bytes_transferred)
            {
                if(error)
                    throw std::runtime_error("Error while reading: " + error.message());
        

                pre_header_.set_protocol_ver(read_byte());
                pre_header_.set_overall_length(read_uint16());   

                boost::asio::async_read(socket_,
                                        boost::asio::buffer(data_),
                                        boost::asio::transfer_at_least(pre_header_.overall_length() + PRE_HEADER_SIZE
                                                                       - bytes_transferred),
                                        boost::bind(&SBDConnection::ie_handler, this, _1, _2));        
            }
    
            void ie_handler(const boost::system::error_code& error, std::size_t bytes_transferred)
            {
                if(error)
                    throw std::runtime_error("Error while reading: " + error.message());

                char iei = read_byte();
                unsigned length = read_uint16();

                switch(iei)
                {
                    case 0x01: // header
                    {
                        header_.set_iei(iei);
                        header_.set_length(length);
                        header_.set_cdr_reference(read_uint32()); // 4 bytes

                        const int imei_size = 15;
                        header_.set_imei(std::string(data_.begin()+pos_, data_.begin()+pos_+imei_size)); // 15 bytes
                        pos_ += imei_size;
                
                        header_.set_session_status(read_byte()); // 1 byte
                        header_.set_momsn(read_uint16()); // 2 bytes
                        header_.set_mtmsn(read_uint16()); // 2 bytes
                        header_.set_time_of_session(read_uint32()); // 4 bytes
                    }
                    break;

                    case 0x02: // payload
                        body_.set_iei(iei);
                        body_.set_length(length);
                        body_.set_payload(std::string(data_.begin()+pos_, data_.begin()+pos_+length));
                        pos_ += length;
                        break;
                
                    default: // skip this IE
                        pos_ += length;
                        break;
                }

                if(pre_header_.IsInitialized() && header_.IsInitialized() && body_.IsInitialized())
                    data_ready_ = true;
                else if(pos_ < data_.size())
                    ie_handler(error, bytes_transferred);
        
            }

            char read_byte()
            {
                return data_.at(pos_++) & 0xff;
            }
    
            unsigned read_uint16()
            {
                unsigned u = (data_.at(pos_++) & 0xff) << BITS_PER_BYTE;
                u |= data_.at(pos_++) & 0xff;
                return u;
            }
    
            unsigned read_uint32()
            {
                unsigned u = 0;
                for(int i = 0; i < 4; ++i)
                    u |= (data_.at(pos_++) & 0xff) << ((3-i)*BITS_PER_BYTE);
                return u;
            }
    
        
        
            goby::acomms::protobuf::DirectIPMOPreHeader pre_header_;
            goby::acomms::protobuf::DirectIPMOHeader header_;
            goby::acomms::protobuf::DirectIPMOPayload body_;

            enum { PRE_HEADER_SIZE = 3, BITS_PER_BYTE = 8 };
    
    
            boost::asio::ip::tcp::socket socket_;
            std::vector<char> data_;
            std::vector<char>::size_type pos_;
            bool data_ready_;
            double connect_time_;

        };

        class SBDServer
        {
          public:
          SBDServer(boost::asio::io_service& io_service, int port)
              : acceptor_(io_service, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port))
            {
                start_accept();
            }

            std::set<boost::shared_ptr<SBDConnection> >& connections() { return connections_; }
    
          private:
            void start_accept()
            {
                boost::shared_ptr<SBDConnection> new_connection = SBDConnection::create(acceptor_.get_io_service());

                connections_.insert(new_connection);
        
                acceptor_.async_accept(new_connection->socket(),
                                       boost::bind(&SBDServer::handle_accept, this, new_connection,
                                                   boost::asio::placeholders::error));
            }

            void handle_accept(boost::shared_ptr<SBDConnection> new_connection,
                               const boost::system::error_code& error)
            {
                if (!error)
                {
                    new_connection->start();
                }

                start_accept();
            }

            std::set<boost::shared_ptr<SBDConnection> > connections_;
            boost::asio::ip::tcp::acceptor acceptor_;
        };

    }
}



#endif
