
#ifndef TCPClientH
#define TCPClientH

#include "client_base.h"
#include "goby/util/as.h"

namespace goby
{
    namespace util
    {
        /// provides a basic TCP client for line by line text based communications to a remote TCP server
        class TCPClient : public LineBasedClient<boost::asio::ip::tcp::socket>
        {
          public:
            /// \brief create a TCPClient
            ///
            /// \param server domain name or IP address of the remote server
            /// \param port port of the remote server
            /// \param delimiter string used to split lines
            TCPClient(const std::string& server,
                      unsigned port,
                      const std::string& delimiter = "\r\n");
            
            boost::asio::ip::tcp::socket& socket() { return socket_; }

            /// \brief string representation of the local endpoint (e.g. 192.168.1.105:54230
            std::string local_endpoint() { return goby::util::as<std::string>(socket_.local_endpoint()); }
            /// \brief string representation of the remote endpoint, (e.g. 192.168.1.106:50000
            std::string remote_endpoint() { return goby::util::as<std::string>(socket_.remote_endpoint()); }

          private:
            bool start_specific();

            friend class TCPConnection;
            friend class LineBasedConnection<boost::asio::ip::tcp::socket>;

          private:
            boost::asio::ip::tcp::socket socket_;
            std::string server_;
            std::string port_;

        }; 
    }
}

#endif
