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
#include "driver_exception.h"

goby::acomms::ModemDriverBase::ModemDriverBase(std::ostream* log /* = 0 */, const std::string& line_delimiter /* = "\r\n"*/ )
    : line_delimiter_(line_delimiter),
      log_(log),
      modem_(0),
      connection_type_(serial)
{ }

goby::acomms::ModemDriverBase::~ModemDriverBase()
{
    if(modem_) delete modem_;
}


void goby::acomms::ModemDriverBase::modem_write(const std::string& out)
{
    modem_->write(out);
}

bool goby::acomms::ModemDriverBase::modem_read(std::string& in)
{
    if(!(in = modem_->readline()).empty())
    {
        return true;
    }
    else
        return false;
}

void goby::acomms::ModemDriverBase::modem_start()
{        
    
    switch(connection_type_)
    {
        case serial:
            if(log_) *log_ << group("mm_out") << "opening serial port " << serial_port_ << " @ " << serial_baud_ << std::endl;
            
            modem_ = new util::SerialClient(serial_port_, serial_baud_, line_delimiter_);
            break;
            
        case tcp_as_client:
            if(log_) *log_ << group("mm_out") << "opening tcp client: " << tcp_server_ << ":" << tcp_port_ << std::endl;
            modem_ = new util::TCPClient(tcp_server_, tcp_port_, line_delimiter_);
            break;
            
        case tcp_as_server:
            if(log_) *log_ << group("mm_out") << "opening tcp server on port" << tcp_port_ << std::endl;
            modem_ = new util::TCPServer(tcp_port_, line_delimiter_);

        case dual_udp_broadcast:
            throw(driver_exception("unimplemented connection type"));
    }
    

    modem_->start();

}

void goby::acomms::ModemDriverBase::add_flex_groups(util::FlexOstream& tout)
{
    tout.add_group("mm_out", util::Colors::lt_magenta, "outgoing micromodem messages (goby_modemdriver)");
    tout.add_group("mm_in", util::Colors::lt_blue, "incoming micromodem messages (goby_modemdriver)");
}
