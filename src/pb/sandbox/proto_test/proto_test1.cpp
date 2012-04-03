// Copyright 2009-2012 Toby Schneider (https://launchpad.net/~tes)
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


#include <iostream>
#include <cmath>
#include <boost/interprocess/ipc/message_queue.hpp>
#include <boost/foreach.hpp>

#include <google/protobuf/message.h>
#include <google/protobuf/descriptor.h>
#include "goby/pb/test/proto_test/goby_double.pb.h"
#include <google/protobuf/descriptor.pb.h>

#include <Wt/Dbo/Dbo>
#include <Wt/Dbo/backend/Sqlite3>

#include <ctime>

//using namespace boost::interprocess;
using namespace google::protobuf;

// 1 megabyte
const unsigned MAX_BUFFER_SIZE = 1 << 20;    


int main()
{
    char buffer [MAX_BUFFER_SIZE];    
    
    try
    {
        boost::interprocess::message_queue::remove("type_queue");
        boost::interprocess::message_queue tq (boost::interprocess::create_only, "type_queue", 100, MAX_BUFFER_SIZE);

        
        // notify proto_test2 of the contents of the type we want to use (GobyDouble)
        google::protobuf::FileDescriptorProto proto_out;
        GobyDouble::descriptor()->file()->CopyTo(&proto_out);
        proto_out.SerializeToArray(&buffer, sizeof(buffer));
        tq.send(&buffer, proto_out.ByteSize(), 0);        


        boost::interprocess::message_queue::remove("message_queue");
        boost::interprocess::message_queue mq (boost::interprocess::create_only, "message_queue", 100, MAX_BUFFER_SIZE);
        
        GobyDouble dbl;
        dbl.set_value(time(0)*19);
        dbl.set_time(time(0));
        dbl.set_foo(16);
        
        dbl.SerializeToArray(&buffer, sizeof(buffer));
        
        std::cout << "sender: " << dbl.ShortDebugString() << std::endl;
        mq.send(&buffer, dbl.ByteSize(), 0);
    }

    catch(boost::interprocess::interprocess_exception &ex)
    {
        std::cout << ex.what() << std::endl;
        return 1;
    }
    
    
    return 0;
}
