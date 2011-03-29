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

// implements DCCLFieldCodec for all the basic DCCL types

#ifndef DCCLFIELDCODECDEFAULT20110322H
#define DCCLFIELDCODECDEFAULT20110322H

#include "dccl_field_codec.h"
#include "goby/protobuf/dccl_option_extensions.pb.h"
#include "goby/util/string.h"
#include "goby/util/sci.h"
#include "goby/util/time.h"
#include "goby/acomms/acomms_constants.h"
#include <boost/utility.hpp>
#include <boost/type_traits.hpp>
#include "dccl.h"

namespace goby
{
    namespace acomms
    {
        template<typename T>
            class DCCLDefaultArithmeticFieldCodec : public DCCLFieldCodec
        {
          protected:
            
            virtual Bitset _encode(const boost::any& field_value)
            {
                if(field_value.empty())
                    return Bitset();
                
                try
                {
                    T t = boost::any_cast<T>(field_value);
                    if(t < get(dccl::min) || t > get(dccl::max))
                        return Bitset();
                    
                    t -= get(dccl::min);
                    t *= pow(10.0, get(dccl::precision));
                    return Bitset(_size(), static_cast<unsigned long>(t)+1);
                }
                catch(boost::bad_any_cast& e)
                {
                    throw(DCCLException("Bad type given to encode"));
                }
            }
            
            virtual boost::any _decode(Bitset* bits)
            {
                unsigned long t = bits->to_ulong();
                if(!t) return boost::any();

                --t;
                return static_cast<T>(t / (std::pow(10.0, get(dccl::precision)))
                                      + get(dccl::min));
            }
            
            virtual unsigned _size()
            {
                return std::ceil(std::log((get(dccl::max)-get(dccl::min))*
                                          std::pow(10.0, get(dccl::precision))+2)/std::log(2));
            }

            virtual void _validate()
            {
                require(dccl::min, "dccl.min");
                require(dccl::max, "dccl.max");
            }
            
        };

        class DCCLDefaultMessageCodec : public DCCLFieldCodec
        {
          protected:
            
            virtual Bitset _encode(const boost::any& field_value);
            virtual boost::any _decode(Bitset* bits);     
            virtual unsigned _size();
            virtual void _validate();
        };

        
        template<typename T>
            class DCCLDefaultFieldCodec
        { };
        
        template<>
            class DCCLDefaultFieldCodec<double>
            : public DCCLDefaultArithmeticFieldCodec<double>
        { };

        template<>
            class DCCLDefaultFieldCodec<google::protobuf::int32>
            : public DCCLDefaultArithmeticFieldCodec<google::protobuf::int32>
        { };

        template<> 
            class DCCLDefaultFieldCodec<google::protobuf::Message>
            : public DCCLDefaultMessageCodec
        { };
        
        
        class DCCLTimeCodec : public DCCLFieldCodec
        {
            Bitset _encode(const boost::any& field_value);

            boost::any _decode(Bitset* bits);
            
            unsigned _size()
            {
                return HEAD_TIME_SIZE;
            }            
        };

        template<typename T>
            class DCCLStaticCodec : public DCCLFieldCodec
        {
            Bitset _encode(const boost::any& field_value)
            {
                return Bitset(_size());
            }
            
            boost::any _decode(Bitset* bits)
            { return goby::util::as<T>(get(dccl::static_value)); }
            
            unsigned _size()
            { return 0; }
            
            void _validate()
            {
                require(dccl::static_value, "dccl.static_value");                
            }
        };


        
        // TODO(tes): THIS IS A PLACEHOLDER
        class DCCLModemIdConverterCodec : public DCCLDefaultFieldCodec<google::protobuf::int32>
        {
            Bitset _encode(const boost::any& field_value)
            {
                int v = 1;
                
                try
                {
                    return DCCLDefaultFieldCodec<google::protobuf::int32>::_encode(v);
                }
                catch(boost::bad_any_cast& e)
                {
                    throw(DCCLException("Bad type given to encode"));
                }
            }
            
            boost::any _decode(Bitset* bits)
            {
                return "unicorn";
            }            
        };

        


        
    }
}


#endif
