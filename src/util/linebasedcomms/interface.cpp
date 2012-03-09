
#include "interface.h"
#include "goby/common/exception.h"
            

goby::util::LineBasedInterface::LineBasedInterface(const std::string& delimiter)
    : work_(io_service_),
      active_(false)
{
    if(delimiter.empty())
        throw Exception("Line based comms started with null string as delimiter!");
    
    delimiter_ = delimiter;
    io_launcher_.reset(new IOLauncher(io_service_));
}


void goby::util::LineBasedInterface::start()
{
    if(active_) return;

    active_ = true;
    io_service_.post(boost::bind(&LineBasedInterface::do_start, this));
}

void goby::util::LineBasedInterface::clear()
{
    boost::mutex::scoped_lock lock(in_mutex_);
    in_.clear();
}

bool goby::util::LineBasedInterface::readline(protobuf::Datagram* msg, AccessOrder order /* = OLDEST_FIRST */)   
{
    if(in_.empty())
    {
        return false;
    }
    else
    {
        boost::mutex::scoped_lock lock(in_mutex_);
        switch(order)
        {
            case NEWEST_FIRST:
                msg->CopyFrom(in_.back());
                in_.pop_back(); 
                break;
                
            case OLDEST_FIRST:
                msg->CopyFrom(in_.front());
                in_.pop_front();       
                break;
        }       
        return true;
    }
}


// pass the write data via the io service in the other thread
void goby::util::LineBasedInterface::write(const protobuf::Datagram& msg)
{ io_service_.post(boost::bind(&LineBasedInterface::do_write, this, msg)); }

// call the do_close function via the io service in the other thread
void goby::util::LineBasedInterface::close()
{ io_service_.post(boost::bind(&LineBasedInterface::do_close, this, boost::system::error_code())); }
