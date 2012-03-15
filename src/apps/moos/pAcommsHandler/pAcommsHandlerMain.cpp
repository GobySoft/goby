// Copyright 2009-2012 Toby Schneider (https://launchpad.net/~tes)
//                     Massachusetts Institute of Technology (2007-)
//                     Woods Hole Oceanographic Institution (2007-)
//                     Goby Developers Team (https://launchpad.net/~goby-dev)
// 
//
// This file is part of the Goby Underwater Autonomy Project Binaries
// ("The Goby Binaries").
//
// The Goby Binaries are free software: you can redistribute them and/or modify
// them under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// The Goby Binaries are distributed in the hope that they will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Goby.  If not, see <http://www.gnu.org/licenses/>.

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
