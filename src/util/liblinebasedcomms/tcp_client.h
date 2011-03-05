// copyright 2009 t. schneider tes@mit.edu
// 
// this file is part of comms, a library for handling various communications.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This software is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this software.  If not, see <http://www.gnu.org/licenses/>.

#ifndef TCPClientH
#define TCPClientH

#include "client_base.h"
#include "goby/util/string.h"

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

        
          private:
            static std::map<std::string, TCPClient*> inst_;
            boost::asio::ip::tcp::socket socket_;
            std::string server_;
            std::string port_;

        }; 
    }
}

#endif
