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

#include "goby/pb/dbo/dbo_plugin.h"
#include <dlfcn.h>
#include "goby/common/logger.h"
#include "goby/pb/dbo/dbo_manager.h"
#include "goby/pb/dbo/wt_dbo_overloads.h"
#include <google/protobuf/descriptor.pb.h>
#include "goby/util/as.h"

#include <boost/preprocessor.hpp>

using namespace goby::common::logger;
using goby::util::as;


boost::bimap<int, std::string> goby::common::ProtobufDBOPlugin::dbo_map;

void goby::common::DBOPluginFactory::add_plugin(DBOPlugin* plugin)
{
    // create an instance of the class
    boost::shared_ptr<PluginLibrary> new_plugin_library(new PluginLibrary(plugin));
    plugins_[new_plugin_library->plugin()->provides()] = new_plugin_library;
}

void goby::common::DBOPluginFactory::add_library(const std::string& lib_name)
{
    // create an instance of the class
    boost::shared_ptr<PluginLibrary> new_plugin_library(new PluginLibrary(lib_name));
    plugins_[new_plugin_library->plugin()->provides()] = new_plugin_library;
}


goby::common::DBOPlugin* goby::common::DBOPluginFactory::plugin(goby::common::MarshallingScheme scheme)
{
    std::map<goby::common::MarshallingScheme, boost::shared_ptr<PluginLibrary> >::iterator it =  plugins_.find(scheme);

    return (it == plugins_.end()) ? 0 : it->second->plugin();
}

std::set<goby::common::DBOPlugin*> goby::common::DBOPluginFactory::plugins()
{
    std::set<DBOPlugin*> plugins;
    for(std::map<goby::common::MarshallingScheme, boost::shared_ptr<PluginLibrary> >::iterator it = plugins_.begin(), n = plugins_.end(); it != n; ++it)
        plugins.insert(it->second->plugin());

    return plugins;
}            

goby::common::DBOPluginFactory::PluginLibrary::PluginLibrary(const std::string& lib_name)
    : handle_(dlopen(lib_name.c_str(), RTLD_LAZY))
{
    if (!handle_) {
        goby::glog << die << "Cannot load library: " << dlerror() << '\n';
    }
                    
    // load the symbols
    create_ = (goby::common::DBOPlugin::create_t*) dlsym(handle_, "create_goby_dbo_plugin");
    destroy_ = (goby::common::DBOPlugin::destroy_t*) dlsym(handle_, "destroy_goby_dbo_plugin");
    if (!create_ || !destroy_) {
        goby::glog << die << "Cannot load symbols: " << dlerror() << std::endl;
    }
                    
    plugin_ = create_();
}

goby::common::DBOPluginFactory::PluginLibrary::~PluginLibrary()
{
    if(destroy_)
        destroy_(plugin_);
    if(handle_)
        dlclose(handle_);
}

goby::common::ProtobufDBOPlugin::ProtobufDBOPlugin()
    : index_(0)
{
}

goby::common::MarshallingScheme goby::common::ProtobufDBOPlugin::provides()
{
    return goby::common::MARSHALLING_PROTOBUF;
}

void goby::common::ProtobufDBOPlugin::add_message(int unique_id, const std::string& identifier, const void* data, int size)
{
    const std::string protobuf_type_name = identifier.substr(0, identifier.find("/"));
        
    boost::shared_ptr<google::protobuf::Message> msg =
        goby::util::DynamicProtobufManager::new_protobuf_message(protobuf_type_name);
    msg->ParseFromArray(data, size);
    add_message(unique_id, msg);
}

void goby::common::ProtobufDBOPlugin::add_message(int unique_id, boost::shared_ptr<google::protobuf::Message> msg)
{
    using goby::util::as;
    
    if(!dbo_map.right.count(msg->GetDescriptor()->full_name()))
        add_type(msg->GetDescriptor());

    
    glog.is(DEBUG1) &&
        glog << group("dbo") << "adding message of type: " << msg->GetTypeName() << std::endl;
    
    switch(dbo_map.right.at(msg->GetTypeName()))
    {

        // preprocessor `for` loop from 0 to GOBY_MAX_PROTOBUF_TYPES
#define BOOST_PP_LOCAL_MACRO(n)                                      \
        case n: goby::common::DBOManager::get_instance()->session()->add(new ProtoBufWrapper<n>(unique_id, msg)); break;
#define BOOST_PP_LOCAL_LIMITS (0, GOBY_MAX_PROTOBUF_TYPES)
#include BOOST_PP_LOCAL_ITERATE()
        // end preprocessor `for` loop

        default: throw(std::runtime_error(std::string("exceeded maximum number of types allowed: " + goby::util::as<std::string>(GOBY_MAX_PROTOBUF_TYPES)))); break;
    }

}

    
void goby::common::ProtobufDBOPlugin::map_types()
{
    for(boost::bimap<int, std::string>::left_iterator it = dbo_map.left.begin(),
            n = dbo_map.left.end();
        it != n;
        ++it)
    {
        map_type(goby::util::DynamicProtobufManager::descriptor_pool().FindMessageTypeByName(it->second));
    }
}

void goby::common::ProtobufDBOPlugin::add_type(const google::protobuf::Descriptor* descriptor)
{
    using goby::util::as;
    
    if(dbo_map.right.count(descriptor->full_name()))
    {
        glog.is(VERBOSE) &&
            glog << group("dbo") << "type with name " << descriptor->full_name()
                 << " already exists" << std::endl;
        return;
    }

    glog.is(VERBOSE) &&
        glog << group("dbo") << "adding type: "
             << descriptor->DebugString() << "\n"
             << "with index: " << index_ << std::endl;
    
    goby::common::DBOManager::get_instance()->reset_session();
    dbo_map.insert(boost::bimap<int, std::string>::value_type(index_, descriptor->full_name()));

    std::string mangled_name = table_prefix_ + descriptor->full_name();
    std::replace(mangled_name.begin(), mangled_name.end(), '.', '_');
    table_names_.insert(std::make_pair(index_, mangled_name));
    ++index_;

    map_type(descriptor);

    try
    { goby::common::DBOManager::get_instance()->session()->createTables(); }
    catch(Wt::Dbo::Exception& e)
    {
        glog.is(WARN) && glog << group("dbo")
                             << "Could not create table for " << mangled_name << "; reason: "
                             << e.what() << std::endl;
    }
    
    glog.is(VERBOSE) &&
        glog << group("dbo") << "created table for " << mangled_name << std::endl;


    // Session::execute added in Wt 3.1.3
#if WT_VERSION >= (((3 & 0xff) << 24) | ((1 & 0xff) << 16) | ((3 & 0xff) << 8))
    // create raw_id index    
    goby::common::DBOManager::get_instance()->session()->execute("CREATE UNIQUE INDEX IF NOT EXISTS " + mangled_name + "_raw_id_index" + " ON " + mangled_name + " (raw_id)");
#else
    glog.is(WARN) &&
        glog << "execute() call not available in Wt Dbo versions 3.1.2 and older. Not creating any indices on the tables. Please upgrade Wt for automatic indexing support." << std::endl;
#endif
    
    goby::common::DBOManager::get_instance()->reset_session();

    // remap all the tables
    goby::common::DBOManager::get_instance()->map_types();
}

void goby::common::ProtobufDBOPlugin::map_type(const google::protobuf::Descriptor* descriptor)
{
    using goby::util::as;
    
    glog.is(VERBOSE) &&
        glog <<group("dbo") << "mapping type: " << descriptor->full_name() << std::endl;
    
    // allows us to select compile time type to use at runtime
    switch(dbo_map.right.at(descriptor->full_name()))
    {
        // preprocessor `for` loop from 0 to GOBY_MAX_PROTOBUF_TYPES
#define BOOST_PP_LOCAL_MACRO(n)                                         \
        case n: goby::common::DBOManager::get_instance()->session()->mapClass< ProtoBufWrapper<n> >(table_names_.find(n)->second.c_str()); break;
#define BOOST_PP_LOCAL_LIMITS (0, GOBY_MAX_PROTOBUF_TYPES)
#include BOOST_PP_LOCAL_ITERATE()
        // end preprocessor `for` loop
        
        default: throw(std::runtime_error(std::string("exceeded maximum number of types allowed: " + as<std::string>(GOBY_MAX_PROTOBUF_TYPES)))); break;
    }
}

