#include <boost/filesystem.hpp>

#include <google/protobuf/compiler/importer.h>
#include <google/protobuf/descriptor.h>

#include "goby/acomms/dccl.h"

#if BOOST_FILESYSTEM_VERSION == 3
namespace bf = boost::filesystem3;
#else
namespace bf = boost::filesystem;
#endif

google::protobuf::compiler::DiskSourceTree disk_source_tree_;

class ErrorCollector: public google::protobuf::compiler::MultiFileErrorCollector
{
    void AddError(const std::string & filename, int line, int column, const std::string & message)
        {
            std::cerr << "File: " << filename << " has error (line: " << line << ", column: " << column << "): " << message << std::endl;
        }
};

ErrorCollector error_collector_;
google::protobuf::compiler::SourceTreeDescriptorDatabase source_database_(&disk_source_tree_);
google::protobuf::DescriptorPoolDatabase generated_database_(*google::protobuf::DescriptorPool::generated_pool());
google::protobuf::MergedDescriptorDatabase merged_database_(&generated_database_, &source_database_);
google::protobuf::DescriptorPool descriptor_pool_(&merged_database_);


int main(int argc, char* argv[])
{
    if(argc != 2)
    {
        std::cerr << "usage: analyze_dccl some_dccl.proto" << std::endl;
        exit(1);
    }

    goby::glog.add_stream(goby::common::logger::VERBOSE, &std::cerr);
    goby::glog.set_name(argv[0]);

    
    source_database_.RecordErrorsTo(&error_collector_);
    disk_source_tree_.MapPath("/", "/");

    bf::path proto_file_path = bf::complete(argv[1]);
    proto_file_path.normalize();

    google::protobuf::FileDescriptorProto file_proto;

    std::string full_proto_path = proto_file_path.string();
    source_database_.FindFileByName(full_proto_path, &file_proto);

    std::cout << "read in: " << full_proto_path << std::endl;

    
    goby::acomms::DCCLCodec& dccl = *goby::acomms::DCCLCodec::get();


    const google::protobuf::FileDescriptor* file_desc = descriptor_pool_.FindFileByName(full_proto_path);    

    if(file_desc)
    {
        for(int i = 0, n = file_proto.message_type_size(); i < n; ++i)
        {
            const google::protobuf::Descriptor* desc = file_desc->FindMessageTypeByName(file_proto.message_type(i).name());

            if(desc)
                dccl.validate(desc);
            else
            {
                std::cerr << "No descriptor with name " <<  file_proto.message_type(i).name() << " found!" << std::endl;
                exit(1);
            }
        }
    }
    
    std::cout << dccl << std::endl;
}
