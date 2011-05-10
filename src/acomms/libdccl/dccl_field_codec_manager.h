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

#ifndef DCCLFieldCodecManager20110405H
#define DCCLFieldCodecManager20110405H

#include "dccl_type_helper.h"
#include "dccl_field_codec.h"

namespace goby
{
    namespace acomms
    {

        /// \todo (tes) Make sanity check for newly added FieldCodecs
        class DCCLFieldCodecManager
        {
          public:
            // field type == wire type
            template<typename FieldType, template <typename FieldType> class Codec>
                static void add(const std::string& name);
            
            template<typename FieldType, class Codec>
                typename boost::enable_if<
                boost::mpl::and_<boost::is_base_of<google::protobuf::Message, FieldType>,
                boost::mpl::not_<boost::is_same<google::protobuf::Message, FieldType> >
                >,
                void>::type 
                static add(const std::string& name);
                
            template<typename FieldType, class Codec>
                typename boost::disable_if<
                boost::mpl::and_<boost::is_base_of<google::protobuf::Message, FieldType>,
                boost::mpl::not_<boost::is_same<google::protobuf::Message, FieldType> >
                >,
                void>::type
                static add(const std::string& name);
                
            template<google::protobuf::FieldDescriptor::Type type, class Codec>
                static void add(const std::string& name);


            // field type != wire type
            template<typename FieldType, typename WireType, class Codec>
                static void add(const std::string& name);
            
            template<typename FieldType, typename WireType,
                template <typename FieldType, typename WireType> class Codec>
                static void add(const std::string& name);

            
            static boost::shared_ptr<DCCLFieldCodecBase> find(
                const google::protobuf::FieldDescriptor* field,
                const std::string& name)
            {
                if(field->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE)
                    return find(field->message_type(), name);
                else
                    return __find(field->type(), name);
            }
                

            static boost::shared_ptr<DCCLFieldCodecBase> find(
                const google::protobuf::Descriptor* desc,
                const std::string& name)
            {
                return __find(google::protobuf::FieldDescriptor::TYPE_MESSAGE,
                              name, desc->full_name());
            }
                
            
          private:
            DCCLFieldCodecManager() { }
            ~DCCLFieldCodecManager() { }
            DCCLFieldCodecManager(const DCCLFieldCodecManager&);
            DCCLFieldCodecManager& operator= (const DCCLFieldCodecManager&);

            
            static boost::shared_ptr<DCCLFieldCodecBase> __find(
                google::protobuf::FieldDescriptor::Type type,
                const std::string& codec_name,
                const std::string& type_name = "");
            
            static std::string __mangle_name(const std::string& codec_name,
                                      const std::string& type_name) 
            { return type_name.empty() ? codec_name : codec_name + "@@" + type_name; }
                
            template<class Codec>
                static void __add(const std::string& name,
                                  google::protobuf::FieldDescriptor::Type field_type,
                                  google::protobuf::FieldDescriptor::CppType wire_type);
          private:
            
            typedef std::map<std::string, boost::shared_ptr<DCCLFieldCodecBase> > InsideMap;
            static std::map<google::protobuf::FieldDescriptor::Type, InsideMap> codecs_;
        };
    }
}

template<typename FieldType, template <typename FieldType> class Codec>
    void goby::acomms::DCCLFieldCodecManager::add(const std::string& name)
{
    add<FieldType, Codec<FieldType> >(name);
}


template<typename FieldType, class Codec>
    typename boost::enable_if<
    boost::mpl::and_<
    boost::is_base_of<google::protobuf::Message, FieldType>,
    boost::mpl::not_<boost::is_same<google::protobuf::Message, FieldType> >
    >,
    void>::type 
    goby::acomms::DCCLFieldCodecManager::add(const std::string& name)
{
    DCCLTypeHelper::add<FieldType>();
    __add<Codec>(__mangle_name(name, FieldType::descriptor()->full_name()),
                 google::protobuf::FieldDescriptor::TYPE_MESSAGE,
                 google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE);
}

template<typename FieldType, class Codec>
    typename boost::disable_if<
    boost::mpl::and_<
    boost::is_base_of<google::protobuf::Message, FieldType>,
    boost::mpl::not_<boost::is_same<google::protobuf::Message, FieldType> >
    >,
    void>::type
    goby::acomms::DCCLFieldCodecManager::add(const std::string& name)
{
    add<FieldType, FieldType, Codec>(name);
}

template<google::protobuf::FieldDescriptor::Type type, class Codec>
    void goby::acomms::DCCLFieldCodecManager::add(const std::string& name)
{
    __add<Codec>(name, type, google::protobuf::FieldDescriptor::TypeToCppType(type));
}


template<typename FieldType, typename WireType,
    template <typename FieldType, typename WireType> class Codec>
    void goby::acomms::DCCLFieldCodecManager::add(const std::string& name)
{
    add<FieldType, WireType, Codec<FieldType, WireType> >(name);
}


template<typename FieldType, typename WireType, class Codec>
    void goby::acomms::DCCLFieldCodecManager::add(const std::string& name)
{
    using google::protobuf::FieldDescriptor;
    const FieldDescriptor::CppType cpp_field_type = ToProtoCppType<FieldType>::as_enum();
    const FieldDescriptor::CppType cpp_wire_type = ToProtoCppType<WireType>::as_enum();

    for(int i = 1, n = FieldDescriptor::MAX_TYPE; i <= n; ++i)
    {
        FieldDescriptor::Type field_type = static_cast<FieldDescriptor::Type>(i);
        if(FieldDescriptor::TypeToCppType(field_type) == cpp_field_type)
        {            
            __add<Codec>(name, field_type, cpp_wire_type);
        }
    }
}

template<class Codec>
void goby::acomms::DCCLFieldCodecManager::__add(const std::string& name,
                                                google::protobuf::FieldDescriptor::Type field_type,
                                                google::protobuf::FieldDescriptor::CppType wire_type)
{
    using google::protobuf::FieldDescriptor;    
    if(!codecs_[field_type].count(name))
    {
        boost::shared_ptr<DCCLFieldCodecBase> new_field_codec(new Codec());
        new_field_codec->set_name(name);
        new_field_codec->set_field_type(field_type);
        new_field_codec->set_wire_type(wire_type);
        
        codecs_[field_type][name] = new_field_codec;
        DCCLCommon::logger() << "Adding codec " << *new_field_codec << std::endl;
    }            
    else
    {
        boost::shared_ptr<DCCLFieldCodecBase> new_field_codec(new Codec());
        new_field_codec->set_name(name);
        new_field_codec->set_field_type(field_type);
        new_field_codec->set_wire_type(wire_type);
        
        DCCLCommon::logger() << warn << "Trying to add: " << *new_field_codec
                             << ", but already have duplicate codec (For `name`/`field type` pair) "
                             << *(codecs_[field_type].find(name)->second)
                             << std::endl;
    }
}







#endif
