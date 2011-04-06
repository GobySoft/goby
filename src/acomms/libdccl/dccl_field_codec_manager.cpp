// copyright 2009-2011 t. schneider tes@mit.edu
// 
// this file is part of the Dynamic Compact Control Language (DCCL),
// the goby-acomms codec. goby-acomms is a collection of libraries 
// for acoustic underwater networking
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

#include "dccl_field_codec_manager.h"

std::map<google::protobuf::FieldDescriptor::Type, goby::acomms::FieldCodecManager::InsideMap> goby::acomms::FieldCodecManager::codecs_;


boost::shared_ptr<goby::acomms::DCCLFieldCodec>
goby::acomms::FieldCodecManager::__find(google::protobuf::FieldDescriptor::Type type,
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
    
    throw(DCCLException("No codec by the name `" + codec_name + "` found for type: " + TypeHelper::find(type)->as_str()));
}


