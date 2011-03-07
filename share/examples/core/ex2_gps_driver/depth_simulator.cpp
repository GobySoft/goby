#include <cstdlib> // for rand

#include "goby/core/core.h"

#include "config.pb.h"
#include "depth_reading.pb.h"

using goby::core::operator<<;

class DepthSimulator : public goby::core::ApplicationBase
{
public:
    DepthSimulator()
        : goby::core::ApplicationBase(&cfg_)
        { }

    void loop()
        {
            DepthReading reading;
            // just post the depth given in the configuration file plus a
            // small random offset
            reading.set_depth(cfg_.depth() + (rand() % 10) / 10.0);

            glogger() << reading << std::flush;
            publish(reading);    
        }

    static DepthSimulatorConfig cfg_;
};

DepthSimulatorConfig DepthSimulator::cfg_;

int main(int argc, char* argv[])
{
    return goby::run<DepthSimulator>(argc, argv);
}

