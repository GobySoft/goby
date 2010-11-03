#include "goby/core/core.h"

class CTDSimulator : public goby::core::ApplicationBase
{

};

int main(int argc, char* argv[])
{
    return goby::run<CTDSimulator>(argc, argv);
}
