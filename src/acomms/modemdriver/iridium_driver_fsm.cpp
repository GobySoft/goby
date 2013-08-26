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

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string.hpp>

#include "goby/common/logger.h"
#include "goby/common/time.h"
#include "goby/util/binary.h"

#include "iridium_driver_fsm.h"
#include "iridium_driver.h"

using goby::glog;
using namespace goby::common::logger;
using goby::common::goby_time;


void goby::acomms::fsm::Command::in_state_react(const EvRxSerial& e)
{
    std::string in = e.line;
    
    boost::trim_if(in, !boost::algorithm::is_alnum());
    glog.is(DEBUG1) && glog << in << std::endl;

    if(in == "OK")
        post_event(EvOk());
    else if(in == "RING")
        post_event(EvRing());
    else if(in == "NO CARRIER")
        post_event(EvNoCarrier());
    else if(in.compare(0, 7, "CONNECT") == 0)
        post_event(EvConnect());
}

void goby::acomms::fsm::Command::in_state_react( const EvTxSerial& )
{    
    if(!at_out().empty())
        context<IridiumDriverFSM>().send().push_back("AT" + at_out().front() + "\r");
}


void goby::acomms::fsm::Ready::in_state_react( const EvOk & e)
{
    if(!context<Command>().at_out().empty())
    {
        const std::string& last_at = context<Command>().at_out().front();
        if(last_at.size() > 0)
        {
            switch(last_at[0])
            {
                case 'A': post_event(EvATA()); break;
                case 'H': post_event(EvATH0()); break;
                case 'O': post_event(EvATO()); break;
                default:
                    break;
            }
        }        
        context<Command>().at_out().pop_front();
    }
    else
    {
        glog.is(DEBUG1) && glog <<  warn << "Unexpected 'OK'" << std::endl;
    }
}


void goby::acomms::fsm::OnCall::in_state_react(const EvRxSerial& e)
{
    std::string in = e.line;
    glog.is(DEBUG1) && glog << goby::util::hex_encode(in) << std::endl;

    std::string bytes;
    try
    {
        parse_rudics_packet(&bytes, in);
                
        protobuf::ModemTransmission msg;
        msg.ParseFromString(bytes);
        context< IridiumDriverFSM >().received().push_back(msg);
    }
    catch(RudicsPacketException& e)
    {
        glog.is(DEBUG1) && glog <<  warn << "Could not decode packet: " << e.what() << std::endl;
    }
}

void goby::acomms::fsm::OnCall::in_state_react( const EvSendData& e)
{
    // send the message
    std::string bytes;
    e.msg.SerializeToString(&bytes);

    // frame message
    std::string rudics_packet;
    serialize_rudics_packet(bytes, &rudics_packet);
    
    data_out_.push_back(rudics_packet);
}

void goby::acomms::fsm::OnCall::in_state_react( const EvTxSerial& )
{
    if(!data_out_.empty())
    {
        context<IridiumDriverFSM>().send().push_back(data_out_.front());
        data_out_.pop_front();
    }
}
