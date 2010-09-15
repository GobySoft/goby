// copyright 2010 t. schneider tes@mit.edu
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
// along with this software.  If not, see <http://www.gnu.org/licenses/>


#ifndef CoreConstants20100813H
#define CoreConstants20100813H

namespace goby
{
    namespace core
    {
        const unsigned MAX_MSG_BUFFER_SIZE = 1 << 20;
        const unsigned MAX_NUM_MSG = 100;

        const std::string QUEUE_PREFIX = "goby_";
        // gobyd will listen for new application connection requests on this queue
        const std::string LISTEN_QUEUE = QUEUE_PREFIX + "connection";
        
        // prefix + application name
        // forms full queue name
        const std::string TO_SERVER_QUEUE_PREFIX = QUEUE_PREFIX + "to_gobyd_";
        const std::string FROM_SERVER_QUEUE_PREFIX = QUEUE_PREFIX + "from_gobyd_";
        
        // prefix + application name + protobuf type name
        // forms full queue name
        const std::string SUBSCRIBE_QUEUE_PREFIX = QUEUE_PREFIX + "subscribe_";
        const std::string PUBLISH_QUEUE_PREFIX = QUEUE_PREFIX + "publish_";       

    }
}

#endif
