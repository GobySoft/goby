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

// implements DCCLFieldCodecBase for all the basic DCCL types

#ifndef DCCLFIELDCODECDEFAULT20110322H
#define DCCLFIELDCODECDEFAULT20110322H

#include <boost/utility.hpp>
#include <boost/type_traits.hpp>
#include <boost/static_assert.hpp>
#include <boost/bimap.hpp>

#include <google/protobuf/descriptor.h>

#include "goby/protobuf/dccl_option_extensions.pb.h"
#include "goby/util/string.h"
#include "goby/util/sci.h"
#include "goby/util/time.h"
#include "goby/acomms/acomms_constants.h"

#include "dccl_field_codec_default_message.h"
#include "dccl_field_codec.h"

namespace goby
{
    namespace acomms
    {
        template<typename T>
            class DCCLDefaultArithmeticFieldCodec : public DCCLFixedFieldCodec
        {
          protected:

            virtual double max()
            { return get(dccl::max); }

            virtual double min()
            { return get(dccl::min); }

            virtual double precision()
            { return has(dccl::precision) ? get(dccl::precision) : 0; }
            
            virtual void _validate()
            {
                require(dccl::min, "dccl.min");
                require(dccl::max, "dccl.max");
            }

            Bitset _encode(const boost::any& wire_value)
            {
                DCCLCommon::logger() << "starting encode of field with max " << max() << ", min " << min() << ", prec " << precision() << std::endl;
                
                if(wire_value.empty())
                    return Bitset(_size());
                
                try
                {
                    T t = boost::any_cast<T>(wire_value);
                    if(t < min() || t > max())
                        return Bitset(_size());

                    t = goby::util::unbiased_round(t, precision());
                    
                    DCCLCommon::logger() << debug1 << "using value " << t << std::endl;
        
                    
                    t -= min();
                    t *= std::pow(10.0, precision());
                    return Bitset(_size(), goby::util::as<unsigned long>(t)+1);
                }
                catch(boost::bad_any_cast& e)
                {
                    throw(DCCLException("Bad type given to encode"));
                }                
            }

            boost::any _decode(Bitset* bits)
            {
                unsigned long t = bits->to_ulong();
                if(!t) return boost::any();

                --t;
                return goby::util::as<T>(
                    goby::util::unbiased_round(
                        t / (std::pow(10.0, precision())) + min(), precision()));
            }

            unsigned _size()
            {
                // leave one value for unspecified (always encoded as 0)
                const unsigned NULL_VALUE = 1;
                return std::ceil(std::log((max()-min())*std::pow(10.0, precision())+1 + NULL_VALUE)/std::log(2));
            }
            
        };

        class DCCLDefaultBoolCodec : public DCCLFixedFieldCodec
        {
          private:
            Bitset _encode(const boost::any& wire_value);
            boost::any _decode(Bitset* bits);     
            unsigned _size();
            void _validate();
        };
        
        class DCCLDefaultStringCodec : public DCCLFieldCodecBase
        {
          private:
            Bitset _encode(const boost::any& wire_value);
            boost::any _decode(Bitset* bits);     
            unsigned _size(const boost::any& field_value);
            unsigned _max_size();
            unsigned _min_size();
            void _validate();
            bool _variable_size() { return true; }
          private:
            enum { MAX_STRING_LENGTH = 255 };
            
        };

        class DCCLDefaultBytesCodec : public DCCLFieldCodecBase
        {
          private:
            Bitset _encode(const boost::any& wire_value);
            boost::any _decode(Bitset* bits);     
            unsigned _size(const boost::any& field_value);
            unsigned _max_size();
            unsigned _min_size();
            bool _variable_size() { return true; }
            void _validate();
        };

        
        class DCCLDefaultEnumCodec : public DCCLDefaultArithmeticFieldCodec<int32>
        {
          public:
            boost::any _pre_encode(const boost::any& field_value);
            boost::any _post_decode(const boost::any& wire_value);

          private:
            void _validate() { }
            
            double max()
            {
                const google::protobuf::EnumDescriptor* e = this_field()->enum_type();
                return e->value_count()-1;
            }
            double min()
            { return 0; }
        };
        
        
        class DCCLTimeCodec : public DCCLDefaultArithmeticFieldCodec<int32>
        {
          public:
            boost::any _pre_encode(const boost::any& field_value);
            boost::any _post_decode(const boost::any& wire_value);

          private:
            void _validate() { }

            double max() { return HOURS_IN_DAY*SECONDS_IN_HOUR; }
            double min() { return 0; }
            enum { HOURS_IN_DAY = 24 };
            enum { SECONDS_IN_HOUR = 3600 };

        };

        
        
        template<typename T>
            class DCCLStaticCodec : public DCCLFixedFieldCodec
        {
            Bitset _encode(const boost::any& wire_value)
            {
                return Bitset(_size());
            }
            
            boost::any _decode(Bitset* bits)
            { return goby::util::as<T>(get(dccl::static_value)); }
            
            unsigned _size()
            {
                return 0;
            }
            
            void _validate()
            {
                require(dccl::static_value, "dccl.static_value");                
            }
            
        };
        class DCCLModemIdConverterCodec : public DCCLDefaultArithmeticFieldCodec<int32>
        {
          public:
            static void add(std::string platform, int32 id)
            {
                platform2modem_id_.left.insert(std::make_pair(platform, id));
            }
            
            boost::any _pre_encode(const boost::any& field_value);
            boost::any _post_decode(const boost::any& wire_value);
            

          private:  
            void _validate() { }
            double max() { return 30; }
            double min() { return 0; }

          private:
            static boost::bimap<std::string, int32> platform2modem_id_;
        };

        
    }
}

#endif
