
#include "goby/moos/transitional/dccl_transitional.h"

int main(int argc, char* argv[])
{
    std::string xml_file;
    std::string proto_folder = "";
    
    switch(argc)
    {
        case 3:
            xml_file = argv[1];
            proto_folder = argv[2];
        
        case 2:
            xml_file = argv[1];
            break;

        default:
            std::cerr << "usage: dccl_xml_to_dccl_proto message_xml_file.xml [directory for generated .proto (default = pwd (.)]" << std::endl;
            exit(EXIT_FAILURE);
    }

    goby::glog.add_stream(goby::common::protobuf::GLogConfig::VERBOSE, &std::cerr);
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

    
    std::cerr << "creating DCCLTransitionalCodec using xml file: [" << xml_file << "]" << std::endl;

    source_database.RecordErrorsTo(&error_collector);
    disk_source_tree.MapPath("/", "/");
    goby::util::DynamicProtobufManager::add_database(&source_database);

    
    goby::transitional::DCCLTransitionalCodec dccl;

    pAcommsHandlerConfig cfg;    
    cfg.mutable_transitional_cfg()->add_message_file()->set_path(xml_file);
    cfg.mutable_transitional_cfg()->set_generated_proto_dir(proto_folder);
    dccl.convert_to_v2_representation(&cfg);

    
    
    std::cout << "received: " << cfg.DebugString();
    
    std::cerr << "wrote proto files" << std::endl;
}
