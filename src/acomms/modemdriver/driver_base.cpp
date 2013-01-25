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


#include <boost/thread/mutex.hpp>

#include "goby/common/logger.h"

#include "driver_base.h"
#include "driver_exception.h"

using namespace goby::common::logger;
using namespace goby::common::logger_lock;


int goby::acomms::ModemDriverBase::count_ = 0;



goby::acomms::ModemDriverBase::ModemDriverBase()
    : modem_(0)
{
    ++count_;

    glog_out_group_ = "goby::acomms::modemdriver::out::" + goby::util::as<std::string>(count_);
    glog_in_group_ = "goby::acomms::modemdriver::in::" + goby::util::as<std::string>(count_);
    
    goby::glog.add_group(glog_out_group_, common::Colors::lt_magenta);
    goby::glog.add_group(glog_in_group_, common::Colors::lt_blue);
}

goby::acomms::ModemDriverBase::~ModemDriverBase()
{
    if(modem_) delete modem_;
}

void goby::acomms::ModemDriverBase::modem_write(const std::string& out)
{
    while(!modem_->active())
    {
        goby::glog.is(DEBUG1, lock) && goby::glog << group(glog_out_group_) << warn << "modem is closed! (check physical connection)" << std::endl << unlock;
        sleep(1);
    }
    
    
    modem_->write(out);
}

bool goby::acomms::ModemDriverBase::modem_read(std::string* in)
{
    while(!modem_->active())
    {
        goby::glog.is(DEBUG1, lock) && goby::glog << group(glog_in_group_) << warn << "modem is closed! (check physical connection)" << std::endl << unlock;
        sleep(1);
    }

    return modem_->readline(in);
}

void goby::acomms::ModemDriverBase::modem_close()
{
    modem_->close();    
}



void goby::acomms::ModemDriverBase::modem_start(const protobuf::DriverConfig& cfg)
{        
    if(!cfg.has_modem_id())
        throw(ModemDriverException("missing modem_id in configuration"));
    
    switch(cfg.connection_type())
    {
        case protobuf::DriverConfig::CONNECTION_SERIAL:
            goby::glog.is(DEBUG1, lock) && goby::glog << group(glog_out_group_) << "opening serial port " << cfg.serial_port() << " @ " << cfg.serial_baud() << std::endl << unlock;

            if(!cfg.has_serial_port())
                throw(ModemDriverException("missing serial port in configuration"));
            if(!cfg.has_serial_baud())
                throw(ModemDriverException("missing serial baud in configuration"));
            
            modem_ = new util::SerialClient(cfg.serial_port(), cfg.serial_baud(), cfg.line_delimiter());
            break;
            
        case protobuf::DriverConfig::CONNECTION_TCP_AS_CLIENT:
            goby::glog.is(DEBUG1, lock) && goby::glog << group(glog_out_group_) << "opening tcp client: " << cfg.tcp_server() << ":" << cfg.tcp_port() << std::endl << unlock;
            if(!cfg.has_tcp_server())
                throw(ModemDriverException("missing tcp server address in configuration"));
            if(!cfg.has_tcp_port())
                throw(ModemDriverException("missing tcp port in configuration"));

            modem_ = new util::TCPClient(cfg.tcp_server(), cfg.tcp_port(), cfg.line_delimiter());
            break;
            
        case protobuf::DriverConfig::CONNECTION_TCP_AS_SERVER:
            goby::glog.is(DEBUG1, lock) && goby::glog << group(glog_out_group_) << "opening tcp server on port" << cfg.tcp_port() << std::endl << unlock;

            if(!cfg.has_tcp_port())
                throw(ModemDriverException("missing tcp port in configuration"));

            modem_ = new util::TCPServer(cfg.tcp_port(), cfg.line_delimiter());

        case protobuf::DriverConfig::CONNECTION_DUAL_UDP_BROADCAST:
            throw(ModemDriverException("unimplemented connection type"));
    }    

    modem_->start();
}

