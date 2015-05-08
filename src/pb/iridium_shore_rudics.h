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

#ifndef IridiumShoreSBD20150508H
#define IridiumShoreSBD20150508H

#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/signals2.hpp>
#include <boost/enable_shared_from_this.hpp>

#include "goby/util/binary.h"
#include "goby/common/time.h"

namespace goby
{
    namespace acomms
    {
        class RUDICSConnection
            : public boost::enable_shared_from_this<RUDICSConnection>
        {
          public:
    
            static boost::shared_ptr<RUDICSConnection> create(boost::asio::io_service& io_service)
            { return boost::shared_ptr<RUDICSConnection>(new RUDICSConnection(io_service)); }

            boost::asio::ip::tcp::socket& socket()
            {
                return socket_;
            }

            void start()
            {                
                read_start();
            }

            void read_start()
            {
                boost::asio::async_read_until(socket_,
                                              buffer_,
                                              '\r',
                                              boost::bind(&RUDICSConnection::handle_read, this, _1, _2));
            }
            
            ~RUDICSConnection()
            {
                using goby::glog;
                using goby::common::logger::DEBUG1;
                glog.is(DEBUG1) && glog << "Disconnecting from: " << socket_.remote_endpoint() << std::endl;
            }

            
            boost::signals2::signal<void (const std::string& line, boost::shared_ptr<RUDICSConnection> connection)> line_signal;
            boost::signals2::signal<void (boost::shared_ptr<RUDICSConnection> connection)> disconnect_signal;

          private:
          RUDICSConnection(boost::asio::io_service& io_service)
              : socket_(io_service)
            {

            }


            void handle_write(const boost::system::error_code& error,
                              size_t /*bytes_transferred*/)
            {
                if(error)
                {
                    using goby::glog;
                    using goby::common::logger::WARN;
                    glog.is(WARN) && glog << "Error writing to TCP connection: " << error << std::endl;
                    disconnect_signal(shared_from_this());
                }
            }

            void handle_read(const boost::system::error_code& error,
                             size_t bytes_transferred)
            {
                if(!error)
                {
                    buffer_.commit(bytes_transferred);
                    std::istream istrm(&buffer_);
                    std::string line;
                    while(std::getline(istrm, line, '\r'))
                        line_signal(line + "\r", shared_from_this());
                    read_start();
                }
                else
                {
                    using goby::glog;
                    using goby::common::logger::WARN;
                    glog.is(WARN) && glog << "Error reading from TCP connection: " << error << std::endl;                
                    disconnect_signal(shared_from_this());
                }
            }

          private:
            boost::asio::ip::tcp::socket socket_;
            boost::asio::streambuf buffer_;

        };

        class RUDICSServer
        {
          public:
          RUDICSServer(boost::asio::io_service& io_service, int port)
              : acceptor_(io_service, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port))
            {
                start_accept();
            }

            std::set<boost::shared_ptr<RUDICSConnection> >& connections() { return connections_; }

            boost::signals2::signal<void (boost::shared_ptr<RUDICSConnection> connection)> connect_signal;

          private:
            void start_accept()
            {
                boost::shared_ptr<RUDICSConnection> new_connection = RUDICSConnection::create(acceptor_.get_io_service());

                connections_.insert(new_connection);
        
                acceptor_.async_accept(new_connection->socket(),
                                       boost::bind(&RUDICSServer::handle_accept, this, new_connection,
                                                   boost::asio::placeholders::error));
            }

            void handle_accept(boost::shared_ptr<RUDICSConnection> new_connection,
                               const boost::system::error_code& error)
            {
                if (!error)
                {
		    using namespace goby::common::logger;
		    using goby::glog;
		    
		    glog.is(DEBUG1) && glog << "Received connection from: " << new_connection->socket().remote_endpoint() << std::endl;

                    new_connection->disconnect_signal.connect(boost::bind(&RUDICSServer::handle_disconnect, this, _1));                    
                    connect_signal(new_connection);
                    new_connection->start();
                }

                start_accept();
            }

            void handle_disconnect(boost::shared_ptr<RUDICSConnection> connection)
            {
                connections_.erase(connection);
            }

            std::set<boost::shared_ptr<RUDICSConnection> > connections_;
            boost::asio::ip::tcp::acceptor acceptor_;
        };

    }
}



#endif
