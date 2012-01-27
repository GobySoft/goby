// t. schneider tes@mit.edu 11.20.09
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


#include "goby/acomms/dccl/dccl.h"
#include "goby/moos/transitional/dccl_transitional.h"

#include <iostream>
#include <cassert>

using namespace goby;
using goby::acomms::operator<<;
using goby::transitional::operator<<;

int main(int argc, char* argv[])
{
    goby::glog.add_stream(goby::common::logger::DEBUG3, &std::cerr);
    goby::glog.set_name(argv[0]);
    
    std::cout << "loading xml file: " << DCCL_EXAMPLES_DIR "transitional1/test.xml" << std::endl;

    google::protobuf::compiler::DiskSourceTree disk_source_tree;
    google::protobuf::compiler::SourceTreeDescriptorDatabase source_database(&disk_source_tree);

    class TranslatorErrorCollector: public google::protobuf::compiler::MultiFileErrorCollector
    {
        void AddError(const std::string & filename, int line, int column, const std::string & message)
        {
            goby::glog.is(goby::common::logger::DIE) &&
                goby::glog << "File: " << filename
                           << " has error (line: " << line << ", column: " << column << "): "
                           << message << std::endl;
        }       
    };
                
    TranslatorErrorCollector error_collector;    

    source_database.RecordErrorsTo(&error_collector);
    disk_source_tree.MapPath("/", "/");
    goby::util::DynamicProtobufManager::add_database(&source_database);
    
    // instantiate the parser with a single xml file
    goby::transitional::DCCLTransitionalCodec transitional_dccl;  
    pAcommsHandlerConfig cfg;
    cfg.mutable_transitional_cfg()->add_message_file()->
        set_path(DCCL_EXAMPLES_DIR "/transitional1/test.xml");

    transitional_dccl.convert_to_v2_representation(&cfg);

    std::cout << cfg.DebugString() << std::endl;
    
    std::cout << "all tests passed" << std::endl;
}
