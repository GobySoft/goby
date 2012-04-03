// Copyright 2009-2012 Toby Schneider (https://launchpad.net/~tes)
//                     Massachusetts Institute of Technology (2007-)
//                     Woods Hole Oceanographic Institution (2007-)
//                     Goby Developers Team (https://launchpad.net/~goby-dev)
// 
//
// This file is part of the Goby Underwater Autonomy Project Libraries
// ("The Goby Libraries").
//
// The Goby Libraries are free software: you can redistribute them and/or modify
// them under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// The Goby Libraries are distributed in the hope that they will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with Goby.  If not, see <http://www.gnu.org/licenses/>.


#include <stdexcept>
#include <algorithm>

#include <Wt/Dbo/backend/Sqlite3>

#include "goby/util/as.h"
#include "goby/common/time.h"
#include "goby/common/logger.h"
#include "wt_dbo_overloads.h"
#include "goby/pb/dbo/dbo_manager.h"

#include "dbo_plugin.h"

using namespace goby::common::logger;

goby::common::DBOManager* goby::common::DBOManager::inst_ = 0;

using goby::common::goby_time;
using goby::glog;


// singleton class, use this to get pointer
goby::common::DBOManager* goby::common::DBOManager::get_instance()
{
    if(!inst_) inst_ = new goby::common::DBOManager();
    return(inst_);
}

void goby::common::DBOManager::shutdown()
{
    if(inst_) delete inst_;
}


goby::common::DBOManager::DBOManager()
       :connection_(0),
        session_(0),
        transaction_(0),
        t_last_commit_(goby_time()),
        static_tables_created_(false),
        raw_id_table_name_("_raw")
{
    glog.add_group("dbo", goby::common::Colors::lt_green, "database");
}

goby::common::DBOManager::~DBOManager()
{
    if(transaction_) delete transaction_;
    if(connection_) delete connection_;
    if(session_) delete session_;
}


void goby::common::DBOManager::add_raw(MarshallingScheme marshalling_scheme,
                                     const std::string& identifier,
                                     const void* data,
                                     int size,
                                     int socket_id)
{
    static int i = -1;
    ++i;
    
    RawEntry *new_entry = new RawEntry();
    new_entry->marshalling_scheme = marshalling_scheme;
    new_entry->identifier = identifier;
    new_entry->unique_id = i;
    new_entry->socket_id = socket_id;

    std::string bytes = std::string(static_cast<const char*>(data), size);
    new_entry->bytes = std::vector<unsigned char>(bytes.begin(), bytes.end());
    new_entry->time = goby::util::as<std::string>(goby_time());
                
    session_->add(new_entry);

    DBOPlugin* plugin = plugin_factory_.plugin(marshalling_scheme);
    if(plugin)
        plugin->add_message(i, identifier, data, size);
}

void goby::common::DBOManager::commit()
{
    glog.is(VERBOSE) &&
        glog << group("dbo") << "starting commit" << std::endl;
        
    transaction_->commit();
        
    glog.is(VERBOSE) &&
        glog << group("dbo") << "finished commit" << std::endl;

    t_last_commit_ = goby_time();

    delete transaction_;
    transaction_ = new Wt::Dbo::Transaction(*session_);
}



void goby::common::DBOManager::connect(const std::string& db_name /* = "" */)
{
    if(!db_name.empty())
        db_name_ = db_name;

    if(transaction_) delete transaction_;
    if(connection_) delete connection_;
    if(session_) delete session_;
    connection_ = new Wt::Dbo::backend::Sqlite3(db_name_);
    session_ = new Wt::Dbo::Session;
    session_->setConnection(*connection_);
    // transaction deleted by session
    transaction_ = new Wt::Dbo::Transaction(*session_);
    
    if(!static_tables_created_)
    {
        map_types();

        try { session_->createTables(); }
        catch(Wt::Dbo::Exception& e)
        {
            glog.is(WARN) &&
                glog << "Could not create static type tables; reason: " << e.what() << std::endl;
        }

        glog.is(VERBOSE) &&
            glog << group("dbo") << "created tables for static types" << std::endl;
        static_tables_created_ = true;
        commit();
        create_indices();
            
//       reset_session();
    }
}

void goby::common::DBOManager::reset_session()
{
    commit();
    glog.is(VERBOSE) &&
        glog << "resetting session" << std::endl;
    connect();
}
            

void goby::common::DBOManager::map_types()
{
    session_->mapClass<RawEntry>(raw_id_table_name_.c_str());
    
    std::set<DBOPlugin*> plugins = plugin_factory_.plugins();
    BOOST_FOREACH(DBOPlugin* plugin, plugins)    
    {
        if(plugin)
            plugin->map_types();
    }
}

void goby::common::DBOManager::create_indices()
{
#if WT_VERSION >= (((3 & 0xff) << 24) | ((1 & 0xff) << 16) | ((3 & 0xff) << 8))    
    goby::common::DBOManager::get_instance()->session()->execute("CREATE INDEX IF NOT EXISTS " + raw_id_table_name_ + "_id_index ON " + raw_id_table_name_ + " (raw_id)");
    
    std::set<DBOPlugin*> plugins = plugin_factory_.plugins();
    BOOST_FOREACH(DBOPlugin* plugin, plugins)    
    {
        if(plugin)
            plugin->create_indices();
    }
#else
    glog.is(WARN) &&
        glog << "execute() call not available in Wt Dbo versions 3.1.2 and older. Not creating any indices on the tables. Please upgrade Wt for automatic indexing support." << std::endl;
#endif
}


void goby::common::DBOManager::set_table_prefix(const std::string& prefix)
{
    raw_id_table_name_ = prefix + "_raw";
    
    
    std::set<DBOPlugin*> plugins = plugin_factory_.plugins();
    BOOST_FOREACH(DBOPlugin* plugin, plugins)    
    {
        if(plugin)
            plugin->set_table_prefix(prefix);
    }
}
