// Copyright 2009-2014 Toby Schneider (https://launchpad.net/~tes)
//                     GobySoft, LLC (2013-)
//                     Massachusetts Institute of Technology (2007-2014)
//                     Goby Developers Team (https://launchpad.net/~goby-dev)
// 
//
// This file is part of the Goby Underwater Autonomy Project Binaries
// ("The Goby Binaries").
//
// The Goby Binaries are free software: you can redistribute them and/or modify
// them under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// The Goby Binaries are distributed in the hope that they will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Goby.  If not, see <http://www.gnu.org/licenses/>.


#include <iostream>

#include "goby/util/linebasedcomms.h"
#include "goby/util/as.h"

using goby::util::as;

void write_serial_tx(const std::string& s, goby::util::SerialClient& serial_client_tx)
{
    std::cout << "MM Tx [out]: " << s << std::flush;
    serial_client_tx.write(s);
}
void write_serial_rx(const std::string& s, goby::util::SerialClient& serial_client_rx)
{
    std::cout << "MM Rx [out]: " << s << std::flush;
    serial_client_rx.write(s);    
}
void write_tcp(const std::string& s, goby::util::TCPServer& tcp_server)
{
    std::cout << "TCP Client [out]: " << s << std::flush;
    tcp_server.write(s);
}

int main(int argc, char* argv[])
{

    int run_freq = 100;
    
    if(argc < 4)
    {
        std::cout << "usage: serial2tcp_server server_port serial_port_tx serial_port_rx" << std::endl;        
        return 1;
    }

    std::string server_port = argv[1];
    std::string serial_port_tx = argv[2];
    std::string serial_port_rx = argv[3];
    
    using goby::util::NMEASentence;
    
    goby::util::TCPServer tcp_server(as<unsigned>(server_port));
    goby::util::SerialClient serial_client_tx(serial_port_tx, 19200);
    goby::util::SerialClient serial_client_rx(serial_port_rx, 19200);

    tcp_server.start();
    serial_client_tx.start(); 
    serial_client_rx.start(); 

    std::string s;
    for(;;)
    {
        while(serial_client_tx.readline(&s))
        {
            std::cout << "MM Tx [in]: " << s << std::flush;
            try
            {
                NMEASentence nmea(s, NMEASentence::VALIDATE);
                write_tcp(s, tcp_server);
            }
            catch(std::exception& e)
            {
                std::cerr << "Bad NMEA sentence: " << s << std::endl;
            }
        }

        while(serial_client_rx.readline(&s))
        {
            std::cout << "MM Rx [in]: " << s << std::flush;
            try
            {
                NMEASentence nmea(s, NMEASentence::VALIDATE);
                if(nmea.sentence_id() == "CYC" ||
                   nmea.sentence_id() == "RXD" ||
                   nmea.sentence_id() == "MSG" ||
                   nmea.sentence_id() == "CST" ||
                   nmea.sentence_id() == "MUA" ||
                   nmea.sentence_id() == "MPR" ||
                   nmea.sentence_id() == "RDP" ||
                   nmea.sentence_id() == "ACK" ||
                   nmea.sentence_id() == "TMS")
                {
                    write_tcp(s, tcp_server);
                }   
            }
            catch(std::exception& e)
            {
                std::cerr << "Bad NMEA sentence: " << s << std::endl;
            }
        }
        
        while(tcp_server.readline(&s))
        {
            std::cout << "TCP Client [in]: " << s << std::flush;
            try
            {
                NMEASentence nmea(s, NMEASentence::VALIDATE);
                
                if(nmea.sentence_id() == "CFG" ||
                   nmea.sentence_id() == "CFQ" ||
                   nmea.sentence_id() == "TMS" ||
                   nmea.sentence_id() == "MSC")
                {
                    write_serial_rx(s, serial_client_rx);
                    write_serial_tx(s, serial_client_tx);
                }
                else
                {
                    write_serial_tx(s, serial_client_tx);
                }
            }
            catch(std::exception& e)
            {
                std::cerr << "Bad NMEA sentence: " << s << std::endl;
            }
        }
        
        
        usleep(1000000/run_freq);
    }
}

