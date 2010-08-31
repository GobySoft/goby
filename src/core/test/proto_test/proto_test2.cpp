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
#include <boost/preprocessor.hpp>


#include <google/protobuf/message.h>
#include <google/protobuf/dynamic_message.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/descriptor.pb.h>

#include <Wt/Dbo/Dbo>
#include <Wt/Dbo/backend/Sqlite3>

#include <ctime>
#include <map>

#define GOBY_MAX_PROTOBUF_TYPES 16

using namespace boost::interprocess;
namespace dbo = Wt::Dbo;

const unsigned MAX_BUFFER_SIZE = 1 << 20;

std::map<int, const google::protobuf::Descriptor*> descriptor_map;
google::protobuf::DynamicMessageFactory msg_factory;
google::protobuf::DescriptorPool descriptor_pool;
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
                        std::cout << "missing mapping from protobuf type to SQL type" << std::endl;
                        break;
                        
                    case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE:
                        double d = refl->GetDouble(obj, p);
                        dbo::field(action, d, p->name());
                        refl->SetDouble(&obj, p, d);
                        break;

                        // NEED SENSIBLE MAPPING FOR REMAINING PROTOBUF TYPES
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

    dbo::backend::Sqlite3 connection("test2.db");
    session.setConnection(connection);
    
    
    try
    {
        message_queue tq (open_only, "type_queue");
        message_queue mq (open_only, "message_queue");
        
        unsigned int priority;
        std::size_t recvd_size;
        const int dbl_index = 15;
        
        while(tq.try_receive(&buffer, MAX_BUFFER_SIZE, recvd_size, priority))
        {            
            google::protobuf::FileDescriptorProto proto_in;
            proto_in.ParseFromArray(&buffer,recvd_size);
            
            descriptor_pool.BuildFile(proto_in);
            const google::protobuf::Descriptor* descriptor_in = descriptor_pool.FindMessageTypeByName("GobyDouble");    
        
            descriptor_map[dbl_index] = descriptor_in;
        }
                       
        
        while(mq.try_receive(&buffer, MAX_BUFFER_SIZE, recvd_size, priority))
        {
            
            google::protobuf::Message* dbl_msg = msg_factory.GetPrototype(descriptor_map[dbl_index])->New();
            dbl_msg->ParseFromArray(&buffer,recvd_size);
            
            std::cout << "know what type i is" << std::endl;
            
            // Map classes to tables
            
            add_dbo(dbl_index, "A"); 
            
            try { session.createTables(); }
            catch (...) 
            { }

            std::cout << "receiver: " << dbl_msg->ShortDebugString() << std::endl;
            add_message(dbl_index, dbl_msg);
            
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
#define BOOST_PP_LOCAL_MACRO(n)     \
        case n: session.mapClass< GobyProtoBufWrapper<n> >(dbo_map[n].c_str()); break;
#define BOOST_PP_LOCAL_LIMITS (0, GOBY_MAX_PROTOBUF_TYPES)
#include BOOST_PP_LOCAL_ITERATE()
    }
}


void add_message(int i, google::protobuf::Message* msg)
{
    dbo::Transaction transaction(session);
    switch(i)
    {
#define BOOST_PP_LOCAL_MACRO(n)     \
        case n: session.add(new GobyProtoBufWrapper<n>(msg)); break;
#define BOOST_PP_LOCAL_LIMITS (0, GOBY_MAX_PROTOBUF_TYPES)
#include BOOST_PP_LOCAL_ITERATE()
    }    
    transaction.commit();
}

// query_message(int i)
// {
//     dbo::Transaction transaction(session);    
//     switch(i)
//     {
// #define BOOST_PP_LOCAL_MACRO(n)  \
//         case n: return *session.find< GobyProtoBufWrapper<n> >().where("value = ?").bind("24358570361")); break;
// #define BOOST_PP_LOCAL_LIMITS (0, GOBY_MAX_PROTOBUF_TYPES)
// #include BOOST_PP_LOCAL_ITERATE()
//     }
    
//     transaction.commit();
// }
