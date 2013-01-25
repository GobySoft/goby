// Copyright 2009-2013 Toby Schneider (https://launchpad.net/~tes)
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

#include <google/protobuf/descriptor.h>

#include "goby/acomms/dccl.h"



int main(int argc, char* argv[])
{
    if(argc < 2)
    {
        std::cerr << "usage: analyze_dccl some_dccl.proto [include_path (0-n)]" << std::endl;
        exit(1);
    }

    goby::glog.add_stream(goby::common::logger::WARN, &std::cerr);
    goby::glog.set_name(argv[0]);

    goby::util::DynamicProtobufManager::enable_compilation();
    
    for(int i = 2; i < argc; ++i)
        goby::util::DynamicProtobufManager::add_include_path(argv[i]);
    
    const google::protobuf::FileDescriptor* file_desc =
        goby::util::DynamicProtobufManager::load_from_proto_file(argv[1]);

    
    goby::acomms::DCCLCodec& dccl = *goby::acomms::DCCLCodec::get();
    if(file_desc)
    {
        std::cout << "read in: " << argv[1] << std::endl;
        for(int i = 0, n = file_desc->message_type_count(); i < n; ++i)
        {
            const google::protobuf::Descriptor* desc = file_desc->message_type(i);
            
            if(desc)
            {
                try { dccl.validate(desc); }
                catch(std::exception& e)
                {
                    std::cerr << "Not a valid DCCL message: " << desc->full_name() << "\n" << e.what() << std::endl;
                }
            }
            else
            {
                std::cerr << "No descriptor with name " <<
                    file_desc->message_type(i)->full_name() << " found!" << std::endl;
                exit(1);
            }
        }
        std::cout << dccl << std::endl;
    }
    else
    {
        std::cerr << "failed to read in: " << argv[1] << std::endl;
    }    
}
