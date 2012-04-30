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
            /* template<typename FieldType, template <typename FieldType> class Codec> */
            /*     static void add(const std::string& name); */
            
            /// \brief Add a new field codec (used for codecs operating on statically generated Protobuf messages, that is, children of google::protobuf::Message but not google::protobuf::Message itself).
            ///
            /// \tparam Codec A child of DCCLFieldCodecBase
            /// \param name Name to use for this codec. Corresponds to (goby.field).dccl.codec="name" in .proto file.
            /// \return nothing (void). Return templates are used for template metaprogramming selection of the proper add() overload.
            template<class Codec>
                typename boost::enable_if<
                boost::mpl::and_<boost::is_base_of<google::protobuf::Message, typename Codec::wire_type>,
                boost::mpl::not_<boost::is_same<google::protobuf::Message, typename Codec::wire_type> >
                >,
                void>::type 
                static add(const std::string& name);
                
            /// \brief Add a new field codec (used for codecs operating on all types except statically generated Protobuf messages).
            ///
            /// \tparam Codec A child of DCCLFieldCodecBase
            /// \param name Name to use for this codec. Corresponds to (goby.field).dccl.codec="name" in .proto file.
            /// \return nothing (void). Return templates are used for template metaprogramming selection of the proper add() overload.
            template<class Codec>
                typename boost::disable_if<
                boost::mpl::and_<boost::is_base_of<google::protobuf::Message, typename Codec::wire_type>,
                boost::mpl::not_<boost::is_same<google::protobuf::Message,typename Codec::wire_type> >
                >,
                void>::type
                static add(const std::string& name);
                
            /// \brief Add a new field codec only valid for a specific google::protobuf::FieldDescriptor::Type. This is useful if a given codec is designed to work with only a specific Protobuf type that shares an underlying C++ type (e.g. Protobuf types `bytes` and `string`)
            ///
            /// \tparam Codec A child of DCCLFieldCodecBase
            /// \tparam type The google::protobuf::FieldDescriptor::Type enumeration that this codec works on.
            /// \param name Name to use for this codec. Corresponds to (goby.field).dccl.codec="name" in .proto file.
            template<class Codec, google::protobuf::FieldDescriptor::Type type>
                static void add(const std::string& name);

            /// \brief Find the codec for a given field. For embedded messages, prefers (goby.field).dccl.codec (inside field) over (goby.msg).dccl.codec (inside embedded message).
            static boost::shared_ptr<DCCLFieldCodecBase> find(
                const google::protobuf::FieldDescriptor* field)
            {
                std::string name = __find_codec(field);
                
                if(field->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE)
                    return find(field->message_type(), name);
                else
                    return __find(field->type(), name);
            }                

            /// \brief Find the codec for a given base (or embedded) message.
            ///
            /// \param desc Message descriptor to find codec for
            /// \param name Codec name (used for embedded messages to prefer the codec listed as a field option). Omit for finding the codec of a base message (one that is not embedded).
            static boost::shared_ptr<DCCLFieldCodecBase> find(
                const google::protobuf::Descriptor* desc,
                std::string name = "")
            {
                if(name.empty())
                    name = desc->options().GetExtension(goby::msg).dccl().codec();

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
                

            template<typename WireType, typename FieldType, class Codec> 
                static void __add(const std::string& name); 

            template<class Codec>
                static void __add(const std::string& name,
                                  google::protobuf::FieldDescriptor::Type field_type,
                                  google::protobuf::FieldDescriptor::CppType wire_type);


            static std::string __find_codec(const google::protobuf::FieldDescriptor* field)
            {
                DCCLFieldOptions dccl_field_options = field->options().GetExtension(goby::field).dccl();
                
                // prefer the codec listed as a field extension
                if(dccl_field_options.has_codec())
                    return dccl_field_options.codec();
                else if(field->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE)
                    return field->message_type()->options().GetExtension(goby::msg).dccl().codec();
                else
                    return dccl_field_options.codec();
            }

          private:
            typedef std::map<std::string, boost::shared_ptr<DCCLFieldCodecBase> > InsideMap;
            static std::map<google::protobuf::FieldDescriptor::Type, InsideMap> codecs_;
        };
    }
}

template<class Codec>
    typename boost::enable_if<
    boost::mpl::and_<
boost::is_base_of<google::protobuf::Message, typename Codec::wire_type>,
    boost::mpl::not_<boost::is_same<google::protobuf::Message, typename Codec::wire_type> >
    >,
    void>::type 
    goby::acomms::DCCLFieldCodecManager::add(const std::string& name)
{
    DCCLTypeHelper::add<typename Codec::wire_type>();
    __add<Codec>(__mangle_name(name, Codec::wire_type::descriptor()->full_name()),
                 google::protobuf::FieldDescriptor::TYPE_MESSAGE,
                 google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE);
}

template<class Codec>
    typename boost::disable_if<
    boost::mpl::and_<
    boost::is_base_of<google::protobuf::Message, typename Codec::wire_type>,
    boost::mpl::not_<boost::is_same<google::protobuf::Message, typename Codec::wire_type> >
    >,
    void>::type
    goby::acomms::DCCLFieldCodecManager::add(const std::string& name)
{
    __add<typename Codec::wire_type, typename Codec::field_type, Codec>(name);
}

template<class Codec, google::protobuf::FieldDescriptor::Type type> 
     void goby::acomms::DCCLFieldCodecManager::add(const std::string& name) 
 { 
     __add<Codec>(name, type, google::protobuf::FieldDescriptor::TypeToCppType(type));
 }


template<typename WireType, typename FieldType, class Codec>
    void goby::acomms::DCCLFieldCodecManager::__add(const std::string& name)
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
        goby::glog.is(common::logger::DEBUG1) && goby::glog << "Adding codec " << *new_field_codec << std::endl;
    }            
    else
    {
        boost::shared_ptr<DCCLFieldCodecBase> new_field_codec(new Codec());
        new_field_codec->set_name(name);
        new_field_codec->set_field_type(field_type);
        new_field_codec->set_wire_type(wire_type);
        
        goby::glog.is(common::logger::DEBUG1) && goby::glog << "Trying to add: " << *new_field_codec
                             << ", but already have duplicate codec (For `name`/`field type` pair) "
                             << *(codecs_[field_type].find(name)->second)
                             << std::endl;
    }
}



#endif
