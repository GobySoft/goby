// copyright 2011 t. schneider tes@mit.edu
// 
// this file is part of goby-acomms, a collection of libraries for acoustic underwater networking
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

#ifndef AcommsHelpers20110208H
#define AcommsHelpers20110208H

#include <string>
#include <limits>
#include <bitset>

#include <cryptopp/filters.h>
#include <cryptopp/hex.h>

#include "goby/protobuf/modem_message.pb.h"
#include "goby/protobuf/xml_config.pb.h"
#include "goby/core/core_constants.h"

namespace goby
{

    namespace acomms
    {            
        /* inline void hex_decode(const std::string& in, std::string* out) */
        /* { */
        /*     int in_size = in.size(); */
        /*     int out_size = in_size >> 1; */
        /*     if(in_size & 1) */
        /*         ++out_size; */
            
        /*     out->resize(out_size); */
        /*     std::stringstream ss; */
        /*     for(int i = 0, n = in.size(); */
        /*         i < n; */
        /*         i += 2) */
        /*     { */
        /*         ss << std::hex << in.substr(i, 2); */
        /*         int test; */
        /*         ss >> test; */
        /*         std::cout << test << std::endl; */
        /*     } */
        /* } */

        /* inline std::string hex_decode(const std::string& in) */
        /* { */
        /*     std::string out; */
        /*     hex_decode(in, &out); */
        /*     return out; */
        /* } */
    
        /* inline void hex_encode(const std::string& in, std::string* out) */
        /* { */
        /*     const bool uppercase = false; */
        /*     CryptoPP::HexEncoder hex(new CryptoPP::StringSink(*out), uppercase); */
        /*     hex.Put((byte*)in.c_str(), in.size()); */
        /*     hex.MessageEnd(); */
        /* } */

        /* inline std::string hex_encode(const std::string& in) */
        /* { */
        /*     std::string out; */
        /*     hex_encode(in, &out); */
        /*     return out; */
        /* } */
        
        // provides stream output operator for Google Protocol Buffers Message 
        inline std::ostream& operator<<(std::ostream& out, const google::protobuf::Message& msg)
        {
            return (out << "[[" << msg.GetDescriptor()->name() <<"]] " << msg.ShortDebugString());
        }


        class ManipulatorManager
        {
          public:
            void add(unsigned id, goby::acomms::protobuf::MessageFile::Manipulator manip)
            {
                manips_.insert(std::make_pair(id, manip));
            }            
            
            bool has(unsigned id, goby::acomms::protobuf::MessageFile::Manipulator manip) const
            {
                typedef std::multimap<unsigned, goby::acomms::protobuf::MessageFile::Manipulator>::const_iterator iterator;
                std::pair<iterator,iterator> p = manips_.equal_range(id);

                for(iterator it = p.first; it != p.second; ++it)
                {
                    if(it->second == manip)
                        return true;
                }

                return false;
            }

            void clear()
            {
                manips_.clear();
            }
            
          private:
            // manipulator multimap (no_encode, no_decode, etc)
            // maps DCCL ID (unsigned) onto Manipulator enumeration (xml_config.proto)
            std::multimap<unsigned, goby::acomms::protobuf::MessageFile::Manipulator> manips_;

        };
        
        

        
    }
}

#endif
