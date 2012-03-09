
#include "dccl_field_codec_manager.h"

std::map<google::protobuf::FieldDescriptor::Type, goby::acomms::DCCLFieldCodecManager::InsideMap> goby::acomms::DCCLFieldCodecManager::codecs_;


boost::shared_ptr<goby::acomms::DCCLFieldCodecBase>
goby::acomms::DCCLFieldCodecManager::__find(google::protobuf::FieldDescriptor::Type type,
                                                 const std::string& codec_name,
                                                 const std::string& type_name /* = "" */)
{
    typedef InsideMap::const_iterator InsideIterator;
    typedef std::map<google::protobuf::FieldDescriptor::Type, InsideMap>::const_iterator Iterator;
    
    Iterator it = codecs_.find(type);
    if(it != codecs_.end())
    {
        InsideIterator inside_it = it->second.end();
        // try specific type codec
        inside_it = it->second.find(__mangle_name(codec_name, type_name));
        if(inside_it != it->second.end())
            return inside_it->second;
        
        // try general 
        inside_it = it->second.find(codec_name);
        if(inside_it != it->second.end())
            return inside_it->second;
    }
    
    throw(DCCLException("No codec by the name `" + codec_name + "` found for type: " + DCCLTypeHelper::find(type)->as_str()));
}


