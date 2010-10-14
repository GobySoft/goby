#include "goby/core/core.h"
#include "hello_world.pb.h"

class HelloWorld2 : public goby::core::ApplicationBase
{
public:
    HelloWorld2()
        {
            // subscribe for all messages of type HelloWorldMsg
            subscribe<HelloWorldMsg>();
        }

private:
    void loop()
        {
            // print to the log the newest received "HelloWorldMsg"
            glogger() << "newest message is: " << newest<HelloWorldMsg>() << std::endl;
        }
};

int main(int argc, char* argv[])
{   
    return goby::run<HelloWorld2>(argc, argv);
}
