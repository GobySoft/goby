
#include <iostream>

#include "goby/util/linebasedcomms.h"
#include "goby/util/as.h"

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
        while(serial_client.readline(&s))
        {
            std::cout << "serial->tcp: " << s << std::flush;
            tcp_server.write(s);
        }
        while(tcp_server.readline(&s))
        {
            std::cout << "tcp->serial: " << s << std::flush;
            serial_client.write(s);
        }
        
        usleep(1000);
    }
}

