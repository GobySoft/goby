// copyright 2011 t. schneider tes@mit.edu
// 
// this file is part of goby_dbo
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

#ifndef DBOPLUGIN20110608H
#define DBOPLUGIN20110608H

#include <Wt/Dbo/Dbo>
#include <Wt/Dbo/backend/Sqlite3>

#include "goby/core/core_constants.h"

namespace goby
{
    namespace core
    {
        class DBOPlugin
        {
          public:
            typedef goby::core::DBOPlugin* create_t();
            typedef void destroy_t(goby::core::DBOPlugin*);
            
            DBOPlugin() { }
            virtual ~DBOPlugin() { }

            virtual MarshallingScheme provides() = 0;
            virtual void add_message(Wt::Dbo::Session* session, int unique_id, const std::string& identifier, const void* data, int size) = 0;
            virtual void map_type(Wt::Dbo::Session* session) = 0;
        };

        class ProtobufDBOPlugin : public DBOPlugin
        {
          public:
            
            ProtobufDBOPlugin() { }
            ~ProtobufDBOPlugin() { }

            MarshallingScheme provides();
            void add_message(Wt::Dbo::Session* session, int unique_id, const std::string& identifier, const void* data, int size);
            void map_type(Wt::Dbo::Session* session);
        };
        
        
        class DBOPluginFactory
        {
          public:

            DBOPluginFactory()
            { }
            ~ DBOPluginFactory()
            {
                
            }
            
            void add_library(const std::string& lib_name);
            void add_plugin(DBOPlugin* plugin);
                

            DBOPlugin* plugin(goby::core::MarshallingScheme scheme);
          private:
            class PluginLibrary
            {
              public:
                PluginLibrary(const std::string& lib_name);
              PluginLibrary(DBOPlugin* plugin)
                  : handle_(0), plugin_(plugin), destroy_(0), create_(0)
                { }
                ~PluginLibrary();
                
                DBOPlugin* plugin() 
                { return plugin_; }
                
              private:
                
                PluginLibrary(const PluginLibrary&);
                PluginLibrary& operator=(const PluginLibrary&);
                
                void* handle_;
                DBOPlugin* plugin_;
                goby::core::DBOPlugin::destroy_t* destroy_;
                goby::core::DBOPlugin::create_t* create_;
            };
            
            std::map<goby::core::MarshallingScheme, boost::shared_ptr<PluginLibrary> > plugins_;
        };
        
        

    }
}


#endif
