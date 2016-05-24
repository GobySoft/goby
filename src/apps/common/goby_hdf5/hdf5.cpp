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

#include "goby/common/logger.h"

#include "hdf5.h"

void* plugin_handle = 0;

using namespace goby::common::logger;
using goby::glog;

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
    return goby::run<goby::common::hdf5::Writer>(argc, argv, &cfg);
}


goby::common::hdf5::Writer::Writer(goby::common::protobuf::HDF5Config* cfg)
    : ApplicationBase(cfg),
      cfg_(*cfg),
      h5file_(cfg_.output_file(), H5F_ACC_TRUNC),
      group_factory_(h5file_)
{
    load();
    collect();
    write();
    quit();    
}

void goby::common::hdf5::Writer::load()
{
    typedef goby::common::HDF5Plugin* (*plugin_func)(goby::common::protobuf::HDF5Config*);
    plugin_func plugin_ptr = (plugin_func) dlsym(plugin_handle, "goby_hdf5_load");

    if(!plugin_ptr)
        glog.is(DIE) && glog << "Function goby_hdf5_load in library defined in GOBY_HDF5_PLUGIN does not exist." <<  std::endl;

    plugin_.reset((*plugin_ptr)(&cfg_));
    
    if(!plugin_)
        glog.is(DIE) && glog << "Function goby_hdf5_load in library defined in GOBY_HDF5_PLUGIN returned a null pointer." <<  std::endl;    

}


void goby::common::hdf5::Writer::collect()
{
    goby::common::HDF5ProtobufEntry entry;
    while(plugin_->provide_entry(&entry))
    {
        boost::trim_if(entry.channel, boost::algorithm::is_space() || boost::algorithm::is_any_of("/"));

        typedef std::map<std::string, goby::common::hdf5::Channel>::iterator It;
        It it = channels_.find(entry.channel);
        if(it == channels_.end())
        {
            std::pair<It, bool> itpair = channels_.insert(std::make_pair(entry.channel, goby::common::hdf5::Channel(entry.channel)));
            it = itpair.first;
        }
        
        
        it->second.add_message(entry);
        entry.clear();
    }
}

    
void goby::common::hdf5::Writer::write()
{
    for(std::map<std::string, goby::common::hdf5::Channel>::const_iterator it = channels_.begin(), end = channels_.end(); it != end; ++it)
        write_channel("/" + it->first, it->second);
}

void goby::common::hdf5::Writer::write_channel(const std::string& group, const goby::common::hdf5::Channel& channel)
{
    std::cout << "Channel: [" << channel.name << "]" << std::endl;
    for(std::map<std::string, goby::common::hdf5::Message>::const_iterator it = channel.entries.begin(), end = channel.entries.end(); it != end; ++it)
        write_message(group + "/" + it->first, it->second);    
}


void goby::common::hdf5::Writer::write_message(const std::string& group, const goby::common::hdf5::Message& message)
{
    std::cout << "Message: [" << message.name << "]" << std::endl;
    std::cout << "group: [" << group << "]" << std::endl;

    write_time(group, message);
    
    const google::protobuf::Descriptor* desc = message.entries.begin()->second->GetDescriptor();
    for(int i = 0, n = desc->field_count(); i < n; ++i)
    {
        const google::protobuf::FieldDescriptor* field_desc = desc->field(i);
        switch(field_desc->cpp_type())
        {
            case google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE:
                break;
                            
            case google::protobuf::FieldDescriptor::CPPTYPE_ENUM:
                break;
                                
            case google::protobuf::FieldDescriptor::CPPTYPE_INT32:
                write_field<goby::int32>(group, field_desc, message);
                break;
                            
            case google::protobuf::FieldDescriptor::CPPTYPE_INT64:
                write_field<goby::int64>(group, field_desc, message);
                break;
                            
            case google::protobuf::FieldDescriptor::CPPTYPE_UINT32:
                write_field<goby::uint32>(group, field_desc, message);
                break;
                            
            case google::protobuf::FieldDescriptor::CPPTYPE_UINT64:
                write_field<goby::uint64>(group, field_desc, message);
                break;
                            
            case google::protobuf::FieldDescriptor::CPPTYPE_BOOL:
                //write_field<bool>(group, field_desc, message);
                break;
                            
            case google::protobuf::FieldDescriptor::CPPTYPE_STRING:
                // if(field_desc->type() ==  google::protobuf::FieldDescriptor::TYPE_STRING)
                //     write_field<std::string>(group, field_desc, message);
                // else if(field_desc->type() ==  google::protobuf::FieldDescriptor::TYPE_BYTES)
                //     write_field<std::string>(group, field_desc, message);
                break;                    
                
            case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT:                
                write_field<float>(group, field_desc, message);
                break;
                            
            case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE:
                write_field<double>(group, field_desc, message);
                break;
        }

    }
    
}


void goby::common::hdf5::Writer::write_time(const std::string& group, const goby::common::hdf5::Message& message)
{
    std::vector<goby::uint64> utime(message.entries.size(), 0);
    std::vector<double> datenum(message.entries.size(), 0);
    int i = 0;
    for(std::multimap<goby::uint64, boost::shared_ptr<google::protobuf::Message> >::const_iterator it = message.entries.begin(), end = message.entries.end(); it != end; ++it)
    {
        utime[i] = it->first;
        // datenum(1970, 1, 1, 0, 0, 0)
        const double datenum_unix_epoch = 719529;
        const double seconds_in_day = 86400;
        goby::uint64 utime_sec = utime[i]/1000000;
        goby::uint64 utime_frac = utime[i] - utime_sec*1000000;
        datenum[i] = datenum_unix_epoch + static_cast<double>(utime_sec)/seconds_in_day + static_cast<double>(utime_frac)/1000000/seconds_in_day;
        ++i;
    }
    write_vector(group, "_utime_", utime);
    write_vector(group, "_datenum_", datenum);
}
