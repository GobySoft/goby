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


#ifndef DCCLFIELDCODECARITHMETIC20120726H
#define DCCLFIELDCODECARITHMETIC20120726H

#include <limits>

#include "goby/acomms/dccl/dccl_field_codec_typed.h"
#include "goby/acomms/protobuf/dccl.pb.h"

namespace goby
{
    namespace acomms
    {
        template<typename FieldType = double>   
            class DCCLArithmeticFieldCodecBase : public DCCLRepeatedTypedFieldCodec<double, FieldType>
        {
          public:
          typedef double WireType;
          typedef uint32 freq_type;
          typedef int symbol_type; // google protobuf RepeatedField size type
            
            
          Bitset encode_repeated(const std::vector<WireType>& wire_value)
          {
              return Bitset();
          }
            
          std::vector<WireType> decode_repeated(Bitset* bits)
          {
              return std::vector<WireType>();
          }


          unsigned size_repeated(const std::vector<WireType>& field_values)
          {
              return 0;
          }
            

          unsigned max_size_repeated()
          {
              return 0;
          }
            

          unsigned min_size_repeated()
          {
              return 0;
          }
          
          void validate()
          {
              DCCLFieldCodecBase::require(DCCLFieldCodecBase::dccl_field_options().has_arithmetic(),
                                          "missing (goby.field).dccl.arithmetic");

              std::string model_name = DCCLFieldCodecBase::dccl_field_options().arithmetic().model();
              DCCLFieldCodecBase::require(models_.count(model_name),
                                          "no such (goby.field).dccl.arithmetic.model called \"" + model_name + "\" loaded.");
              
          }


          struct Model
          {
              protobuf::ArithmeticModel user_model;
              std::map<WireType, symbol_type> value_to_symbol;
              std::map<symbol_type, freq_type> symbol_to_cumulative_freq;
          };
            
          
          static void set_model(const std::string& name, const protobuf::ArithmeticModel& model)
          {
              Model new_model;
              new_model.user_model = model;
              create_and_validate_model(&new_model);
              models_[name] = new_model;
          }

          static void create_and_validate_model(Model* model)
          {
              if(!model->user_model.IsInitialized())
              {
                  throw(DCCLException("Invalid model: " +
                                      model->user_model.DebugString() +
                                      "Missing fields: " + model->user_model.InitializationErrorString()));
              }

              freq_type cumulative_freq = 0;
              for(symbol_type symbol = 0, n = model->user_model.frequency_size(); symbol < n; ++symbol)
              {
                  model->value_to_symbol[symbol] = cumulative_freq;
                  
              }
              
          }
          
            
          private:
          static const int CODE_VALUE_BITS;
          static const int FREQUENCY_BITS;
            
          static const freq_type MAX_FREQUENCY;
            
            
          static std::map<std::string, Model> models_;
        };
        
        template<typename WireType>
            const int DCCLArithmeticFieldCodecBase<WireType>::CODE_VALUE_BITS = 32;
        template<typename WireType>
            const int DCCLArithmeticFieldCodecBase<WireType>::FREQUENCY_BITS = 30;
        template<typename WireType>
            const typename DCCLArithmeticFieldCodecBase<WireType>::freq_type DCCLArithmeticFieldCodecBase<WireType>::MAX_FREQUENCY = (1 << DCCLArithmeticFieldCodecBase<WireType>::FREQUENCY_BITS) - 1;
        
        template<typename WireType>
            std::map<std::string, typename DCCLArithmeticFieldCodecBase<WireType>::Model > DCCLArithmeticFieldCodecBase<WireType>::models_;
        
        template<typename FieldType>   
            class DCCLArithmeticFieldCodec : public DCCLArithmeticFieldCodecBase<FieldType>
        {
            double pre_encode(const FieldType& field_value)
            { return static_cast<double>(field_value); }
            
            FieldType post_decode(const double& wire_value)
            { return static_cast<FieldType>(wire_value); }
        };

        
        template <>
            class DCCLArithmeticFieldCodec<const google::protobuf::EnumValueDescriptor*> : public DCCLArithmeticFieldCodecBase<const google::protobuf::EnumValueDescriptor*>
        {
          public:
            double pre_encode(const google::protobuf::EnumValueDescriptor* const& field_value)
            { return field_value->index(); }
            
            const google::protobuf::EnumValueDescriptor* post_decode(const double& wire_value)
            {
                const google::protobuf::EnumDescriptor* e = DCCLFieldCodecBase::this_field()->enum_type();
                const google::protobuf::EnumValueDescriptor* return_value = e->value(wire_value);
                
                if(return_value)
                    return return_value;
                else
                    throw(DCCLNullValueException());
            }

        };
        
        
        
    }
}


#endif
