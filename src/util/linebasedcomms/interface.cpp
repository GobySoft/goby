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
#include "goby/core/libcore/exception.h"

std::string goby::util::LineBasedInterface::delimiter_;
boost::asio::io_service goby::util::LineBasedInterface::io_service_;
std::deque<goby::util::protobuf::Datagram> goby::util::LineBasedInterface::in_;
boost::mutex goby::util::LineBasedInterface::in_mutex_;


            

goby::util::LineBasedInterface::LineBasedInterface(const std::string& delimiter)
    : work_(io_service_),
      active_(false)
{
    if(delimiter.empty())
        throw Exception("Line based comms started with null string as delimiter!");
    
    delimiter_ = delimiter;
    boost::thread t(boost::bind(&boost::asio::io_service::run, &io_service_));
}


void goby::util::LineBasedInterface::start()
{
    if(active_) return;

    active_ = true;
    io_service_.post(boost::bind(&LineBasedInterface::do_start, this));
}

void goby::util::LineBasedInterface::clear()
{
    boost::mutex::scoped_lock lock(in_mutex_);
    in_.clear();
}

bool goby::util::LineBasedInterface::readline(protobuf::Datagram* msg, AccessOrder order /* = OLDEST_FIRST */)   
{
    if(in_.empty())
    {
        return false;
    }
    else
    {
        boost::mutex::scoped_lock lock(in_mutex_);
        switch(order)
        {
            case NEWEST_FIRST:
                msg->CopyFrom(in_.back());
                in_.pop_back(); 
                break;
                
            case OLDEST_FIRST:
                msg->CopyFrom(in_.front());
                in_.pop_front();       
                break;
        }       
        return true;
    }
}


// pass the write data via the io service in the other thread
void goby::util::LineBasedInterface::write(const protobuf::Datagram& msg)
{ io_service_.post(boost::bind(&LineBasedInterface::do_write, this, msg)); }

// call the do_close function via the io service in the other thread
void goby::util::LineBasedInterface::close()
{ io_service_.post(boost::bind(&LineBasedInterface::do_close, this, boost::system::error_code())); }
