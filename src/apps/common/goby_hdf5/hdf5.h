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

#ifndef GOBYHDF520160524H
#define GOBYHDF520160524H

#include <boost/algorithm/string.hpp>

#include "H5Cpp.h"

#include "goby/common/hdf5_plugin.h"
#include "goby/common/application_base.h"
#include "goby/common/protobuf/hdf5.pb.h"

namespace goby
{
    namespace common
    {
        namespace hdf5
        {
            template <typename T> H5::PredType predicate();
            template<> H5::PredType predicate<goby::int32>() { return H5::PredType::NATIVE_INT32; }
            template<> H5::PredType predicate<goby::int64>() { return H5::PredType::NATIVE_INT64; }
            template<> H5::PredType predicate<goby::uint32>() { return H5::PredType::NATIVE_UINT32; }
            template<> H5::PredType predicate<goby::uint64>() { return H5::PredType::NATIVE_UINT64; }
            template<> H5::PredType predicate<float>() { return H5::PredType::NATIVE_FLOAT; }
            template<> H5::PredType predicate<double>() { return H5::PredType::NATIVE_DOUBLE; }

            template <typename T> void retrieve_single_value(T* val, const google::protobuf::Reflection* refl,  const google::protobuf::FieldDescriptor* field_desc, const google::protobuf::Message& msg);
            
            struct Message
            {
            Message(const std::string& n) : name(n) { }
                std::string name;

                // time -> Message contents
                std::multimap<goby::uint64, boost::shared_ptr<google::protobuf::Message> > entries;
            };
    

            struct Channel
            {
            Channel(const std::string& n) : name(n) { }
                std::string name;

                void add_message(const goby::common::HDF5ProtobufEntry& entry)
                    {
                        const std::string& msg_name = entry.msg->GetDescriptor()->full_name();
                        typedef std::map<std::string, Message>::iterator It;
                        It it = entries.find(msg_name);
                        if(it == entries.end())
                        {
                            std::pair<It, bool> itpair = entries.insert(std::make_pair(msg_name, Message(msg_name)));
                            it = itpair.first;
                        }
                        it->second.entries.insert(std::make_pair(entry.time, entry.msg));
                    }
    
                // message name -> hdf5::Message
                std::map<std::string, Message> entries;
            };

            // keeps track of HDF5 groups for us, and creates them as needed
            class GroupFactory
            {
            public:
            GroupFactory(H5::H5File& h5file) :
                root_group_(h5file.openGroup("/"))
                {
                }

                // creates or opens group
                H5::Group& fetch_group(const std::string& group_path)
                {
                    std::deque<std::string> nodes;
                    std::string clean_path = boost::trim_copy_if(group_path, boost::algorithm::is_space() || boost::algorithm::is_any_of("/"));
                    boost::split(nodes, clean_path, boost::is_any_of("/"));
                    return root_group_.fetch_group(nodes);
                }
    
    
            private:
                class GroupWrapper
                {
                public:
                    // for root group (already exists)
                GroupWrapper(H5::Group group) : group_(group)
                    { }

                    // for children groups
                    GroupWrapper(const std::string& name, H5::Group& parent)
                    {
                        group_ = parent.createGroup(name);
                    }

                    H5::Group& fetch_group(std::deque<std::string>& nodes)
                    {
                        if(nodes.empty())
                        {
                            return group_;
                        }
                        else
                        {
                            typedef std::map<std::string, GroupWrapper>::iterator It;
                            It it = children_.find(nodes[0]);
                            if(it == children_.end())
                            {
                                std::pair<It, bool> itpair = children_.insert(std::make_pair(nodes[0], GroupWrapper(nodes[0], group_)));
                                it = itpair.first;
                            }
                            nodes.pop_front();
                            return it->second.fetch_group(nodes); 
                        }
                    }
        
        
                private:
                    H5::Group group_;
                    std::map<std::string, GroupWrapper> children_;
                };
                GroupWrapper root_group_;
            };
            
            class Writer : public goby::common::ApplicationBase
            {
            public:
                Writer(goby::common::protobuf::HDF5Config* cfg);

            private:
                void load();
                void collect();
                void write();
                void write_channel(const std::string& group, const goby::common::hdf5::Channel& channel);
                void write_message(const std::string& group, const goby::common::hdf5::Message& message);
                void write_time(const std::string& group, const goby::common::hdf5::Message& message);


                template<typename T>
                    void write_field(const std::string& group, const google::protobuf::FieldDescriptor* field_desc, const goby::common::hdf5::Message& message)
                {
                    std::vector<T> values(message.entries.size(), 0);
                    int i = 0;
                    for(std::multimap<goby::uint64, boost::shared_ptr<google::protobuf::Message> >::const_iterator it = message.entries.begin(), end = message.entries.end(); it != end; ++it)
                    {
                        const google::protobuf::Reflection* refl = it->second->GetReflection();
                        
                        if(field_desc->is_repeated())
                            continue; // TODO: support repeateds
                        else
                            retrieve_single_value<T>(&values[i], refl, field_desc, *it->second);
                        
                        ++i;
                    }
                    write_vector(group, field_desc->name(), values);
                }
                
                template<typename T>
                    void write_vector(const std::string& group, const std::string dataset_name, const std::vector<T>& data)
                {
                    const int rank = 1;
                    hsize_t dimsf[] = {data.size()}; 
                    H5::DataSpace dataspace(rank, dimsf, dimsf);
                    H5::Group& grp = group_factory_.fetch_group(group);
                    H5::DataSet dataset = grp.createDataSet(dataset_name,
                                                            predicate<T>(),
                                                            dataspace);
                    dataset.write(&data[0], predicate<T>());
                }
                
                
                void iterate() { }

            private:
                goby::common::protobuf::HDF5Config& cfg_;
                boost::shared_ptr<goby::common::HDF5Plugin> plugin_;

                // channel name -> hdf5::Channel
                std::map<std::string, goby::common::hdf5::Channel> channels_;
                H5::H5File h5file_;

                goby::common::hdf5::GroupFactory group_factory_;
            };



            template <>
                void retrieve_single_value(goby::int32* val,
                                           const google::protobuf::Reflection* refl,
                                           const google::protobuf::FieldDescriptor* field_desc,
                                           const google::protobuf::Message& msg)
            {
                if(field_desc->is_optional() && !refl->HasField(msg, field_desc))
                    *val = std::numeric_limits<goby::int32>::max();
                else
                    *val = refl->GetInt32(msg, field_desc);
            }

            template <>
                void retrieve_single_value(goby::uint32* val,
                                           const google::protobuf::Reflection* refl,
                                           const google::protobuf::FieldDescriptor* field_desc,
                                           const google::protobuf::Message& msg)
            {
                if(field_desc->is_optional() && !refl->HasField(msg, field_desc))
                    *val = std::numeric_limits<goby::uint32>::max();
                else
                    *val = refl->GetUInt32(msg, field_desc);
            }

            template <>
                void retrieve_single_value(goby::int64* val,
                                           const google::protobuf::Reflection* refl,
                                           const google::protobuf::FieldDescriptor* field_desc,
                                           const google::protobuf::Message& msg)
            {
                if(field_desc->is_optional() && !refl->HasField(msg, field_desc))
                    *val = std::numeric_limits<goby::int64>::max();
                else
                    *val = refl->GetInt64(msg, field_desc);
            }


            template <>
                void retrieve_single_value(goby::uint64* val,
                                           const google::protobuf::Reflection* refl,
                                           const google::protobuf::FieldDescriptor* field_desc,
                                           const google::protobuf::Message& msg)
            {
                if(field_desc->is_optional() && !refl->HasField(msg, field_desc))
                    *val = std::numeric_limits<goby::uint64>::max();
                else
                    *val = refl->GetUInt64(msg, field_desc);
            }


            template <>
                void retrieve_single_value(double* val,
                                           const google::protobuf::Reflection* refl,
                                           const google::protobuf::FieldDescriptor* field_desc,
                                           const google::protobuf::Message& msg)
            {
                if(field_desc->is_optional() && !refl->HasField(msg, field_desc))
                    *val = std::numeric_limits<double>::quiet_NaN();
                else
                    *val = refl->GetDouble(msg, field_desc);
            }

            template <>
                void retrieve_single_value(float* val,
                                           const google::protobuf::Reflection* refl,
                                           const google::protobuf::FieldDescriptor* field_desc,
                                           const google::protobuf::Message& msg)
            {
                if(field_desc->is_optional() && !refl->HasField(msg, field_desc))
                    *val = std::numeric_limits<float>::quiet_NaN();
                else
                    *val = refl->GetFloat(msg, field_desc);
            }
        }
    }
}




#endif
