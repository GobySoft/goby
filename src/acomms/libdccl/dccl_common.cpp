// copyright 2009-2011 t. schneider tes@mit.edu
// 
// this file is part of the Dynamic Compact Control Language (DCCL),
// the goby-acomms codec. goby-acomms is a collection of libraries 
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

#include "dccl_common.h"

std::ostream* goby::acomms::DCCLCommon::log_ = 0;
std::ofstream goby::acomms::DCCLCommon::null_;
google::protobuf::DynamicMessageFactory* goby::acomms::DCCLCommon::message_factory_ = 0;

void goby::acomms::DCCLCommon::set_log(std::ostream* log)
{
    log_ = log;
    
    util::FlexOstream* flex_log = dynamic_cast<util::FlexOstream*>(log);
    if(flex_log)
        add_flex_groups(flex_log);
}
void goby::acomms::DCCLCommon::initialize()
{
    message_factory_ = new google::protobuf::DynamicMessageFactory;
    
    null_.open("/dev/null");

    if(!log_)
        log_ = &null_;
}

void goby::acomms::DCCLCommon::shutdown()
{
    delete message_factory_;
}


void goby::acomms::DCCLCommon::add_flex_groups(util::FlexOstream* tout) 
{
    tout->add_group("dccl_enc", util::Colors::lt_magenta, "encoder messages (goby_dccl)");
    tout->add_group("dccl_dec", util::Colors::lt_blue, "decoder messages (goby_dccl)");
}

