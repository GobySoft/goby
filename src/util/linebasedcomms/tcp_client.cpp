
#include "goby/util/as.h"

#include "tcp_client.h"

goby::util::TCPClient::TCPClient(const std::string& server,
                                 unsigned port,
                                 const std::string& delimiter /*= "\r\n"*/)
    : LineBasedClient<boost::asio::ip::tcp::socket>(delimiter),
      socket_(io_service_),
      server_(server),
      port_(as<std::string>(port))
{ }


bool goby::util::TCPClient::start_specific()
{
    boost::asio::ip::tcp::resolver resolver(io_service_);
    boost::asio::ip::tcp::resolver::query query(server_, port_);
    boost::asio::ip::tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
    boost::asio::ip::tcp::resolver::iterator end;

    boost::system::error_code error = boost::asio::error::host_not_found;
    while (error && endpoint_iterator != end)
    {
        socket_.close();
        socket_.connect(*endpoint_iterator++, error);
    }
    
    return error ? false : true;
}

