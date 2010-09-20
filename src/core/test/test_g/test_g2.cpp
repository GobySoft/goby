#include "goby/core/libcore/goby_app_base.h"
#include "goby/util/time.h"
#include "test.pb.h"
#include <ctime>
#include <google/protobuf/text_format.h>

using goby::util::goby_time;

class TestG2 : public GobyAppBase
{
public:
    TestG2()
        : GobyAppBase("test_app2", boost::posix_time::seconds(1))
        {
            subscribe(&TestG2::handler, this);
        }
    
    void handler(const TestMessage& msg)
        {
            glogger << "in: " << msg << std::endl;
        }
    void loop()
        {
            static int i = 0;
            TestMessage b;

            b.set_foo(++i);
            publish(b, "joe");
            glogger << "out: " << b << std::endl;
        }
    
};

int main()
{
    TestG2 g2;
    g2.run();
}
