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

#include "test.pb.h"
#include "goby/acomms/queue.h"
#include "goby/acomms/connect.h"
#include "goby/util/binary.h"
#include "goby/util/logger.h"

// tests basic DCCL queuing

using goby::acomms::operator<<;

int receive_count = 0;
TestMsg test_msg1;

void handle_receive(const google::protobuf::Message &msg);

int main(int argc, char* argv[])
{
    goby::glog.add_stream(goby::util::Logger::DEBUG3, &std::cerr);
    goby::glog.set_name(argv[0]);
    
    goby::acomms::QueueManager* q_manager = goby::acomms::QueueManager::get();
    goby::acomms::protobuf::QueueManagerConfig cfg;
    const int MY_MODEM_ID = 1;
    cfg.set_modem_id(MY_MODEM_ID);
    q_manager->set_cfg(cfg);
    
    goby::acomms::connect(&q_manager->signal_receive, &handle_receive);

    test_msg1.set_double_default_optional(1.23);
    test_msg1.set_float_default_optional(0.2);

    std::cout << "Pushed: " << test_msg1 << std::endl;
    q_manager->push_message(test_msg1);


    goby::acomms::protobuf::ModemTransmission msg;
    msg.set_max_frame_bytes(256);
    q_manager->handle_modem_data_request(&msg);
    

    std::cout << "requesting data, got: " << msg << std::endl;
    std::cout << "\tdata as hex: " << goby::util::hex_encode(msg.frame(0)) << std::endl;

    std::string encoded;
    goby::acomms::DCCLCodec::get()->encode(&encoded, test_msg1);
    
    assert(msg.frame(0) == encoded);
    assert(msg.src() == MY_MODEM_ID);
    assert(msg.dest() == goby::acomms::BROADCAST_ID);

    assert(msg.ack_requested() == false);

    // feed back the modem layer
    q_manager->handle_modem_receive(msg);

    assert(receive_count == 1);

    std::cout << "all tests passed" << std::endl;    
}

void handle_receive(const google::protobuf::Message &msg)
{
    std::cout << "Received: " << msg << std::endl;

    assert(test_msg1.SerializeAsString() == msg.SerializeAsString());
    
    ++receive_count;
}
