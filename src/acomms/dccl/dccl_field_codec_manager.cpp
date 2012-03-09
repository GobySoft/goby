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


