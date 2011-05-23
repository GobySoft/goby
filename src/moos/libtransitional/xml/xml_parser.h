// copyright 2008, 2009 t. schneider tes@mit.edu
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

// based initially on work in C++ Cookbook by D. Ryan Stephens, Christopher Diggins, Jonathan Turkanis, and Jeff Cogswell. Copyright 2006 O'Reilly Media, INc., 0-596-00761-2.

#ifndef XML_PARSER20091211H
#define XML_PARSER20091211H


#include <exception>
#include <iostream>     // cout
#include <memory>       // auto_ptr
#include <vector>
#include <fstream>

#include <xercesc/sax2/SAX2XMLReader.hpp>
#include <xercesc/sax2/XMLReaderFactory.hpp>
#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/sax2/DefaultHandler.hpp>

#include "xerces_strings.h" 
#include "message_schema.xsd.h"

    
// RAII utility that initializes the parser and frees resources
// when it goes out of scope
class XercesInitializer {
  public:
    XercesInitializer( ) { xercesc::XMLPlatformUtils::Initialize( ); }
    ~XercesInitializer( ) { xercesc::XMLPlatformUtils::Terminate( ); }
  private:
    // Prohibit copying and assignment
    XercesInitializer(const XercesInitializer&);
    XercesInitializer& operator=(const XercesInitializer&);
};

class XMLParser {
  public:
  XMLParser(xercesc::DefaultHandler& content, xercesc::DefaultHandler& error)
      : content_(content),
        error_(error) 
        { }
        
    bool parse(const std::string& file)
    {
        // Initialize Xerces and obtain parser
        XercesInitializer init; 
        std::auto_ptr<xercesc::SAX2XMLReader> parser(xercesc::XMLReaderFactory::createXMLReader());

        // write XML schema to file
        const std::string schema = "/tmp/dccl_message_schema.xsd";
        std::ofstream fout;
        fout.open(schema.c_str());
        for(int i = 0; i < message_schema_xsd_len; ++i)
            fout << message_schema_xsd[i];
        fout.close();
        
        const XMLCh* xs_schema = fromNative(schema.c_str());
        const XMLCh* const schema_location = xs_schema;
        parser->setFeature(xercesc::XMLUni::fgSAX2CoreValidation, true);
        parser->setProperty(xercesc::XMLUni::fgXercesSchemaExternalNoNameSpaceSchemaLocation, (void*)schema_location);
        
        parser->setContentHandler(&content_);
        parser->setErrorHandler(&error_);
        
        // Parse the XML document
        parser->parse(file.c_str());
        
        return true;
    }       
        
  private:
    xercesc::DefaultHandler& content_;
    xercesc::DefaultHandler& error_;
};




#endif
