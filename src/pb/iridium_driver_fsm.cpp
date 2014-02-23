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

#include "rudics_packet.h"
#include "iridium_driver_fsm.h"

using goby::glog;
using namespace goby::common::logger;
using goby::common::goby_time;

int goby::acomms::fsm::IridiumDriverFSM::count_ = 0;

void goby::acomms::fsm::IridiumDriverFSM::buffer_data_out( const goby::acomms::protobuf::ModemTransmission& msg)
{
    data_out_.push_back(msg);
}

void goby::acomms::fsm::Command::in_state_react(const EvRxSerial& e)
{
    std::string in = e.line;
    
    boost::trim_if(in, !boost::algorithm::is_alnum());
    
    
    static const std::string connect = "CONNECT";
    
    if(in == "OK")
        post_event(EvAck(in));
    else if(in == "RING")
        post_event(EvRing());
    else if(in == "NO CARRIER")
    {
        post_event(EvAck(in));
        post_event(EvNoCarrier());
    }
    else if(in.compare(0, connect.size(), connect) == 0)
    {
        post_event(EvAck(in));
        post_event(EvConnect());
    }
    else if(in == "NO DIALTONE")
    {
        post_event(EvAck(in));
        post_event(EvNoCarrier());
    }
    else if(in == "BUSY")
    {
        post_event(EvAck(in));
        post_event(EvNoCarrier());
    }
    else if(in == "ERROR")
    {
        post_event(EvAck(in));
    }
}


void goby::acomms::fsm::Command::in_state_react( const EvTxSerial& )
{
    double now = goby_time<double>();
    
    if(!at_out_.empty())
    {
        double timeout = COMMAND_TIMEOUT_SECONDS;
        switch(at_out_.front().second[0])
        {
            default: break;
            case 'D': timeout = DIAL_TIMEOUT_SECONDS; break;
            case 'A': timeout = ANSWER_TIMEOUT_SECONDS; break;
            case '+': timeout = TRIPLE_PLUS_TIMEOUT_SECONDS; break;
        }
        

        if((at_out_.front().first + timeout) < now)
        {
            std::string at_command;
            if(at_out_.front().second != "+++")
                at_command = "AT" + at_out_.front().second + "\r";
            else
                at_command = at_out_.front().second;        
            
            context<IridiumDriverFSM>().serial_tx_buffer().push_back(at_command);
            at_out_.front().first = now;
        }
        
    }
}


void goby::acomms::fsm::Online::in_state_react(const EvRxSerial& e)
{
    EvRxOnCallSerial eo;
    eo.line = e.line;
    post_event(eo);
}


void goby::acomms::fsm::Online::in_state_react( const EvTxSerial& )
{
    post_event(EvTxOnCallSerial());
}

void goby::acomms::fsm::Command::in_state_react( const EvAck & e)
{
    if(!at_out().empty())
    {
        const std::string& last_at = at_out().front().second;
       if(last_at.size() > 0 && e.response_ == "OK")
       {
           switch(last_at[0])
           {
               case 'H': post_event(EvNoCarrier()); break;
//               case 'O': post_event(EvOnline()); break;
               default:
                   break;
           }
      }
        at_out().pop_front();
        if(at_out().empty())
            post_event(EvAtEmpty());
        
    }
    else
    {
        glog.is(DEBUG1) && glog << group("iridiumdriver") <<  warn << "Unexpected 'OK'" << std::endl;
    }
}

boost::statechart::result goby::acomms::fsm::Dial::react( const EvNoCarrier& x)
{
    glog.is(DEBUG1) && glog  << group("iridiumdriver") << "Redialing..."  << std::endl;
    
    const int max_attempts = context<IridiumDriverFSM>().driver_cfg().GetExtension(IridiumDriverConfig::dial_attempts);
    if(dial_attempts_ < max_attempts)
    {
        dial();
        return discard_event();
    }
    else
    {
        glog.is(DEBUG1) && glog <<  warn  << group("iridiumdriver") << "Failed to connect after " << max_attempts << " tries." << std::endl;
                
        return transit<Ready>();
    }
    
}

void  goby::acomms::fsm::Dial::dial()
{
    ++dial_attempts_;
    context<Command>().push_at_command(
        "D" +
        context<IridiumDriverFSM>().driver_cfg().GetExtension(
            IridiumDriverConfig::remote).iridium_number());
}    


void goby::acomms::fsm::OnCall::in_state_react(const EvRxOnCallSerial& e)
{
    std::string in = e.line;

    // check that it's not "NO CARRIER"
    static const std::string no_carrier = "NO CARRIER";
    if(in.find(no_carrier) != std::string::npos)
    {
        post_event(EvNoCarrier());
    }
    else if(boost::trim_copy(in) == "goby")
    {
        glog.is(DEBUG1) && glog << group("iridiumdriver") << "Detected start of Goby RUDICS call" << std::endl;        
    }
    else
    {
        std::string bytes;
        try
        {
            parse_rudics_packet(&bytes, in);
            
            protobuf::ModemTransmission msg;
            msg.ParseFromString(bytes);
            context< IridiumDriverFSM >().received().push_back(msg);
            context< IridiumDriverFSM >().last_rx_tx_time() = goby_time<double>();
        }
        catch(RudicsPacketException& e)
        {
	  glog.is(DEBUG1) && glog <<  warn  << group("iridiumdriver") << "Could not decode packet: " << e.what() << std::endl;
        }
    }
}


void goby::acomms::fsm::OnCall::in_state_react( const EvTxOnCallSerial& )
{
    boost::circular_buffer<protobuf::ModemTransmission>& data_out = context<IridiumDriverFSM>().data_out();
    if(!data_out.empty())
    {
        // serialize the (protobuf) message
        std::string bytes;
        data_out.front().SerializeToString(&bytes);

        // frame message
        std::string rudics_packet;
        serialize_rudics_packet(bytes, &rudics_packet);

        context<IridiumDriverFSM>().serial_tx_buffer().push_back(rudics_packet);
        data_out.pop_front();

        context<IridiumDriverFSM>().last_rx_tx_time() = goby_time<double>();
    }
}
