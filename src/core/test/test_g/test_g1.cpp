#include "goby/core/core.h"
#include "goby/util/time.h"
#include "test.pb.h"
#include <ctime>
#include <google/protobuf/text_format.h>
#include <google/protobuf/io/zero_copy_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <boost/unordered_map.hpp>
#include <Wt/Dbo/Query>
#include <Wt/Dbo/backend/Sqlite3>
#include <Wt/Dbo/Exception>
#include <iostream>
#include <fstream>

using goby::util::goby_time;
using goby::core::proto::Filter;

namespace dbo = Wt::Dbo;

// class User {
// public:
//   enum Role {
//     Visitor = 0,
//     Admin = 1,
//     Alien = 42
//   };

//   std::string name;
//   std::string password;
//   Role        role;
//   int         karma;

//   template<class Action>
//   void persist(Action& a)
//   {
//     dbo::field(a, name,     "name");
//     dbo::field(a, password, "password");
//     dbo::field(a, role,     "role");
//     dbo::field(a, karma,    "karma");
//   }
// };

class TestG1 : public goby::core::ApplicationBase
{
public:
    TestG1() : goby::core::ApplicationBase(&cfg_)
        {
            
            // MetaConfig cfg_out;
            
            // TestGConfig* cfg = cfg_out.MutableExtension(TestGConfig::testg_ext);
            // cfg->set_dvalue1(3.21);
            // cfg->set_svalue2("hi");
            // cfg->set_evalue3(TestGConfig::YES);
            
            // std::ofstream fout;
            // fout.open("test.cfg");
            //     {
            //         google::protobuf::io::OstreamOutputStream os(&fout);
            //         google::protobuf::TextFormat::Print(cfg_out, &os);
            //     }

            // fout.close();
            
            glogger() << cfg_ << std::endl;
            
            
            //subscribe(&TestG1::handler, this, make_filter("name", Filter::EQUAL, "joe"));
            //subscribe(&TestG1::handler2, this, make_filter("name", Filter::EQUAL, "bob"));

            db_session().mapClass<TestMessage>("TestMessage");

            // db_session().mapClass<User>("user");
            
            // dbo::Transaction transaction(db_session());
            
            // User *user = new User();
            // user->name = "Joe";
            // user->password = "Secret";
            // user->role = User::Visitor;
            // user->karma = 13;
            
            // dbo::ptr<User> userPtr = db_session().add(user);

            
            // try {
            //     db_session().createTables();
            // } catch (...) { }
            
            // transaction.commit();
        }

private:
    void loop()
        {    
            try
            {
                Wt::Dbo::Transaction transaction(db_session());
                Wt::Dbo::ptr<TestMessage> x = db_session().find<TestMessage>("where foo = ?").bind(11);
                if(x) std::cout << *x << std::endl;
                
                transaction.commit();
            }
            catch(Wt::Dbo::Exception& e)
            {
                glogger() << warn << e.what() << std::endl;
            }
            
            // static int i = 0;
            // TestMessage a;
            
            // a.set_foo(++i);
            // publish(a);
            // std::string out;
            // google::protobuf::TextFormat::PrintToString(a, &out);
            // glogger() << a.GetDescriptor()->full_name() << std::endl;
            // glogger() << out << std::endl;
            
//            glogger() << "out: " << a << std::endl;
        }

    void handler(const TestMessage& msg)
        {
            double d = msg.foo();
            
            glogger() << "in1: " << msg << std::endl;
        }

    void handler2(const TestMessage& msg)
        {
            glogger() << "in2: " << msg << std::endl;
        }

    // needs to be static so it gets constructed before TestG1
    static TestGConfig cfg_;
};    

TestGConfig TestG1::cfg_;


int main(int argc, char* argv[])
{
    return goby::run<TestG1>(argc, argv);
}
