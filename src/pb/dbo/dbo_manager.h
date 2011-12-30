// copyright 2010 t. schneider tes@mit.edu
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

#ifndef DBOMANAGER20100901H
#define DBOMANAGER20100901H

#include <stdexcept>

#include <boost/shared_ptr.hpp>

#include "goby/common/core_constants.h"

#include <Wt/Dbo/Dbo>

#include "goby/util/time.h"
#include "dbo_plugin.h"


namespace Wt
{
    namespace Dbo
    {
        namespace backend
        {
            class Sqlite3;
        }
        
        class Session;
        class Transaction;
    }
}

namespace goby
{
    namespace core
    {
        struct RawEntry
        {
            MarshallingScheme marshalling_scheme;
            std::string identifier;
            std::vector<unsigned char> bytes;
            std::string time;
            int unique_id;
            int socket_id;
            
            template<class Action>
            void persist(Action& a)
                {
                    Wt::Dbo::field(a, unique_id, "raw_id");
                    Wt::Dbo::field(a, socket_id, "socket_id");
                    Wt::Dbo::field(a, time, "time_written");
                    Wt::Dbo::field(a, marshalling_scheme, "marshalling_scheme");
                    Wt::Dbo::field(a, identifier, "identifier");
                    Wt::Dbo::field(a, bytes, "bytes");
                }
        };

        /// \brief provides a way for Wt::Dbo to work with arbitrary
        /// (run-time provided) Google Protocol Buffers types
        class DBOManager
        {
          public:
            /// \brief singleton class: use this to get a pointer
            static DBOManager* get_instance();
            /// \brief if desired, this will release all resources (use right before exiting)
            static void shutdown();            
            
            void add_raw(MarshallingScheme marshalling_scheme,
                         const std::string& identifier,
                         const void* data,
                         int size,
                         int socket_id);
            
            
            /// \brief commit all changes to the Wt::Dbo SQL database
            void commit();

            /// \brief connect to the Wt::Dbo SQL database
            void connect(const std::string& db_name = "");            

            void set_table_prefix(const std::string& prefix);

            
            void reset_session();
            void map_types();
            void create_indices();
            Wt::Dbo::Session* session() { return session_; }

            DBOPluginFactory& plugin_factory()
            { return plugin_factory_; }

          private:    
            static DBOManager* inst_;
            DBOManager();
            
            ~DBOManager();
            
            DBOManager(const DBOManager&);
            DBOManager& operator = (const DBOManager&);

            std::string db_name_;

            // Wt Dbo has its own brand of memory management
            Wt::Dbo::backend::Sqlite3* connection_;
            Wt::Dbo::Session* session_;
            Wt::Dbo::Transaction* transaction_;

            boost::posix_time::ptime t_last_commit_;

            bool static_tables_created_;

            DBOPluginFactory plugin_factory_;

            std::string raw_id_table_name_;
        };


    }
}




#endif // DBOMANAGER20100901H
