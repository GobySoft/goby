// copyright 2011 t. schneider tes@mit.edu
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

#include "moos_dbo_helper.h"
#include "goby/moos/moos_serializer.h"
#include "goby/pb/dbo/dbo_manager.h"
#include "goby/util/logger.h"

void goby::moos::MOOSDBOPlugin::add_message(int unique_id, const std::string& identifier, const void* data, int size)
{
    CMOOSMsg msg;
    std::string bytes(static_cast<const char*>(data), size);
    goby::moos::MOOSSerializer::parse(&msg, bytes);
    add_message(unique_id, msg);
    
}

void goby::moos::MOOSDBOPlugin::add_message(int unique_id, const CMOOSMsg& msg)
{
    goby::core::DBOManager::get_instance()->session()->add(new std::pair<int, CMOOSMsg>(std::make_pair(unique_id, msg)));
}

void goby::moos::MOOSDBOPlugin::map_types()
{
    goby::core::DBOManager::get_instance()->session()->mapClass<std::pair<int, CMOOSMsg> >(table_name_.c_str());
}

void goby::moos::MOOSDBOPlugin::create_indices()
{
    // Session::execute added in Wt 3.1.3
#if WT_VERSION >= (((3 & 0xff) << 24) | ((1 & 0xff) << 16) | ((3 & 0xff) << 8))
    goby::core::DBOManager::get_instance()->session()->execute("CREATE UNIQUE INDEX IF NOT EXISTS " + table_name_ + "_raw_id_index" + " ON " + table_name_ + " (raw_id)");
    goby::core::DBOManager::get_instance()->session()->execute("CREATE INDEX IF NOT EXISTS " + table_name_ + "_moosmsg_time_index" + " ON " + table_name_ + " (moosmsg_time)");
#else
    goby::glog.is(warn) &&
        goby::glog << "execute() call not available in Wt Dbo versions 3.1.2 and older. Not creating any indices on the tables. Please upgrade Wt for automatic indexing support." << std::endl;
#endif
    
}

extern "C" goby::core::DBOPlugin* create_goby_dbo_plugin()
{
    return new goby::moos::MOOSDBOPlugin;
}

extern "C" void destroy_goby_dbo_plugin(goby::core::DBOPlugin* plugin)
{
    delete plugin;
}
