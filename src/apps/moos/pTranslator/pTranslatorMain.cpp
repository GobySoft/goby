
#include <dlfcn.h>

#include "pTranslator.h"

std::vector<void *> dl_handles;

 
int main(int argc, char* argv[])
{
    int return_value = goby::moos::run<CpTranslator>(argc, argv);

    goby::transitional::DCCLAlgorithmPerformer::deleteInstance();
    CpTranslator::delete_instance();

    goby::util::DynamicProtobufManager::protobuf_shutdown();
    for(std::vector<void *>::iterator it = dl_handles.begin(),
            n = dl_handles.end(); it != n; ++it)
        dlclose(*it);
    
    return return_value;
}
