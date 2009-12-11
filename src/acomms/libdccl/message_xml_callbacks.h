// copyright 2008, 2009 t. schneider tes@mit.edu
// ocean engineering graudate student - mit / whoi joint program
// massachusetts institute of technology (mit)
// laboratory for autonomous marine sensing systems (lamss)
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

// xml code based initially on work in C++ Cookbook by D. Ryan Stephens, Christopher Diggins, Jonathan Turkanis, and Jeff Cogswell. Copyright 2006 O'Reilly Media, INc., 0-596-00761-2

#ifndef MESSAGE_XML_CALLBACKS_H
#define MESSAGE_XML_CALLBACKS_H

#include <stdexcept>                       // runtime_error
#include <vector>
#include <sstream>

#include <xercesc/sax2/Attributes.hpp>
#include <xercesc/sax2/DefaultHandler.hpp> 
#include <boost/algorithm/string.hpp> // for string functions
#include <boost/lexical_cast.hpp>

#include "acomms/xml/xerces_strings.h"

#include "message.h"
#include "message_xml_tags.h"

namespace dccl
{
    
// Implements callbacks that receive character data and
// notifications about the beginnings and ends of elements 
    class MessageContentHandler : public xercesc::DefaultHandler {
      public:
      MessageContentHandler(std::vector<Message> & messages) : messages(messages)
        {
            in_message = false; // true = in <message>
            in_layout = false; // true = in <layout>
            in_message_var = false; // true = in <int>, <bool>, <string>, <float>
            in_publish = false; // true = in <publish>

            initialize_tags(tags_map_);
        }

        void startElement( 
            const XMLCh *const uri,        // namespace URI
            const XMLCh *const localname,  // tagname w/ out NS prefix
            const XMLCh *const qname,      // tagname + NS pefix
            const xercesc::Attributes &attrs );      // elements's attributes

        void endElement(          
            const XMLCh *const uri,        // namespace URI
            const XMLCh *const localname,  // tagname w/ out NS prefix
            const XMLCh *const qname );     // tagname + NS pefix

#if XERCES_VERSION_MAJOR < 3
        void characters(const XMLCh* const chars, const unsigned int length)
	{ current_text.append(chars, length); }
#else
        void characters(const XMLCh* const chars, const XMLSize_t length )
	{ current_text.append(chars, length); }
#endif
    
      private:
        std::vector<Message>&  messages;
        XercesString current_text;

        bool in_message; // true = inside <message>
        bool in_layout; // true = inside <layout>
        bool in_message_var; // true = in <int>, <bool>, <string>, <float>
        XercesString type; // current message_var type: "int", "bool", etc.
        bool in_publish; // true = in <publish>
        std::map<std::string, Tags> tags_map_;
    
    };

// Receives Error notifications.
    class MessageErrorHandler : public xercesc::DefaultHandler 
    {

      public:
        void warning(const xercesc::SAXParseException& e)
        {
            std::cout << "warning:" << toNative(e.getMessage());
        }
        void error(const xercesc::SAXParseException& e)
        {
            XMLSSize_t line = e.getLineNumber();
            std::stringstream ss;
            ss << "message xml parsing error on line " << line << ": " << std::endl << toNative(e.getMessage());
            
            throw std::runtime_error(ss.str());
        }
        void fatalError(const xercesc::SAXParseException& e) 
        {
            error(e);
        }
    };
}

#endif
