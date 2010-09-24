#include "goby/core/core.h"
#include "goby/util/time.h"
#include "test.pb.h"
#include <ctime>
#include <google/protobuf/text_format.h>

using goby::util::goby_time;

class TestG2 : public goby::core::ApplicationBase
{
public:
    TestG2()
        : goby::core::ApplicationBase("test_app2", boost::posix_time::milliseconds(200))
        {
            // int i = 0;

            // google::protobuf::TextFormat::Printer printer;
            // printer.SetSingleLineMode(true);
            
            // TestMessage b;
            // b.set_foo(++i);
            // b.set_name("bob");
            // b.set_evalue(TestMessage::YES);
            // b.mutable_subfoo()->set_baz(i);
            // b.add_rstr("fe\n\ne");
            // b.add_rstr("fie");
            // b.add_rstr("foe");
            // //publish(b);
            // std::string str;
            // printer.PrintToString(b, &str);
            // std::cout << str << std::endl;

            // TestMessage c;
            // google::protobuf::TextFormat::ParseFromString(str, &c);
            // std::cout << c << std::endl;
            
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
            b.set_name("bob");
            b.set_evalue(TestMessage::YES);
                {
                    Sub* p = b.mutable_subfoo();
                    p->set_baz(i);
                    p->set_bar(2*i);
                    p->add_rint(1);
                    p->add_rint(2);
                    p->add_rint(3);
                    p->add_rint(i);

                }
                // {
                //     Sub* p = b.add_subfoo();
                //     p->set_baz(i+1);
                //     p->set_bar(2*(i+1));
                // }
                // {
                //     Sub* p = b.add_subfoo();
                //     p->set_baz(i);
                //     p->set_bar(2*(i+1));
                // }
                
            
            b.add_rstr("fee");
            b.add_rstr("fie");
            b.add_rstr("foe");
            publish(b);
            glogger << "out: " << b << std::endl;
        }
    
};

int main()
{
    TestG2 g2;
    g2.run();
}
