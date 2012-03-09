
#include <iostream>

#include <boost/thread.hpp>

#include "serial_client.h"


goby::util::SerialClient::SerialClient(const std::string& name,
                                       unsigned baud,
                                       const std::string& delimiter)
    : LineBasedClient<boost::asio::serial_port>(delimiter),
      serial_port_(io_service_),
      name_(name),
      baud_(baud)
{
}

bool goby::util::SerialClient::start_specific()
{
    try
    {
        serial_port_.open(name_);
    }
    catch(std::exception& e)
    {
        serial_port_.close();
        return false;
    }
    
    serial_port_.set_option(boost::asio::serial_port_base::baud_rate(baud_));

    // no flow control
    serial_port_.set_option(boost::asio::serial_port_base::flow_control(boost::asio::serial_port_base::flow_control::none));

    // 8N1
    serial_port_.set_option(boost::asio::serial_port_base::character_size(8));
    serial_port_.set_option(boost::asio::serial_port_base::parity(boost::asio::serial_port_base::parity::none));
    serial_port_.set_option(boost::asio::serial_port_base::stop_bits(boost::asio::serial_port_base::stop_bits::one));
    
    return true;    
}

