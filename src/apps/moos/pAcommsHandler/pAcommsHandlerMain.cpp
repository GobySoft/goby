
#include <dlfcn.h>

#include "pAcommsHandler.h"

std::vector<void *> dl_handles;

int main(int argc, char* argv[])
{
    goby::glog.add_group("pAcommsHandler", goby::common::Colors::yellow);
    int return_value = goby::moos::run<CpAcommsHandler>(argc, argv);

    goby::transitional::DCCLAlgorithmPerformer::deleteInstance();
    CpAcommsHandler::delete_instance();

    goby::util::DynamicProtobufManager::protobuf_shutdown();
    for(std::vector<void *>::iterator it = dl_handles.begin(),
            n = dl_handles.end(); it != n; ++it)
        dlclose(*it);
    
    return return_value;
}
