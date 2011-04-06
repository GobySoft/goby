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
        class FieldCodecManager
        {
          public:
            template<typename T, template <typename T> class Codec>
                static void add(const std::string& name);

            template<typename T, class Codec>
                typename boost::enable_if<
                boost::mpl::and_<boost::is_base_of<google::protobuf::Message, T>,
                boost::mpl::not_<boost::is_same<google::protobuf::Message, T> >
                >,
                void>::type 
                static add(const std::string& name);
                
            template<typename T, class Codec>
                typename boost::disable_if<
                boost::mpl::and_<boost::is_base_of<google::protobuf::Message, T>,
                boost::mpl::not_<boost::is_same<google::protobuf::Message, T> >
                >,
                void>::type
                static add(const std::string& name);
                
            template<google::protobuf::FieldDescriptor::Type type, class Codec>
                static void add(const std::string& name);
                
            static boost::shared_ptr<DCCLFieldCodec> find(
                const google::protobuf::FieldDescriptor* field,
                const std::string& name)
            {
                if(field->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE)
                    return find(field->message_type(), name);
                else
                    return __find(field->type(), name);
            }
                

            static boost::shared_ptr<DCCLFieldCodec> find(
                const google::protobuf::Descriptor* desc,
                const std::string& name)
            {
                return __find(google::protobuf::FieldDescriptor::TYPE_MESSAGE,
                              name, desc->full_name());
            }
                
            
          private:
            FieldCodecManager() { }
            ~FieldCodecManager() { }
            FieldCodecManager(const FieldCodecManager&);
            FieldCodecManager& operator= (const FieldCodecManager&);

            
            static boost::shared_ptr<DCCLFieldCodec> __find(
                google::protobuf::FieldDescriptor::Type type,
                const std::string& codec_name,
                const std::string& type_name = "");
                
            static std::string __mangle_name(const std::string& codec_name,
                                      const std::string& type_name) 
            { return type_name.empty() ? codec_name : codec_name + "@@" + type_name; }
                
            template<class Codec>
                static void __add(const std::string& name, google::protobuf::FieldDescriptor::Type type);
          private:
            
            typedef std::map<std::string, boost::shared_ptr<DCCLFieldCodec> > InsideMap;
            static std::map<google::protobuf::FieldDescriptor::Type, InsideMap> codecs_;
        };
    }
}

template<typename T, template <typename T> class Codec>
    void goby::acomms::FieldCodecManager::add(const std::string& name)
{
    add<T, Codec<T> >(name);
}

template<typename T, class Codec>
    typename boost::enable_if<
    boost::mpl::and_<
    boost::is_base_of<google::protobuf::Message, T>,
    boost::mpl::not_<boost::is_same<google::protobuf::Message, T> >
    >,
    void>::type 
    goby::acomms::FieldCodecManager::add(const std::string& name)
{
    __add<Codec>(__mangle_name(name, T::descriptor()->full_name()),
                 google::protobuf::FieldDescriptor::TYPE_MESSAGE);
    TypeHelper::add<T>();
}

template<typename T, class Codec>
    typename boost::disable_if<
    boost::mpl::and_<
    boost::is_base_of<google::protobuf::Message, T>,
    boost::mpl::not_<boost::is_same<google::protobuf::Message, T> >
    >,
    void>::type
    goby::acomms::FieldCodecManager::add(const std::string& name)
{
    using google::protobuf::FieldDescriptor;
    const FieldDescriptor::CppType cpp_type = ToProtoCppType<T>::as_enum();

    for(int i = 1, n = FieldDescriptor::MAX_TYPE; i <= n; ++i)
    {
        FieldDescriptor::Type type = static_cast<FieldDescriptor::Type>(i);
        if(FieldDescriptor::TypeToCppType(type) == cpp_type)
        {
            __add<Codec>(name, type);
        }
    }    
}

template<google::protobuf::FieldDescriptor::Type type, class Codec>
    void goby::acomms::FieldCodecManager::add(const std::string& name)
{
    __add<Codec>(name, type);
}


template<class Codec>
void goby::acomms::FieldCodecManager::__add(const std::string& name, google::protobuf::FieldDescriptor::Type type)
{
    using google::protobuf::FieldDescriptor;    
    //const FieldDescriptor::CppType cpp_type = FieldDescriptor::TypeToCppType(type);
    if(!codecs_[type].count(name))
    {
        codecs_[type][name] = boost::shared_ptr<DCCLFieldCodec>(new Codec());
        /* if(log_) */
        /*     *log_ << "Adding codec " << name */
        /*           << " for Type " */
        /*           << type_helper_.find(type)->as_str() */
        /*           << " (" << type_helper_.find(cpp_type)->as_str() << ")"  */
        /*           << std::endl; */
    }            
    else
    {
        /* if(log_) */
        /*     *log_ << warn << "Ignoring duplicate codec " << name */
        /*           << " for Type " */
        /*           << type_helper_.find(type)->as_str() */
        /*           << " (" << type_helper_.find(cpp_type)->as_str() << ")" */
        /*           << std::endl; */
    }
}







#endif
