// copyright 2009, 2010 t. schneider tes@mit.edu
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

#include <iostream>

#include <boost/thread.hpp>

#include "serial_client.h"

std::map<std::string, goby::serial::SerialClient*> goby::serial::SerialClient::inst_;


goby::serial::SerialClient* goby::serial::SerialClient::getInstance(const std::string& name,
                                                        unsigned baud,
                                                        std::deque<std::string>* in,
                                                        boost::mutex* in_mutex,
                                                        const std::string& delimiter /*= "\r\n"*/)
{
    // uniquely defines the serial port
    std::string port_baud = name + ":" + boost::lexical_cast<std::string>(baud);
    
    if(!inst_.count(port_baud))
        inst_[port_baud] = new SerialClient(name, baud, in, in_mutex, delimiter);
    else
        inst_[port_baud]->add_user(in);
        
    return(inst_[port_baud]);
}


goby::serial::SerialClient::SerialClient(const std::string& name,
                                   unsigned baud,
                                   std::deque<std::string>* in,
                                   boost::mutex* in_mutex,
                                   const std::string& delimiter)
    : comms::ClientBase<asio::serial_port>(serial_port_, in, in_mutex, delimiter),
      serial_port_(io_service_),
      name_(name),
      baud_(baud)
{ }

bool goby::serial::SerialClient::start_specific()
{
    try
    {
        serial_port_.open(name_);
    }
    catch(std::exception& e)
    {
        serial_port_.close();
        return false;
    }
    
    serial_port_.set_option(asio::serial_port_base::baud_rate(baud_));
    return true;
    
}

