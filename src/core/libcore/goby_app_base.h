// copyright 2010 t. schneider tes@mit.edu
// 
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

#ifndef GOBYAPPBASE20100908H
#define GOBYAPPBASE20100908H

#include <vector>

#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/interprocess/ipc/message_queue.hpp>
#include <boost/date_time.hpp>

#include <google/protobuf/message.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/descriptor.pb.h>

#include "goby/core/core_constants.h"

class GobyAppBase
{
  public:
    GobyAppBase(const std::string& application_name);
    virtual ~GobyAppBase();

  protected:    
    template<typename ProtoBufMessage>
        void publish(const ProtoBufMessage& msg)
    { }

    // subscribes by binding a handler to a
    // generic function object (boost::function)
    // void handler(const ProtoBufMessage& msg)
    // ProtoBufMessage is any google::protobuf::Message derivatives
    template<typename ProtoBufMessage>
        void subscribe(boost::function<void (const ProtoBufMessage&)> handler)
    {
        
    }

    // overload subscribe for member functions of a class object
    // void C::mem_func(const ProtoBufMessage& msg)    
    template<class C, typename ProtoBufMessage>
        void subscribe(void(C::*mem_func)(const ProtoBufMessage&), C* obj)
    { subscribe(boost::bind(mem_func, obj, _1)); }
    
  private:
    static boost::posix_time::time_duration MAX_CONNECTION_TIME;
    
    void connect()
    { }
    
    
    void mail_listener(const google::protobuf::Message* message_template)
    {
        

    }
    
  private:
    std::vector< boost::shared_ptr<boost::thread> > subscription_threads_;
    boost::mutex mutex_;
    std::string application_name_;
    
};


#endif
