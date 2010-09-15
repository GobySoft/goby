#include "goby/core/libcore/goby_app_base.h"

class TestG : public GobyAppBase
{
public:
    TestG()
        : GobyAppBase("test_app")
        { }
    

    
};
    


int main()
{
    TestG g;
    sleep(3);
    
}
