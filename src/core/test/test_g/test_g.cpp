#include "goby/core/libcore/goby_app_base.h"

#include "test.pb.h"
#include <ctime>

class TestG1 : public GobyAppBase
{
public:
    TestG1()
        : GobyAppBase("test_app1")
        {
            TestMessage a;

            for(int i = 0; i < 100; ++i)
            {
                a.set_foo(i);
                publish(a);
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
            glogger_ << msg.ShortDebugString() << std::endl;
        }
};

int main()
{
    TestG2 g2;
    TestG1 g1;
    sleep(5);
}
