// copyright 2010 t. schneider tes@mit.edu
// 
// this file is part of proto_test
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
#include <cmath>
#include <boost/interprocess/ipc/message_queue.hpp>
#include <boost/foreach.hpp>

#include <boost/type_traits/is_convertible.hpp>
#include <boost/utility/enable_if.hpp>

#include <google/protobuf/message.h>
#include <google/protobuf/dynamic_message.h>
#include <google/protobuf/descriptor.h>
#include "goby_double.pb.h"

#include <Wt/Dbo/Dbo>
#include <Wt/Dbo/backend/Sqlite3>

#include <ctime>
#include <map>


using namespace boost::interprocess;
namespace dbo = Wt::Dbo;

const unsigned MAX_BUFFER_SIZE = 1 << 20;

std::map<int, const google::protobuf::Descriptor*> descriptor_map;
google::protobuf::DynamicMessageFactory msg_factory;
std::map<int, std::string> dbo_map;

// A session manages persist objects
dbo::Session session;
    

void add_dbo(int i, const std::string& name);
void add_message(int i, google::protobuf::Message * msg);

template <int i>
class GobyProtoBufWrapper
{
public:
    GobyProtoBufWrapper(google::protobuf::Message* p = 0)
        : p_(p)
        {
            if(!p)
            {
                std::cout << "need to know type" << std::endl;
                p_ = msg_factory.GetPrototype(descriptor_map[i])->New();
            }            
        }

    ~GobyProtoBufWrapper()
        {
            if(p_) delete p_;
        }
    
    google::protobuf::Message& msg() const { return *p_; }
    
    
private:
    bool own_ptr_;
    google::protobuf::Message* p_;
};

namespace Wt
{
    namespace Dbo
    {
        template <typename T, typename A>
        void protobuf_message_persist(T& obj, A& action)
        {
            const google::protobuf::Descriptor* desc = obj.GetDescriptor();
            const google::protobuf::Reflection* refl = obj.GetReflection();
            
            for(int i = 0, n = desc->field_count(); i < n; ++i)
            {
                const google::protobuf::FieldDescriptor* p = desc->field(i);
                
                switch(p->cpp_type())
                {
                    default:
                        std::cout << "error" << std::endl;
                        break;
                        
                    case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE:
                        double d = refl->GetDouble(obj, p);
                        dbo::field(action, d, p->name());
                        refl->SetDouble(&obj, p, d);
                        break;
                }
            }
        }
    

        template <typename C>
        struct persist<C, typename boost::enable_if<boost::is_base_of<google::protobuf::Message, C> >::type>
        {
            template<typename A>
            static void apply(C& obj, A& action)
                { protobuf_message_persist(obj, action); }
        };

        template <>
        template <int i>
        struct persist<GobyProtoBufWrapper<i> >
        {
            template<typename A>
            static void apply(GobyProtoBufWrapper<i>& wrapper, A& action)
                { protobuf_message_persist(wrapper.msg(), action); }
        };
    }
}


int main()
{
    char buffer [MAX_BUFFER_SIZE];

    // Currentely we only have an Sqlite3 backend
    dbo::backend::Sqlite3 connection("test2.db");
    session.setConnection(connection);
    
    
    try
    {
        message_queue mq (open_only, "message_queue");
        
        unsigned int priority;
        std::size_t recvd_size;
        
        while(mq.try_receive(&buffer, MAX_BUFFER_SIZE, recvd_size, priority))
        {
            GobyDouble dbl;
            
            descriptor_map[0] = dbl.GetDescriptor();
            google::protobuf::Message* dbl_msg = msg_factory.GetPrototype(descriptor_map[0])->New();
            dbl_msg->ParseFromArray(&buffer,recvd_size);
            
            std::cout << "know what type i is" << std::endl;
            
            // Map classes to tables
            
            add_dbo(0, "A");            
            
            try { session.createTables(); }
            catch (...) 
            { }

            std::cout << "receiver: " << dbl_msg->ShortDebugString() << std::endl;
            add_message(0, dbl_msg);
            
        }
    }
    catch(interprocess_exception &ex){
        message_queue::remove("message_queue");
        std::cout << ex.what() << std::endl;
        return 1;
    }

    message_queue::remove("message_queue");
    return 0;
}

void add_dbo(int i, const std::string& name)
{
    dbo_map[i] = name;

    switch(i)
    {
        case 0: session.mapClass< GobyProtoBufWrapper<0> >(dbo_map[0].c_str()); break;
        case 1: session.mapClass< GobyProtoBufWrapper<1> >(dbo_map[1].c_str()); break;
        case 2: session.mapClass< GobyProtoBufWrapper<2> >(dbo_map[2].c_str()); break;
        case 3: session.mapClass< GobyProtoBufWrapper<3> >(dbo_map[3].c_str()); break;
    }
}


void add_message(int i, google::protobuf::Message * msg)
{
    dbo::Transaction transaction(session);
    switch(i)
    {
        case 0: session.add(new GobyProtoBufWrapper<0>(msg)); break;
        case 1: session.add(new GobyProtoBufWrapper<1>(msg)); break;
        case 2: session.add(new GobyProtoBufWrapper<2>(msg)); break;
        case 3: session.add(new GobyProtoBufWrapper<3>(msg)); break;            
    }    
    transaction.commit();
}

