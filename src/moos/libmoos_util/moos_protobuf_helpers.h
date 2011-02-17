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


inline void serialize_for_moos(std::string* out, const google::protobuf::Message& msg)
{
    google::protobuf::TextFormat::Printer printer;
    printer.SetSingleLineMode(true);
    printer.PrintToString(msg, out);
}


inline void parse_for_moos(const std::string& in, google::protobuf::Message* msg)
{
    google::protobuf::TextFormat::Parser parser;
    FlexOStreamErrorCollector error_collector(in);
    parser.RecordErrorsTo(&error_collector);
    parser.ParseFromString(in, msg);
}


#endif
