// copyright 2010 t. schneider tes@mit.edu
// 
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
#ifndef CONFIGURATIONREADER20100929H
#define CONFIGURATIONREADER20100929H

#include <sstream>
#include <iostream>
#include <fstream>

#include <boost/foreach.hpp>
#include <boost/program_options.hpp>
#include <boost/cstdint.hpp>
#include <boost/filesystem.hpp>

#include <google/protobuf/message.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/io/zero_copy_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/text_format.h>


namespace goby
{
    namespace core
    {
        class ConfigReader
        {
          public:
            static bool read_cfg(int argc, char* argv[], google::protobuf::Message* message, std::string* application_name, boost::program_options::variables_map* var_map);
            
          private:
            static void get_protobuf_program_options(boost::program_options::options_description& po_desc, const google::protobuf::Message& message, const std::string& prefix = "");
            static void set_protobuf_program_option(const boost::program_options::variables_map& vm, google::protobuf::Message& message, const std::string& full_name, const boost::program_options::variable_value& value);
        };
    }
}

#endif
