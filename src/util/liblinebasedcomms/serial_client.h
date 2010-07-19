// copyright 2009, 2010 t. schneider tes@mit.edu
// 
// this file is part of comms, a library for handling various communications.
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

// credit for much of this goes to Jeff Gray-3 (http://www.nabble.com/Simple-serial-port-demonstration-with-boost-asio-asynchronous-I-O-p19849520.html)

#ifndef SerialClient20091211H
#define SerialClient20091211H

#include "client_base.h"

namespace goby
{
    namespace util
    {    
        class SerialClient: public LineBasedClient<asio::serial_port>
        {
          public:
            static SerialClient* get_instance(unsigned& clientkey,
                                              const std::string& name,
                                              unsigned baud,
                                              const std::string& delimiter = "\r\n");

          private:
            SerialClient(const std::string& name,
                         unsigned baud,
                         const std::string& delimiter);

            bool start_specific();        
  
          private:
            static std::map<std::string, SerialClient*> inst_;
            asio::serial_port serial_port_; // the serial port this instance is connected to
            std::string name_;
            unsigned baud_;
        
        }; 
    }
}
#endif
