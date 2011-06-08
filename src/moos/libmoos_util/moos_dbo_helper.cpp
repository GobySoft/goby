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
#include "goby/moos/libmoos_util/moos_serializer.h"
#include "goby/core/libdbo/dbo_manager.h"

void goby::moos::MOOSDBOPlugin::add_message(int unique_id, const std::string& identifier, const void* data, int size)
{
    CMOOSMsg msg;
    std::string bytes(static_cast<const char*>(data), size);
    goby::moos::MOOSSerializer::parse(&msg, bytes);
    goby::core::DBOManager::get_instance()->session()->add(new CMOOSMsg(msg));
}

void goby::moos::MOOSDBOPlugin::map_types()
{
    goby::core::DBOManager::get_instance()->session()->mapClass<CMOOSMsg>("CMOOSMsg");
}

extern "C" goby::core::DBOPlugin* create_goby_dbo_plugin()
{
    return new goby::moos::MOOSDBOPlugin;
}

extern "C" void destroy_goby_dbo_plugin(goby::core::DBOPlugin* plugin)
{
    delete plugin;
}
