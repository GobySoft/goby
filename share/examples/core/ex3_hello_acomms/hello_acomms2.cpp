#include "goby/core/core.h"
#include "hello_acomms.pb.h"

using goby::core::operator<<;

class HelloAcomms2 : public goby::core::ApplicationBase
{
public:
    HelloAcomms2()
        {
            // subscribe for all messages of type HelloAcommsMsg
            subscribe<HelloAcommsMsg>(&HelloAcomms2::receive_msg, this);
        }

private:
    void receive_msg(const HelloAcommsMsg& msg)
        {
            // print to the log the newest received "HelloAcommsMsg"
            glogger() << "received: " << msg << std::endl;
        }
};

int main(int argc, char* argv[])
{   
    return goby::run<HelloAcomms2>(argc, argv);
}
