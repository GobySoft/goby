#include "goby/core/core.h"
#include "goby/util/time.h"
#include "test.pb.h"
#include <ctime>
#include <google/protobuf/text_format.h>
#include <google/protobuf/io/zero_copy_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <boost/unordered_map.hpp>
#include <Wt/Dbo/Query>
#include <iostream>
#include <fstream>

using goby::util::goby_time;
using goby::core::proto::Filter;

TestGConfig cfg;

namespace dbo = Wt::Dbo;

class User {
public:
  enum Role {
    Visitor = 0,
    Admin = 1,
    Alien = 42
  };

  std::string name;
  std::string password;
  Role        role;
  int         karma;

  template<class Action>
  void persist(Action& a)
  {
    dbo::field(a, name,     "name");
    dbo::field(a, password, "password");
    dbo::field(a, role,     "role");
    dbo::field(a, karma,    "karma");
  }
};

class TestG1 : public goby::core::ApplicationBase
{
public:
    TestG1()
        : goby::core::ApplicationBase("test_app1", boost::posix_time::milliseconds(1000))
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
            
            std::ifstream fin;
            fin.open("test.cfg");
            
            goby::core::proto::Config cfg_in;
            google::protobuf::io::IstreamInputStream is(&fin);
            google::protobuf::TextFormat::Parse(&is, &cfg_in);
            
            std::cout << cfg_in << std::endl;
            
            const TestGConfig cfg = cfg_in.GetExtension(test_g1);
            std::cout << "[" << cfg.svalue2() << "]" << std::endl;
            
            //subscribe(&TestG1::handler, this, make_filter("name", Filter::EQUAL, "joe"));
            //subscribe(&TestG1::handler2, this, make_filter("name", Filter::EQUAL, "bob"));

            // db_session().mapClass<TestMessage>("TestMessage");

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
            // try
            // {
            //     Wt::Dbo::Transaction transaction(db_session());
            //     Wt::Dbo::ptr<TestMessage> x = db_session().find<TestMessage>("where foo = ?").bind(11);
            //     std::cout << *x << std::endl;
                
            //     // dbo::Transaction transaction(db_session());
            //     // dbo::ptr<User> joe = db_session().find<User>(" where name = 'Joe'");
            //     // std::cerr << "Joe has karma: " << joe->karma << std::endl;
            //     transaction.commit();
            // }
            // catch(Wt::Dbo::Exception& e)
            // {}
            
            
            // static int i = 0;
            // TestMessage a;
            
            // a.set_foo(++i);
            // publish(a);
            // std::string out;
            // google::protobuf::TextFormat::PrintToString(a, &out);
            // glogger << a.GetDescriptor()->full_name() << std::endl;
            // glogger << out << std::endl;
            
//            glogger << "out: " << a << std::endl;
        }

    void handler(const TestMessage& msg)
        {
            double d = msg.foo();
            
            glogger << "in1: " << msg << std::endl;
        }

    void handler2(const TestMessage& msg)
        {
            glogger << "in2: " << msg << std::endl;
        }

};    

int main()
{
    // boost::unordered_multimap<GobyVariable, int> m;
   
    // m.insert(std::make_pair(GobyVariable("Foo", "a"),1));
    // m.insert(std::make_pair(GobyVariable("Foo"),2));
    // m.insert(std::make_pair(GobyVariable("Foo", "b"),3));
    // m.insert(std::make_pair(GobyVariable("Bar"),4));
    // m.insert(std::make_pair(GobyVariable("Bar", "a"),5));
    // m.insert(std::make_pair(GobyVariable("Baz", "c"),6));
    // m.insert(std::make_pair(GobyVariable("Quk", "c"),6));

    // typedef boost::unordered_multimap<GobyVariable,int>::iterator iterator;

    // for (iterator it = m.begin(); it != m.end(); ++it)
    //     std::cout << (*it).first.type_name << " " << (*it).first.var_name << " => " << (*it).second << std::endl;

    // std::cout << std::endl;
    
    // std::pair<iterator,iterator> p_it = m.equal_range(GobyVariable("Foo", ""));
    
    //  for (iterator it = p_it.first; it != p_it.second; ++it)
    //      std::cout << (*it).first.type_name << " " << (*it).first.var_name << " => " << (*it).second << std::endl;
    
    
    TestG1 g1;
    g1.run();
}

