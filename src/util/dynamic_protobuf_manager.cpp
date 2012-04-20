// Copyright 2009-2012 Toby Schneider (https://launchpad.net/~tes)
//                     Massachusetts Institute of Technology (2007-)
//                     Woods Hole Oceanographic Institution (2007-)
//                     Goby Developers Team (https://launchpad.net/~goby-dev)
// 
//
// This file is part of the Goby Underwater Autonomy Project Libraries
// ("The Goby Libraries").
//
// The Goby Libraries are free software: you can redistribute them and/or modify
// them under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// The Goby Libraries are distributed in the hope that they will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with Goby.  If not, see <http://www.gnu.org/licenses/>.


#include "dynamic_protobuf_manager.h"

#include "goby/common/exception.h"
#include "goby/common/logger.h"

boost::shared_ptr<goby::util::DynamicProtobufManager> goby::util::DynamicProtobufManager::inst_;

boost::signal<void (const google::protobuf::FileDescriptor*)> goby::util::DynamicProtobufManager::new_descriptor_hooks;

const google::protobuf::FileDescriptor* goby::util::DynamicProtobufManager::add_protobuf_file(const google::protobuf::FileDescriptorProto& proto)
{
    simple_database().Add(proto);
    
    const google::protobuf::FileDescriptor* return_desc = user_descriptor_pool().FindFileByName(proto.name());
    new_descriptor_hooks(return_desc);
    return return_desc; 
}

void goby::util::DynamicProtobufManager::enable_disk_source_database()
{
    if(disk_source_tree_)
        return;
    
    disk_source_tree_ = new google::protobuf::compiler::DiskSourceTree;
    source_database_ = new google::protobuf::compiler::SourceTreeDescriptorDatabase(disk_source_tree_);
    error_collector_ = new GLogMultiFileErrorCollector;
    
    source_database_->RecordErrorsTo(error_collector_);
    disk_source_tree_->MapPath("/", "/");
    add_database(source_database_);
}

// GLogMultiFileErrorCollector

void goby::util::DynamicProtobufManager::GLogMultiFileErrorCollector::AddError(const std::string & filename, int line, int column,
              const std::string & message)
{
    goby::glog.is(goby::common::logger::DIE) &&
        goby::glog << "File: " << filename
                   << " has error (line: " << line << ", column: "
                   << column << "): "
                   << message << std::endl;
}
