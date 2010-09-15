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
// along with this software.  If not, see <http://www.gnu.org/licenses/>.

#include <iostream>

#include "goby/util/linebasedcomms.h"
#include "goby/util/string.h"

using goby::util::as;

int main(int argc, char* argv[])
{
    std::string server_port;
    std::string serial_port;
    std::string serial_baud;

    if(argc != 4)
    {
        std::cout << "usage: serial2tcp_server server_port serial_port serial_baud" << std::endl;        
        return 1;
    }

    server_port = argv[1];
    serial_port = argv[2];
    serial_baud = argv[3];

    goby::util::TCPServer tcp_server(as<unsigned>(server_port));
    goby::util::SerialClient serial_client(serial_port, as<unsigned>(serial_baud));

    tcp_server.start();
    serial_client.start(); 

    for(;;)
    {
        std::string s;
        while(!(s = serial_client.readline()).empty())
        {
            std::cout << "serial->tcp: " << s << std::flush;
            tcp_server.write(s);
        }
        while(!(s = tcp_server.readline()).empty())
        {
            std::cout << "tcp->serial: " << s << std::flush;
            serial_client.write(s);
        }
        
        usleep(1000);
    }
}

