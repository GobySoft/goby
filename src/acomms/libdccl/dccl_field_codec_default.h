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


namespace goby
{
    namespace acomms
    {
        template<typename T>
            class DCCLDefaultArithmeticFieldCodec : public DCCLFieldCodec
        {
          protected:
            
            virtual Bitset _encode(const boost::any& field_value,
                                   const google::protobuf::FieldDescriptor* field)
            {
                if(field_value.empty())
                    return Bitset();
                
                try
                {
                    const double min = field->options().GetExtension(dccl::min);
                    const double max = field->options().GetExtension(dccl::max);
                    const double precision = field->options().GetExtension(dccl::precision);
                    
                    T t = boost::any_cast<T>(field_value);
                    if(t < min || t > max)
                        return Bitset();
                    
                    t -= min;
                    t *= pow(10.0, precision);
                    return Bitset(_size(field), static_cast<unsigned long>(t)+1);
                }
                catch(boost::bad_any_cast& e)
                {
                    throw(DCCLException("Bad type given to encode"));
                }
            }
            
            virtual boost::any _decode(const Bitset& bits,
                                       const google::protobuf::FieldDescriptor* field)
            {
                const double precision = field->options().GetExtension(dccl::precision);
                const double min = field->options().GetExtension(dccl::min);
                unsigned long t = bits.to_ulong();
                if(!t) return boost::any();

                --t;
                return static_cast<T>(t / (std::pow(10.0, precision)) + min);
            }
            
            virtual unsigned _size(const google::protobuf::FieldDescriptor* field)
            {
                const double min = field->options().GetExtension(dccl::min);
                const double max = field->options().GetExtension(dccl::max);
                const double precision = field->options().GetExtension(dccl::precision);
                return std::ceil(std::log((max-min)*std::pow(10.0, precision)+2)/std::log(2));
            }

            virtual void _validate(const google::protobuf::FieldDescriptor* field)
            {
                if(!field->options().HasExtension(dccl::min))
                    throw(DCCLException("Field " + field->name() + " missing option extension `dccl.min`"));
                else if(!field->options().HasExtension(dccl::max))
                    throw(DCCLException("Field " + field->name() + " missing option extension `dccl.max`"));
                else if(field->options().GetExtension(dccl::min) > field->options().GetExtension(dccl::max))
                    throw(DCCLException("Field " + field->name() + "`dccl.min` > `dccl.max`"));
            }
            
        };        

        class DCCLDefaultMessageCodec : public DCCLFieldCodec
        {
          protected:
            
            virtual Bitset _encode(const boost::any& field_value,
                                   const google::protobuf::FieldDescriptor*)
            {
                const google::protobuf::Message* msg =
                    boost::any_cast<const google::protobuf::Message*>(field_value);
                
                const Descriptor* desc = msg->GetDescriptor();
                const Reflection* refl = msg->GetReflection();       
                for(int i = 0, n = desc->field_count(); i < n; ++i)
                {
                    const FieldDescriptor* field_desc = desc->field(i);
                    std::string field_codec = field_desc->options().GetExtension(dccl::codec);
        
                    if(field_desc->options().GetExtension(dccl::omit)
                       || (is_header() && !field_desc->options().GetExtension(dccl::in_head))
                       || (!is_header() && field_desc->options().GetExtension(dccl::in_head)))
                    {
                        continue;
                    }
                    else
                    {
                        if(field_desc->is_repeated())
                        {
                            std::vector<boost::any> field_values;
                            for(int j = 0, m = refl->FieldSize(msg, field_desc); j < m; ++j)
                                field_values.push_back(cpptype_helper_[field_desc->cpp_type()]->
                                                       get_repeated_value(field_desc, msg, j));
                            
                            field_codecs()->[field_desc->cpp_type()][field_codec]->
                                encode(bits, field_values, field_desc);
                        }
                        else
                        {
                            field_codecs()->[field_desc->cpp_type()][field_codec]->
                                encode(bits,cpptype_helper_[field_desc->cpp_type()]->
                                       get_value(field_desc, msg),
                                       field_desc);
                        }
                    }
                }
            }
            
            virtual boost::any _decode(const Bitset& bits,
                                       const google::protobuf::FieldDescriptor* field)
            {
            }
            
            virtual unsigned _size(const google::protobuf::FieldDescriptor* field)
            {
            }

            virtual void _validate(const google::protobuf::FieldDescriptor* field)
            {
            }  
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
            Bitset _encode(const boost::any& field_value,
                           const google::protobuf::FieldDescriptor* field)
            {
                if(field_value.empty())
                    return Bitset(_size(field));
                
                try
                {
                    // trim to time of day
                    boost::posix_time::time_duration time_of_day =
                        util::as<boost::posix_time::ptime>(
                            boost::any_cast<std::string>(field_value)
                            ).time_of_day();
                
                    return Bitset(_size(field),
                                  static_cast<unsigned long>(
                                      util::unbiased_round(
                                          util::time_duration2double(time_of_day), 0)));
                }
                catch(boost::bad_any_cast& e)
                {
                    throw(DCCLException("Bad type given to encode"));
                }
            }
            
            boost::any _decode(const Bitset& bits,
                               const google::protobuf::FieldDescriptor* field)
            {
                // add the date back in (assumes message sent the same day received)
                unsigned long v = bits.to_ulong();

                using namespace boost::posix_time;
                using namespace boost::gregorian;
                
                ptime now = util::goby_time();
                date day_sent;
                
                // this logic will break if there is a separation between message sending and
                // message receipt of greater than 1/2 day (twelve hours)
            
                // if message is from part of the day removed from us by 12 hours, we assume it
                // was sent yesterday
                if(abs(now.time_of_day().total_seconds() - v) > hours(12).total_seconds())
                    day_sent = now.date() - days(1);
                else // otherwise figure it was sent today
                    day_sent = now.date();
                
                return util::as<std::string>(ptime(day_sent,seconds(v)));
            }
            
            unsigned _size(const google::protobuf::FieldDescriptor* field)
            {
                return HEAD_TIME_SIZE;
            }            
        };



        template<typename T>
            class DCCLStaticCodec : public DCCLFieldCodec
        {
            Bitset _encode(const boost::any& field_value,
                           const google::protobuf::FieldDescriptor* field)
            {
                return Bitset(_size(field));
            }
            
            boost::any _decode(const Bitset& bits,
                               const google::protobuf::FieldDescriptor* field)
            {
                return util::as<T>(field->options().GetExtension(dccl::static_value));
            }

            unsigned _size(const google::protobuf::FieldDescriptor* field)
            { return 0; }
            
            void _validate(const google::protobuf::FieldDescriptor* field)
            {
                if(!field->options().HasExtension(dccl::static_value))
                    throw(DCCLException("Field " + field->name() + " missing option extension `dccl.static_value`"));
            }
            
        };


        
        // TODO(tes): THIS IS A PLACEHOLDER
        class DCCLModemIdConverterCodec : public DCCLDefaultFieldCodec<google::protobuf::int32>
        {
            Bitset _encode(const boost::any& field_value,
                           const google::protobuf::FieldDescriptor* field)
            {
                int v = 1;
                
                try
                {
                    return DCCLDefaultFieldCodec<google::protobuf::int32>::_encode(v, field);
                }
                catch(boost::bad_any_cast& e)
                {
                    throw(DCCLException("Bad type given to encode"));
                }
            }
            
            boost::any _decode(const Bitset& bits,
                               const google::protobuf::FieldDescriptor* field)
            {
                return "unicorn";
            }            
        };

        


        
    }
}


#endif
