// copyright 2010 t. schneider tes@mit.edu
//
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

#include "tcp_server.h"

boost::shared_ptr<goby::util::TCPConnection> goby::util::TCPConnection::create(boost::asio::io_service& io_service,
                                                                   std::deque<std::string>& in,
                                                                   boost::mutex& in_mutex,
                                                                   const std::string& delimiter)
{
    return boost::shared_ptr<TCPConnection>(new TCPConnection(io_service, in, in_mutex, delimiter));
}        

void goby::util::TCPConnection::socket_write(const std::string& line) // give it a string
{ // callback to handle write call from outside this class
    bool write_in_progress = !out_.empty(); // is there anything currently being written?
    out_.push_back(line); // store in write buffer
    if (!write_in_progress) // if nothing is currently being written, then start
        write_start();
}    

void goby::util::TCPConnection::socket_close(const boost::system::error_code& error)
{ // something has gone wrong, so close the socket & make this object inactive
    if (error == boost::asio::error::operation_aborted) // if this call is the result of a timer cancel()
        return; // ignore it because the connection cancelled the timer
    
    if(error)
        socket_.close();
}

    
void goby::util::TCPServer::do_write(const std::string& line)
{
    BOOST_FOREACH(boost::shared_ptr<TCPConnection> c, connections_)
        c->write(line);
}

// close all the connections, it's up to the clients to try to reconnect
void goby::util::TCPServer::do_close(const boost::system::error_code& error)
{
    BOOST_FOREACH(boost::shared_ptr<TCPConnection> c, connections_)
        c->close(error);
}    

void goby::util::TCPServer::start_accept()
{
    new_connection_ = TCPConnection::create(acceptor_.io_service(), in_, in_mutex_, delimiter_);
    acceptor_.async_accept(new_connection_->socket(),
                           boost::bind(&TCPServer::handle_accept, this, new_connection_, boost::asio::placeholders::error));
}

void goby::util::TCPServer::handle_accept(boost::shared_ptr<TCPConnection> new_connection, const boost::system::error_code& error)
{
    if (!error)
    {
        new_connection_->start();
        connections_.insert(new_connection_);
        start_accept();
    }
}
