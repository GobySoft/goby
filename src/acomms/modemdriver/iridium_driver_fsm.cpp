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


void goby::acomms::fsm::IridiumDriverFSM::buffer_data_out( const goby::acomms::protobuf::ModemTransmission& msg)
{
    // serialize the (protobuf) message
    std::string bytes;
    msg.SerializeToString(&bytes);

    // frame message
    std::string rudics_packet;
    serialize_rudics_packet(bytes, &rudics_packet);

    glog.is(DEBUG1) && glog << "Buffering message to send: " << goby::util::hex_encode(rudics_packet) << std::endl;
    
    data_out_.push_back(rudics_packet);


    // for now, make a call
    process_event(EvDial());
    
}

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
    double now = goby_time<double>();
    if(!at_out_.empty() && at_out_.front().first + COMMAND_TIMEOUT_SECONDS < now)
    {
        context<IridiumDriverFSM>().serial_tx_buffer().push_back("AT" + at_out_.front().second + "\r");
        at_out_.front().first = now;
    }
}


void goby::acomms::fsm::Ready::in_state_react( const EvOk & e)
{
    if(!context<Command>().at_out().empty())
    {
        const std::string& last_at = context<Command>().at_out().front().second;
        if(last_at.size() > 0)
        {
            switch(last_at[0])
            {
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


void goby::acomms::fsm::OnCall::in_state_react( const EvTxSerial& )
{
    boost::circular_buffer<std::string>& data_out = context<IridiumDriverFSM>().data_out();
    if(!data_out.empty())
    {
        context<IridiumDriverFSM>().serial_tx_buffer().push_back(data_out.front());
        data_out.pop_front();
    }
}
