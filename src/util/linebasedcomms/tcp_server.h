

#ifndef TCPServer20100719H
#define TCPServer20100719H

#include <iostream>
#include <string>
#include <set>
#include <deque>

#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/foreach.hpp>
#include <boost/asio.hpp>

#include "goby/util/as.h"

#include "interface.h"
#include "connection.h"

namespace goby
{
    namespace util
    {
        class TCPConnection;
        
        /// provides a basic TCP server for line by line text based communications to a one or more remote TCP clients  
        class TCPServer : public LineBasedInterface
        {
          public:
            /// \brief create a TCP server
            ///
            /// \param port port of the server (use 50000+ to avoid problems with special system ports)
            /// \param delimiter string used to split lines
          TCPServer(unsigned port,
                    const std::string& delimiter = "\r\n")
              : LineBasedInterface(delimiter),
                acceptor_(io_service(),
                          boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port))
                {  }
            virtual ~TCPServer() 
            { }
            
            
            /// \brief string representation of the local endpoint (e.g. 192.168.1.105:54230
            std::string local_endpoint() { return goby::util::as<std::string>(acceptor_.local_endpoint()); }

            
            friend class TCPConnection;
            friend class LineBasedConnection<boost::asio::ip::tcp::socket>;

          private:
            void do_start()
            { start_accept(); }
        
            void do_write(const protobuf::Datagram& line);
            void do_close(const boost::system::error_code& error);
            
          private:
            void start_accept();
            void handle_accept(boost::shared_ptr<TCPConnection> new_connection,
                               const boost::system::error_code& error);
            
          private:
            std::string server_;
            boost::asio::ip::tcp::acceptor acceptor_;
            boost::shared_ptr<TCPConnection> new_connection_;
            std::set< boost::shared_ptr<TCPConnection> > connections_;    
        };


        class TCPConnection : public boost::enable_shared_from_this<TCPConnection>,
            public LineBasedConnection<boost::asio::ip::tcp::socket>
            {
              public:
                static boost::shared_ptr<TCPConnection> create(LineBasedInterface* interface);
                
                boost::asio::ip::tcp::socket& socket()
                { return socket_; }
    
                void start()
                { socket_.get_io_service().post(boost::bind(&TCPConnection::read_start, this)); }

                void write(const protobuf::Datagram& msg)
                { socket_.get_io_service().post(boost::bind(&TCPConnection::socket_write, this, msg)); }    

                void close(const boost::system::error_code& error)
                { socket_.get_io_service().post(boost::bind(&TCPConnection::socket_close, this, error)); }
                
                std::string local_endpoint() { return goby::util::as<std::string>(socket_.local_endpoint()); }
                std::string remote_endpoint() { return goby::util::as<std::string>(socket_.remote_endpoint()); }
                
              private:
                void socket_write(const protobuf::Datagram& line);
                void socket_close(const boost::system::error_code& error);
    
              TCPConnection(LineBasedInterface* interface)
                  : LineBasedConnection<boost::asio::ip::tcp::socket>(interface),
                    socket_(interface->io_service())
                { }
                
                
              private:
                boost::asio::ip::tcp::socket socket_;
            };
    }

}

#endif
