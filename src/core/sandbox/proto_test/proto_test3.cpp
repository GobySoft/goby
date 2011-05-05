// copyright 2010 t. schneider tes@mit.edu
// 
// this file is part of proto_test
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

#include <iostream>
#include <cmath>
#include <boost/interprocess/ipc/message_queue.hpp>

#include "goby_double.pb.h"

//using namespace boost::interprocess;

// 1 megabyte
const unsigned MAX_BUFFER_SIZE = 1 << 20;

int main()
{
    char buffer [MAX_BUFFER_SIZE];
    
    GobyDouble dbl;
    dbl.set_value(2);
    std::cout << dbl.ShortDebugString() << std::endl;
    dbl.SerializeToArray(&buffer, sizeof(buffer));
    
    try
    {
        boost::interprocess::message_queue mq (boost::interprocess::open_only, "message_queue");
        mq.send(&buffer, dbl.ByteSize(), 0);
    }

    catch(boost::interprocess::interprocess_exception &ex)
    {
       std::cout << ex.what() << std::endl;
       return 1;
    }
    
   return 0;
}
