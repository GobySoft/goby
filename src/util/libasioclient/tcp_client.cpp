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

#include "tcp_client.h"


std::map<std::string, goby::util::TCPClient*> goby::util::TCPClient::inst_;


goby::util::TCPClient* goby::util::TCPClient::get_instance(unsigned& clientkey,
                                                           const std::string& server,
                                                           unsigned port,
                                                           const std::string& delimiter /*= "\r\n"*/)
{
    std::string port_str = boost::lexical_cast<std::string>(port);
    std::string server_port = server + ":" + port_str;
    
    if(!inst_.count(server_port)) // if no connection for this server:port combination, create one
    {
        inst_[server_port] = new TCPClient(server, port_str, delimiter);
        clientkey = 0;
    }
    else // otherwise, register this user with an existing client
        clientkey = inst_[server_port]->add_user();
        
    return(inst_[server_port]);
}

goby::util::TCPClient::TCPClient(const std::string& server,
                                 const std::string& port,
                                 const std::string& delimiter /*= "\r\n"*/)
    : ASIOStreamClient<asio::ip::tcp::socket>(socket_, delimiter),
      socket_(io_service_),
      server_(server),
      port_(port)
{ }


bool goby::util::TCPClient::start_specific()
{
    asio::ip::tcp::resolver resolver(io_service_);
    asio::ip::tcp::resolver::query query(server_, port_);
    asio::ip::tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
    asio::ip::tcp::resolver::iterator end;

    asio::error_code error = asio::error::host_not_found;
    while (error && endpoint_iterator != end)
    {
        socket_.close();
        socket_.connect(*endpoint_iterator++, error);
    }
    
    return error ? false : true;
}

