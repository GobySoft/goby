
// credit for much of this goes to Jeff Gray-3 (http://www.nabble.com/Simple-serial-port-demonstration-with-boost-asio-asynchronous-I-O-p19849520.html)

#ifndef SerialClient20091211H
#define SerialClient20091211H

#include "client_base.h"

namespace goby
{
    namespace util
    {
        /// provides a basic client for line by line text based communications over a 8N1 tty (such as an RS-232 serial link) without flow control
        class SerialClient: public LineBasedClient<boost::asio::serial_port>
        {
          public:
            /// \brief create a serial client
            ///
            /// \param name name of the serial connection (e.g. "/dev/ttyS0")
            /// \param baud baud rate of the serial connection (e.g. 9600)
            /// \param delimiter string used to split lines
            SerialClient(const std::string& name = "",
                         unsigned baud = 9600,
                         const std::string& delimiter = "\r\n");
            
            /// set serial port name, e.g. "/dev/ttyS0"
            void set_name(const std::string& name) { name_ = name; }
            /// baud rate, e.g. 4800
            void set_baud(unsigned baud) { baud_ = baud; }

            /// serial port name, e.g. "/dev/ttyS0"
            std::string name() const { return name_; }

            /// baud rate, e.g. 4800
            unsigned baud() const { return baud_; }            

            boost::asio::serial_port& socket() { return serial_port_; }
            /// our serial port, e.g. "/dev/ttyUSB1"
            std::string local_endpoint() { return name_; }
            /// who knows where the serial port goes?! (empty string)
            std::string remote_endpoint() { return ""; }

          private:
            bool start_specific();        
  
          private:
            boost::asio::serial_port serial_port_; // the serial port this instance is connected to
            std::string name_;
            unsigned baud_;
        
        }; 
    }
}
#endif
