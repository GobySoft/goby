// copyright 2011 t. schneider tes@mit.edu
// 
// this file is part of goby-acomms, a collection of libraries for acoustic underwater networking
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

#ifndef AcommsHelpers20110208H
#define AcommsHelpers20110208H

#include <string>
#include <limits>
#include <bitset>

#include <google/protobuf/descriptor.h>

#include "goby/protobuf/acomms_modem_message.pb.h"

namespace goby
{

    namespace acomms
    {            
        // provides stream output operator for Google Protocol Buffers Message 
        inline std::ostream& operator<<(std::ostream& out, const google::protobuf::Message& msg)
        {
            return (out << "[[" << msg.GetDescriptor()->name() <<"]] " << msg.DebugString());
        }
        

        
    }
}

#endif
