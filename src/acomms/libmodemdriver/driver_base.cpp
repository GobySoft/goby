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

goby::modem::DriverBase::DriverBase(std::ostream* os, const std::string& line_delimiter)
    : serial_delimiter_(line_delimiter),
      os_(os)
{
    
}

void goby::modem::DriverBase::serial_write(const std::string& out)
{
    serial_->write(out);
    if(callback_out_raw) callback_out_raw(out);
}

bool goby::modem::DriverBase::serial_read(std::string& in)
{
    // SerialClient can write to this deque, so lock it
    boost::mutex::scoped_lock lock(in_mutex_);

    if(in_.empty())
        return false;
    else
    {
        in = in_.front();

        if(callback_in_raw) callback_in_raw(in);
        in_.pop_front();
        return true;
    }
}

void goby::modem::DriverBase::serial_start()
{        
    if(os_) *os_ << group("mm_out") << "opening serial port " << serial_port_
          << " @ " << baud_ << std::endl;
    
    try
    {
        serial_ = serial::SerialClient::getInstance(serial_port_, baud_, &in_, &in_mutex_, serial_delimiter_);
        serial_->start();
    }
    catch (...)
    {
        throw(std::runtime_error(std::string("failed to open serial port: " + serial_port_ + ". make sure this port exists and that you have permission to use it.")));
    }
    
}
