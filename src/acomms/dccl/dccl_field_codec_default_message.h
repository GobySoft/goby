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


#ifndef DCCLFIELDCODECDEFAULTMESSAGE20110510H
#define DCCLFIELDCODECDEFAULTMESSAGE20110510H

#include "dccl_field_codec.h"
#include "dccl_field_codec_manager.h"
#include "goby/common/protobuf/acomms_option_extensions.pb.h"


namespace goby
{
    namespace acomms
    {
        /// \brief Provides the default codec for encoding a base Google Protobuf message or an embedded message by calling the appropriate field codecs for every field.
        class DCCLDefaultMessageCodec : public DCCLFieldCodecBase
        {
          private:
            
            void any_encode(Bitset* bits, const boost::any& wire_value);
            void any_decode(Bitset* bits, boost::any* wire_value); 
            unsigned max_size();
            unsigned min_size();
            unsigned any_size(const boost::any& wire_value);
            void any_run_hooks(const boost::any& field_value);

            
            void validate();
            std::string info();
            bool check_field(const google::protobuf::FieldDescriptor* field);

            struct Size
            {
                static void repeated(boost::shared_ptr<DCCLFieldCodecBase> codec,
                                     unsigned* return_value,
                                     const std::vector<boost::any>& field_values,
                                     const google::protobuf::FieldDescriptor* field_desc)
                    {
                        codec->field_size_repeated(return_value, field_values, field_desc);
                    }
                
                static void single(boost::shared_ptr<DCCLFieldCodecBase> codec,
                                   unsigned* return_value,
                                   const boost::any& field_value,
                                   const google::protobuf::FieldDescriptor* field_desc)
                    {
                        codec->field_size(return_value, field_value, field_desc);
                    }
                
            };
            
            struct Encoder
            {
                static void repeated(boost::shared_ptr<DCCLFieldCodecBase> codec,
                                     Bitset* return_value,
                                     const std::vector<boost::any>& field_values,
                                     const google::protobuf::FieldDescriptor* field_desc)
                    {
                        codec->field_encode_repeated(return_value, field_values, field_desc);
                    }
                
                static void single(boost::shared_ptr<DCCLFieldCodecBase> codec,
                                   Bitset* return_value,
                                   const boost::any& field_value,
                                   const google::protobuf::FieldDescriptor* field_desc)
                    {
                        codec->field_encode(return_value, field_value, field_desc);
                    }
            };

            
            struct RunHooks
            {
                static void repeated(boost::shared_ptr<DCCLFieldCodecBase> codec,
                                     bool* return_value,
                                     const std::vector<boost::any>& field_values,
                                     const google::protobuf::FieldDescriptor* field_desc);
                
                static void single(boost::shared_ptr<DCCLFieldCodecBase> codec,
                                   bool* return_value,
                                   const boost::any& field_value,
                                   const google::protobuf::FieldDescriptor* field_desc);
            };


            struct MaxSize
            {
                static void field(boost::shared_ptr<DCCLFieldCodecBase> codec,
                                  unsigned* return_value,
                                  const google::protobuf::FieldDescriptor* field_desc)
                    {
                        codec->field_max_size(return_value, field_desc);
                    }
            };

            struct MinSize
            {
                static void field(boost::shared_ptr<DCCLFieldCodecBase> codec,
                                  unsigned* return_value,
                                  const google::protobuf::FieldDescriptor* field_desc)
                    {
                        codec->field_min_size(return_value, field_desc);
                    }
            };
            
            
            struct Validate
            {
                static void field(boost::shared_ptr<DCCLFieldCodecBase> codec,
                                  bool* return_value,
                                  const google::protobuf::FieldDescriptor* field_desc)
                    {
                        codec->field_validate(return_value, field_desc);
                    }
            };

            struct Info
            {
                static void field(boost::shared_ptr<DCCLFieldCodecBase> codec,
                                  std::stringstream* return_value,
                                  const google::protobuf::FieldDescriptor* field_desc)
                    {
                        codec->field_info(return_value, field_desc);
                    }
            };
            
            
            template<typename Action, typename ReturnType>
                void traverse_descriptor(ReturnType* return_value)
            {
                const google::protobuf::Descriptor* desc =
                    DCCLFieldCodecBase::this_descriptor();
                for(int i = 0, n = desc->field_count(); i < n; ++i)
                {
                    const google::protobuf::FieldDescriptor* field_desc = desc->field(i);
                    
                    if(!check_field(field_desc))
                        continue;

                    Action::field(DCCLFieldCodecManager::find(field_desc),
                                  return_value, field_desc);                    
                }
            }
            

            template<typename Action, typename ReturnType>
                ReturnType traverse_const_message(const boost::any& wire_value)
            {
                try
                {
                    ReturnType return_value = ReturnType();
       
                    const google::protobuf::Message* msg = boost::any_cast<const google::protobuf::Message*>(wire_value);
                    const google::protobuf::Descriptor* desc = msg->GetDescriptor();
                    const google::protobuf::Reflection* refl = msg->GetReflection();
                    for(int i = 0, n = desc->field_count(); i < n; ++i)
                    {       
                        const google::protobuf::FieldDescriptor* field_desc = desc->field(i);

                        if(!check_field(field_desc))
                            continue;
           
                        boost::shared_ptr<DCCLFieldCodecBase> codec =
                            DCCLFieldCodecManager::find(field_desc);
                        boost::shared_ptr<FromProtoCppTypeBase> helper =
                            DCCLTypeHelper::find(field_desc);
            
            
                        if(field_desc->is_repeated())
                        {
                            std::vector<boost::any> field_values;
                            for(int j = 0, m = refl->FieldSize(*msg, field_desc); j < m; ++j)
                                field_values.push_back(helper->get_repeated_value(field_desc, *msg, j));
                   
                            Action::repeated(codec, &return_value, field_values, field_desc);
                        }
                        else
                        {
                            Action::single(codec, &return_value, helper->get_value(field_desc, *msg), field_desc);
                        }
                    }
                    return return_value;
                }
                catch(boost::bad_any_cast& e)
                {
                    throw(DCCLException("Bad type given to traverse const, expecting const google::protobuf::Message*, got " + std::string(wire_value.type().name())));
                }
                
            }

            template<typename Action, typename ReturnType>
                ReturnType traverse_mutable_message(const boost::any& wire_value)
            {
                try
                {
                    ReturnType return_value = ReturnType();
       
                    const google::protobuf::Message* msg = boost::any_cast<const google::protobuf::Message*>(wire_value);
                    const google::protobuf::Descriptor* desc = msg->GetDescriptor();
                    const google::protobuf::Reflection* refl = msg->GetReflection();
                    for(int i = 0, n = desc->field_count(); i < n; ++i)
                    {       
                        const google::protobuf::FieldDescriptor* field_desc = desc->field(i);

                        if(!check_field(field_desc))
                            continue;
           
                        boost::shared_ptr<DCCLFieldCodecBase> codec =
                            DCCLFieldCodecManager::find(field_desc);
                        boost::shared_ptr<FromProtoCppTypeBase> helper =
                            DCCLTypeHelper::find(field_desc);
            
            
                        if(field_desc->is_repeated())
                        {
                            std::vector<boost::any> field_values;
                            for(int j = 0, m = refl->FieldSize(*msg, field_desc); j < m; ++j)
                                field_values.push_back(helper->get_repeated_value(field_desc, *msg, j));
                            
                            Action::repeated(codec, &return_value, field_values, field_desc);
                        }
                        else
                        {
                            Action::single(codec, &return_value, helper->get_value(field_desc, *msg), field_desc);
                        }
                    }
                    return return_value;
                }
                catch(boost::bad_any_cast& e)
                {
                    throw(DCCLException("Bad type given to traverse, expecting const google::protobuf::Message*"));
                }
                
            }

            

        };


    }
}


//encode, size, etc.




#endif
