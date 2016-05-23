// Copyright 2009-2016 Toby Schneider (http://gobysoft.org/index.wt/people/toby)
//                     GobySoft, LLC (2013-)
//                     Massachusetts Institute of Technology (2007-2014)
//
//
// This file is part of the Goby Underwater Autonomy Project Binaries
// ("The Goby Binaries").
//
// The Goby Binaries are free software: you can redistribute them and/or modify
// them under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// The Goby Binaries are distributed in the hope that they will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Goby.  If not, see <http://www.gnu.org/licenses/>.

#include <dlfcn.h>
#include <iostream>
#include <cstdlib>

#include "goby/common/application_base.h"
#include "goby/common/protobuf/hdf5.pb.h"
#include "goby/common/hdf5_plugin.h"
#include "goby/common/logger.h"

void* plugin_handle = 0;



using goby::glog;
using namespace goby::common::logger;


class HDF5Writer : public goby::common::ApplicationBase
{
public:
    HDF5Writer(goby::common::protobuf::HDF5Config* cfg);

private:
    void load();
    void collect();
    void write();
    
    void iterate() { }

private:
    goby::common::protobuf::HDF5Config& cfg_;
    boost::shared_ptr<goby::common::HDF5Plugin> plugin_;

    std::multimap<std::string, goby::common::HDF5ProtobufEntry> entries_;
};



int main(int argc, char * argv[])
{
    // load plugin driver from environmental variable GOBY_HDF5_PLUGIN
    const char* plugin_path = getenv ("GOBY_HDF5_PLUGIN");
    if (plugin_path)
    {
        std::cerr << "Loading plugin library: " << plugin_path << std::endl;
        plugin_handle = dlopen(plugin_path, RTLD_LAZY);
        if(!plugin_handle)
        {
            std::cerr << "Failed to open library: " << plugin_path << std::endl;
            exit(EXIT_FAILURE);
        }
    }        
    else
    {
        std::cerr << "Environmental variable GOBY_HDF5_PLUGIN must be set with name of the dynamic library containing the specific frontend plugin to use." << std::endl;
        exit(EXIT_FAILURE);
    }

    goby::common::protobuf::HDF5Config cfg;
    return goby::run<HDF5Writer>(argc, argv, &cfg);
}


HDF5Writer::HDF5Writer(goby::common::protobuf::HDF5Config* cfg)
    : ApplicationBase(cfg),
      cfg_(*cfg)
{
    load();
    collect();
    write();
    quit();    
}

void HDF5Writer::load()
{
    typedef goby::common::HDF5Plugin* (*plugin_func)(goby::common::protobuf::HDF5Config*);
    plugin_func plugin_ptr = (plugin_func) dlsym(plugin_handle, "goby_hdf5_load");

    if(!plugin_ptr)
        glog.is(DIE) && glog << "Function goby_hdf5_load in library defined in GOBY_HDF5_PLUGIN does not exist." <<  std::endl;

    plugin_.reset((*plugin_ptr)(&cfg_));
    
    if(!plugin_)
        glog.is(DIE) && glog << "Function goby_hdf5_load in library defined in GOBY_HDF5_PLUGIN returned a null pointer." <<  std::endl;    
}


void HDF5Writer::collect()
{
    goby::common::HDF5ProtobufEntry entry;
    while(plugin_->provide_entry(&entry))
    {
        entries_.insert(std::make_pair(entry.channel, entry));
        entry.clear();        
    }
}

    
void HDF5Writer::write()
{
    for(std::multimap<std::string, goby::common::HDF5ProtobufEntry>::const_iterator it = entries_.begin(), n = entries_.end(); it != n; ++it)
    {
        std::cout << it->second << std::endl;
    }
}

