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

#include "interface.h"

void goby::util::LineBasedInterface::start()
{
    if(active_) return;
    
    io_service_.post(boost::bind(&LineBasedInterface::do_start, this));
}

void goby::util::LineBasedInterface::clear()
{
    boost::mutex::scoped_lock lock(in_mutex_);
    in_.clear();
}


std::string goby::util::LineBasedInterface::readline_oldest()
{
    boost::mutex::scoped_lock lock(in_mutex_);
    if(in_.empty()) return "";
    else
    {
        std::string in = in_.front();
        in_.pop_front();
        return in;
    }
}

std::string goby::util::LineBasedInterface::readline_newest()
{
    boost::mutex::scoped_lock lock(in_mutex_);
    if(in_.empty()) return "";
    else
    {
        std::string in = in_.back();
        in_.pop_back();
        return in;
    }
}
            

goby::util::LineBasedInterface::LineBasedInterface()
    : work_(io_service_),
      active_(false)
{
    boost::thread t(boost::bind(&boost::asio::io_service::run, &io_service_));
}

// pass the write data via the io service in the other thread
void goby::util::LineBasedInterface::write(const std::string& msg)
{ io_service_.post(boost::bind(&LineBasedInterface::do_write, this, msg)); }

// call the do_close function via the io service in the other thread
void goby::util::LineBasedInterface::close()
{ io_service_.post(boost::bind(&LineBasedInterface::do_close, this, boost::system::error_code())); }
