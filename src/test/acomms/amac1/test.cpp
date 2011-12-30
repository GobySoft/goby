// copyright 2011 t. schneider tes@mit.edu
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


// tests fixed TDMA

#include "goby/acomms/amac.h"
#include "goby/common/logger.h"
#include "goby/util/sci.h"
#include "goby/acomms/connect.h"

goby::acomms::MACManager mac;
const int num_cycles_check = 3;
int first_cycle = -1;
int current_cycle = -1;
int me = 1;

using goby::acomms::operator<<;

void initiate_transmission(const goby::acomms::protobuf::ModemTransmission& msg)
{
    std::cout << "We were told to start transmission of " << msg << std::endl;
    assert(msg.src() == me);
    double cycles_since_day = (goby::util::goby_time().time_of_day().total_milliseconds() / 1000.0) / mac.cycle_duration();
    
    std::cout << std::setprecision(15) << cycles_since_day << std::endl;
    std::cout << std::setprecision(15) << goby::util::unbiased_round(cycles_since_day,0)
              << std::endl;

    current_cycle = cycles_since_day;
    if(first_cycle == -1)
        first_cycle = current_cycle;

    assert(mac.cycle_count() == 3);
    
    assert(goby::util::unbiased_round(cycles_since_day - goby::util::unbiased_round(cycles_since_day,0), 1) == 0);
}


int main(int argc, char* argv[])
{
    goby::glog.add_stream(goby::util::logger::DEBUG3, &std::cerr);
    goby::glog.set_name(argv[0]);

    // add slots as part of cfg
    goby::acomms::protobuf::MACConfig cfg;
    cfg.set_modem_id(me);
    cfg.set_type(goby::acomms::protobuf::MAC_FIXED_DECENTRALIZED);

    goby::acomms::protobuf::ModemTransmission downlink_slot;
    downlink_slot.set_src(1);
    downlink_slot.set_rate(0);
    downlink_slot.set_type(goby::acomms::protobuf::ModemTransmission::DATA);
    downlink_slot.SetExtension(goby::acomms::protobuf::slot_seconds, 0.1);
    cfg.add_slot()->CopyFrom(downlink_slot);

    goby::acomms::protobuf::ModemTransmission uplink3_slot;
    uplink3_slot.set_src(3);
    uplink3_slot.set_rate(0);
    uplink3_slot.set_type(goby::acomms::protobuf::ModemTransmission::DATA);
    uplink3_slot.SetExtension(goby::acomms::protobuf::slot_seconds, 0.1);
    cfg.add_slot()->CopyFrom(uplink3_slot);


    goby::acomms::protobuf::ModemTransmission uplink4_slot;
    uplink4_slot.set_src(4);
    uplink4_slot.set_rate(0);
    uplink4_slot.set_type(goby::acomms::protobuf::ModemTransmission::DATA);
    uplink4_slot.SetExtension(goby::acomms::protobuf::slot_seconds, 0.1);
    cfg.add_slot()->CopyFrom(uplink4_slot);

    goby::acomms::connect(&mac.signal_initiate_transmission,
                          &initiate_transmission);
    
    
    mac.startup(cfg);
    
    
    while(first_cycle == -1 || (current_cycle < first_cycle + num_cycles_check))
    {
        mac.do_work();
        usleep(1e2);
    }
    
    first_cycle = -1;
    current_cycle = -1;
    
    mac.shutdown();

    // add slots not through cfg
    mac.clear();
    mac.push_back(downlink_slot);
    mac.push_back(uplink3_slot);
    mac.update();
    
    mac.push_back(uplink4_slot);
    mac.remove(downlink_slot);
    mac.push_back(downlink_slot);    
    mac.update();

    cfg.Clear();
    me = 3;
    
    cfg.set_modem_id(me);
    cfg.set_type(goby::acomms::protobuf::MAC_FIXED_DECENTRALIZED);
    mac.startup(cfg);
    
    
    while(first_cycle == -1 || (current_cycle < first_cycle + num_cycles_check))
        mac.get_io_service().run_one();
        
    
    std::cout << "all tests passed" << std::endl;
}

