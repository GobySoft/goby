#include <iostream>
#include <string>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <asio.hpp>
#include <set>
#include <deque>
#include <boost/thread.hpp>
#include <boost/foreach.hpp>

#include "base.h"

using asio::ip::tcp;


namespace goby
{
    namespace util
    {
        class TCPConnection : public boost::enable_shared_from_this<TCPConnection>,
            public ASIOAsyncConnection<tcp::socket>
            {
              public:
                typedef boost::shared_ptr<TCPConnection> pointer;
    
                static pointer create(asio::io_service& io_service,
                                      std::vector< std::deque<std::string> >& in,
                                      boost::mutex& in_mutex,
                                      const std::string& delimiter)
                {
                    return pointer(new TCPConnection(io_service, in, in_mutex, delimiter));
                }

                tcp::socket& socket()
                { return socket_; }
    
                void start()
                { socket_.get_io_service().post(boost::bind(&TCPConnection::read_start, this)); }

                void write(const std::string& msg)
                {
                    socket_.get_io_service().post(boost::bind(&TCPConnection::socket_write, this, msg));
                }    

                void close(const asio::error_code& error)
                { socket_.get_io_service().post(boost::bind(&TCPConnection::socket_close, this, error)); }    
        
              private:
                void socket_write(const std::string& line) // give it a string
                { // callback to handle write call from outside this class
                    bool write_in_progress = !out_.empty(); // is there anything currently being written?
                    out_.push_back(line); // store in write buffer
                    if (!write_in_progress) // if nothing is currently being written, then start
                        write_start();
                }    

                void socket_close(const asio::error_code& error)
                { // something has gone wrong, so close the socket & make this object inactive
                    if (error == asio::error::operation_aborted) // if this call is the result of a timer cancel()
                        return; // ignore it because the connection cancelled the timer

                    if(error)
                        socket_.close();
                }
    
    
              TCPConnection(asio::io_service& io_service,
                            std::vector< std::deque<std::string> >& in,
                            boost::mutex& in_mutex,
                            const std::string& delimiter)
                  : ASIOAsyncConnection<tcp::socket>(socket_, out_, in, in_mutex, delimiter),
                    socket_(io_service)
                    { }
    

              private:
                tcp::socket socket_;
                std::deque<std::string> out_; // buffered write data
    
            };


        class TCPServer : public ASIOLineBasedInterface
        {
          public:    
            static TCPServer* get_instance(unsigned& clientkey,
                                           unsigned port,
                                           const std::string& delimiter = "\r\n")
            {
                std::string port_str = boost::lexical_cast<std::string>(port);
        
                if(!inst_.count(port_str))
                {
                    inst_[port_str] = new TCPServer(port_str, delimiter);
                    clientkey = 0;
                }
                else
                    clientkey = inst_[port_str]->add_user();
        
                return(inst_[port_str]);
            }
    
          private:
          TCPServer(const std::string& port,
                    const std::string& delimiter = "\r\n")
              : acceptor_(io_service_, tcp::endpoint(tcp::v4(), 9000)),
                delimiter_(delimiter)
                {  }
    
    
            void do_start()
            { start_accept(); }
        
            void do_write(const std::string& line)
            {
                BOOST_FOREACH(TCPConnection::pointer c, connections_)
                    c->write(line);
            }
    
            void do_close(const asio::error_code& error)
            {
                BOOST_FOREACH(TCPConnection::pointer c, connections_)
                    c->close(error);
            }    
    
          private:
            void start_accept()
            {
                new_connection_ = TCPConnection::create(acceptor_.io_service(), in_, in_mutex_, delimiter_);
                acceptor_.async_accept(new_connection_->socket(),
                                       boost::bind(&TCPServer::handle_accept, this, new_connection_, asio::placeholders::error));
            }

            void handle_accept(TCPConnection::pointer new_connection,
                               const asio::error_code& error)
            {
                if (!error)
                {
                    new_connection_->start();
                    connections_.insert(new_connection_);
                    start_accept();
                }
            }

          private:
            static std::map<std::string, TCPServer*> inst_;
            std::string server_;
            std::string port_;
            tcp::acceptor acceptor_;
            TCPConnection::pointer new_connection_;
            std::set<TCPConnection::pointer> connections_;
            std::string delimiter_;
    
        };
        std::map<std::string, goby::util::TCPServer*> goby::util::TCPServer::inst_;

    }

}


