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

#ifndef MOOSPROTOBUFHELPERS20110216H
#define MOOSPROTOBUFHELPERS20110216H

#include <google/protobuf/io/tokenizer.h>
#include "goby/util/liblogger/flex_ostream.h"

/// \brief Converts the Google Protocol Buffers message `msg` into a suitable (human readable) string `out` for sending via MOOS
///
/// \param out pointer to std::string to store serialized result
/// \param msg Google Protocol buffers message to serialize
inline void serialize_for_moos(std::string* out, const google::protobuf::Message& msg)
{
    google::protobuf::TextFormat::Printer printer;
    printer.SetSingleLineMode(true);
    printer.PrintToString(msg, out);
}


/// \brief Parses the string `in` to Google Protocol Buffers message `msg`. All errors are written to the goby::util::glogger().
///
/// \param in std::string to parse
/// \param msg Google Protocol buffers message to store result
inline void parse_for_moos(const std::string& in, google::protobuf::Message* msg)
{
    google::protobuf::TextFormat::Parser parser;
    FlexOStreamErrorCollector error_collector(in);
    parser.RecordErrorsTo(&error_collector);
    parser.ParseFromString(in, msg);
}


#endif
