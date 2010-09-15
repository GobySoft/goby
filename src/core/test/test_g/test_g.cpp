#include "goby/core/libcore/goby_app_base.h"

#include "test.pb.h"

class TestG : public GobyAppBase
{
public:
    TestG()
        : GobyAppBase("test_app")
        {
            subscribe(&TestG::handler, this);
        }

    void handler(const TestMessage& msg)
        {
        }
    

};
    


int main()
{
    TestG g;
    sleep(3);
    
}
