// Copyright 2009-2013 Toby Schneider (https://launchpad.net/~tes)
//                     Massachusetts Institute of Technology (2007-)
//                     Woods Hole Oceanographic Institution (2007-)
//                     Goby Developers Team (https://launchpad.net/~goby-dev)
// 
//
// This file is part of the Goby Underwater Autonomy Project Libraries
// ("The Goby Libraries").
//
// The Goby Libraries are free software: you can redistribute them and/or modify
// them under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// The Goby Libraries are distributed in the hope that they will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with Goby.  If not, see <http://www.gnu.org/licenses/>.


#include "tcp_server.h"

boost::shared_ptr<goby::util::TCPConnection> goby::util::TCPConnection::create(LineBasedInterface* interface)
{
    return boost::shared_ptr<TCPConnection>(new TCPConnection(interface));
}        

void goby::util::TCPConnection::socket_write(const protobuf::Datagram& line) // give it a string
{
    bool write_in_progress = !out().empty(); // is there anything currently being written?
    out().push_back(line); // store in write buffer
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

    
void goby::util::TCPServer::do_write(const protobuf::Datagram& line)
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
    new_connection_ = TCPConnection::create(this);
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
