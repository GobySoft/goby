// t. schneider tes@mit.edu 06.05.08
// ocean engineering graudate student - mit / whoi joint program
// massachusetts institute of technology (mit)
// laboratory for autonomous marine sensing systems (lamss)
// 
// this is pTranslatorMain.cpp, part of pTranslator
//
// see the readme file within this directory for information
// pertaining to usage and purpose of this script.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This software is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this software.  If not, see <http://www.gnu.org/licenses/>.

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
