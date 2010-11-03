#include "goby/core/core.h"

class SonarSimulator : public goby::core::ApplicationBase
{

};

int main(int argc, char* argv[])
{
    return goby::run<SonarSimulator>(argc, argv);
}
