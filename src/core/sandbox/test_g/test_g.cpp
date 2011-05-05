#include "goby/core/libcore/goby_app_base.h"
#include "goby/util/time.h"
#include "test.pb.h"
#include <ctime>
#include <google/protobuf/text_format.h>

using goby::util::goby_time;

TestGConfig cfg;

class TestG1 : public GobyAppBase
{
public:
    TestG1()
        : GobyAppBase("test_app1")
        {
            TestMessage a;

            std::cout << goby_time() << ": begin" << std::endl;

            for(int i = 0; i < 10000; ++i)
            {
                a.set_foo(i);
                publish(a);
                // std::string out;
                // google::protobuf::TextFormat::PrintToString(a, &out);
                // glogger << a.GetDescriptor()->full_name() << std::endl;
                // glogger << out << std::endl;
            }
        }
};    

class TestG2 : public GobyAppBase
{
public:
    TestG2()
        : GobyAppBase("test_app2")
        {
            subscribe(&TestG2::handler, this);
        }
    
    void handler(const TestMessage& msg)
        {
            if(msg.foo() == 9999)
                std::cout << goby_time() << ": end" << std::endl;
        }
};

int main()
{
    TestG1 g1;
    g1.run();
}
