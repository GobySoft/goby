// copyright 2009 t. schneider tes@mit.edu
// 
// this file is part of the goby-acomms acoustic modem driver.
// goby-acomms is a collection of libraries 
// for acoustic underwater networking
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

#include <boost/thread/mutex.hpp>

#include "goby/util/logger.h"

#include "driver_base.h"

goby::acomms::ModemDriverBase::ModemDriverBase(std::ostream* log /* = 0 */, const std::string& line_delimiter /* = "\r\n"*/ )
    : line_delimiter_(line_delimiter),
      log_(log),
      connection_type_(serial)
{ }

goby::acomms::ModemDriverBase::~ModemDriverBase()
{
    if(modem_) modem_->remove_user(modem_key_);
}


void goby::acomms::ModemDriverBase::modem_write(const std::string& out)
{
    modem_->write(out);
    if(callback_out_raw) callback_out_raw(out);
}

bool goby::acomms::ModemDriverBase::modem_read(std::string& in)
{
    if(!(in = modem_->readline(modem_key_)).empty())
    {
        if(callback_in_raw) callback_in_raw(in); 
        return true;
    }
    else
        return false;
}

void goby::acomms::ModemDriverBase::modem_start()
{        
    if(log_) *log_ << group("mm_out") << "opening serial port " << serial_port_
          << " @ " << serial_baud_ << std::endl;
    
    switch(connection_type_)
    {
        case serial:
            modem_ = util::SerialClient::get_instance(modem_key_, serial_port_, serial_baud_, line_delimiter_);
            break;

        case tcp_as_client:
            modem_ = util::TCPClient::get_instance(modem_key_, tcp_server_, tcp_port_, line_delimiter_);
            break;
            
        case tcp_as_server:
            modem_ = util::TCPServer::get_instance(modem_key_, tcp_port_, line_delimiter_);

        case dual_udp_broadcast:
            throw(std::runtime_error("unimplemented connection type"));
    }
    

    modem_->start();
    
}
