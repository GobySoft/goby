#include "goby/moos/goby_moos_app.h"
#include "config.pb.h"

class GobyMOOSAppTest : public GobyMOOSApp
{
  public:
    static GobyMOOSAppTest* get_instance();

  private:
    GobyMOOSAppTest(AppConfig& cfg) : GobyMOOSApp(&cfg)
    {
        assert(!cfg.has_submessage());
        RequestQuit();
        std::cout << "All tests passed. " << std::endl;
    }
    
    ~GobyMOOSAppTest() { }
    void loop() {}
    static GobyMOOSAppTest* inst_;    

};

boost::shared_ptr<AppConfig> master_config;
GobyMOOSAppTest* GobyMOOSAppTest::inst_ = 0;


GobyMOOSAppTest* GobyMOOSAppTest::get_instance()
{
    if(!inst_)
    {
        master_config.reset(new AppConfig);
        inst_ = new GobyMOOSAppTest(*master_config);
    }
    return inst_;
}

int main(int argc, char * argv[])
{
    return goby::moos::run<GobyMOOSAppTest>(argc, argv);
}

