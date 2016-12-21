// Copyright 2009-2016 Toby Schneider (http://gobysoft.org/index.wt/people/toby)
//                     GobySoft, LLC (2013-)
//                     Massachusetts Institute of Technology (2007-2014)
//                     Community contributors (see AUTHORS file)
//
//
// This file is part of the Goby Underwater Autonomy Project Libraries
// ("The Goby Libraries").
//
// The Goby Libraries are free software: you can redistribute them and/or modify
// them under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 2.1 of the License, or
// (at your option) any later version.
//
// The Goby Libraries are distributed in the hope that they will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with Goby.  If not, see <http://www.gnu.org/licenses/>.

#include "benthos_atm900_driver.h"
#include "driver_exception.h"

#include "goby/common/logger.h"
#include "goby/util/binary.h"

using goby::util::hex_encode;
using goby::util::hex_decode;
using goby::glog;
using namespace goby::common::logger;

goby::acomms::BenthosATM900Driver::BenthosATM900Driver()
{
}

void goby::acomms::BenthosATM900Driver::startup(const protobuf::DriverConfig& cfg)
{
    driver_cfg_ = cfg;

    if(!driver_cfg_.has_serial_baud())
        driver_cfg_.set_serial_baud(DEFAULT_BAUD);

    glog.is(DEBUG1) && glog << group(glog_out_group()) << "BenthosATM900Driver: Starting modem..." << std::endl;
    ModemDriverBase::modem_start(driver_cfg_);

}

void goby::acomms::BenthosATM900Driver::shutdown()
{
    // put the modem in a low power state?
    ModemDriverBase::modem_close();
} 

void goby::acomms::BenthosATM900Driver::handle_initiate_transmission(const protobuf::ModemTransmission& orig_msg)
{
} 

void goby::acomms::BenthosATM900Driver::do_work()
{
    std::string in;
    while(modem_read(&in))
    {

    }
}

