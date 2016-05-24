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
#include "goby/util/binary.h"

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
            template<> H5::PredType predicate<unsigned char>() { return H5::PredType::NATIVE_UCHAR; }
 
            struct PBMeta
            {
                PBMeta(const google::protobuf::Reflection* r,
                       const google::protobuf::FieldDescriptor* f,
                       const google::protobuf::Message& m) : refl(r), field_desc(f), msg(m)
                    { }
                
                const google::protobuf::Reflection* refl;
                const google::protobuf::FieldDescriptor* field_desc;
                const google::protobuf::Message& msg;
            };
            
            template <typename T> void retrieve_empty_value(T* val);            
            template <typename T> void retrieve_single_value(T* val, PBMeta m)
            {
                if(m.field_desc->is_optional() && !m.refl->HasField(m.msg, m.field_desc))
                    retrieve_empty_value(val);
                else
                    retrieve_single_present_value(val, m);
            }
            
            template <typename T> void retrieve_single_present_value(T* val, PBMeta meta);
            template <typename T> void retrieve_repeated_value(T* val, int index, PBMeta meta);
            
            struct MessageCollection
            {
            MessageCollection(const std::string& n) : name(n) { }
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
                        typedef std::map<std::string, MessageCollection>::iterator It;
                        It it = entries.find(msg_name);
                        if(it == entries.end())
                        {
                            std::pair<It, bool> itpair = entries.insert(std::make_pair(msg_name, MessageCollection(msg_name)));
                            it = itpair.first;
                        }
                        it->second.entries.insert(std::make_pair(entry.time, entry.msg));
                    }
    
                // message name -> hdf5::Message
                std::map<std::string, MessageCollection> entries;
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
                void write_message_collection(const std::string& group, const goby::common::hdf5::MessageCollection& message_collection);
                void write_time(const std::string& group, const goby::common::hdf5::MessageCollection& message_collection);

                void write_field_selector(const std::string& group, const google::protobuf::FieldDescriptor* field_desc, const std::vector<const google::protobuf::Message* >& messages);

                template<typename T>
                    void write_field(const std::string& group, const google::protobuf::FieldDescriptor* field_desc, const std::vector<const google::protobuf::Message* >& messages)
                {
                    if(field_desc->is_repeated())
                    {
                        std::vector<std::vector<T> > values(messages.size(), std::vector<T>());
                    
                        int i = 0;
                        int max_field_size = 0;
                        for(std::vector<const google::protobuf::Message* >::const_iterator it = messages.begin(), end = messages.end(); it != end; ++it)
                        {
                            const google::protobuf::Reflection* refl = (*it)->GetReflection();
                            std::vector<T>& v_row = values[i];
                            int field_size = refl->FieldSize(**it, field_desc);
                            if(field_size > max_field_size) max_field_size = field_size;
                            v_row.resize(field_size);

                            for(int j = 0; j < field_size; ++j)
                                retrieve_repeated_value<T>(&v_row[j], j, PBMeta(refl, field_desc, (**it)));
                            ++i;
                        }
                        write_matrix(group, field_desc->name(), values, max_field_size);
                    }
                    else
                    {
                        std::vector<T> values(messages.size(), T());
                    
                        int i = 0;
                        for(std::vector<const google::protobuf::Message* >::const_iterator it = messages.begin(), end = messages.end(); it != end; ++it)
                        {
                            const google::protobuf::Reflection* refl = (*it)->GetReflection();
                            retrieve_single_value<T>(&values[i], PBMeta(refl, field_desc, (**it)));
                            ++i;
                        }
                        write_vector(group, field_desc->name(), values);
                    }
                }

                
                void write_embedded_message(const std::string& group, const google::protobuf::FieldDescriptor* field_desc, const std::vector<const google::protobuf::Message*> messages);

                

                
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
                    if(data.size())
                        dataset.write(&data[0], predicate<T>());
                }


                template<typename T>
                    void write_matrix(const std::string& group, const std::string dataset_name, const std::vector<std::vector<T> >& data, int max_field_size)
                {
                    hsize_t nx = data.size(), ny = max_field_size;
                    const int rank = 2;
                    hsize_t dimsf[] = { nx, ny };

                    T cdata[nx][ny];
                    for(int i = 0; i < nx; ++i)
                    {
                        const std::vector<T>& inner = data[i];
                        for(int j = 0; j < ny; ++j)
                        {
                            if(j < inner.size())
                                cdata[i][j] = data[i][j];
                            else
                                retrieve_empty_value(&cdata[i][j]);
                        }
                    }
                    
                    H5::DataSpace dataspace(rank, dimsf, dimsf);
                    H5::Group& grp = group_factory_.fetch_group(group);

                    H5::DataSet dataset = grp.createDataSet(dataset_name,
                                                            predicate<T>(),
                                                            dataspace);
                    dataset.write(cdata, predicate<T>());
                }

                
                
                void write_vector(const std::string& group,
                                  const std::string dataset_name,
                                  const std::vector<std::string>& data)
                {                   
                    std::vector<const char*> data_c_str;
                    for (unsigned i = 0, n = data.size(); i < n; ++i) 
                        data_c_str.push_back(data[i].c_str());
                    
                    const int rank = 1;
                    hsize_t dimsf[] = {data.size()}; 
                    H5::DataSpace dataspace(rank, dimsf, dimsf);
                    H5::StrType datatype(H5::PredType::C_S1, H5T_VARIABLE);          
                    H5::Group& grp = group_factory_.fetch_group(group);
                    H5::DataSet dataset = grp.createDataSet(dataset_name,
                                                            datatype,
                                                            dataspace);

                    if(data_c_str.size())
                        dataset.write(data_c_str.data(), datatype);
                }
                

                void write_matrix(const std::string& group,
                                  const std::string dataset_name,
                                  const std::vector<std::vector<std::string > >& data,
                                  int max_field_size)
                {                   
                    // TODO: implement if this makes sense
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
                void retrieve_empty_value(goby::int32* val)
            { *val = std::numeric_limits<goby::int32>::max(); }
            template <>
                void retrieve_single_present_value(goby::int32* val, PBMeta m)
            { *val = m.refl->GetInt32(m.msg, m.field_desc); }
            template <>
                void retrieve_repeated_value(goby::int32* val, int index, PBMeta m)
            { *val = m.refl->GetRepeatedInt32(m.msg, m.field_desc, index); }

            template <>
                void retrieve_empty_value(goby::uint32* val)
            { *val = std::numeric_limits<goby::uint32>::max(); }
            template <>
                void retrieve_single_present_value(goby::uint32* val, PBMeta m)
            { *val = m.refl->GetUInt32(m.msg, m.field_desc); }
            template <>
                void retrieve_repeated_value(goby::uint32* val, int index, PBMeta m)
            { *val = m.refl->GetRepeatedUInt32(m.msg, m.field_desc, index); }

            template <>
                void retrieve_empty_value(goby::int64* val)
            { *val = std::numeric_limits<goby::int64>::max(); }
            template <>
                void retrieve_single_present_value(goby::int64* val, PBMeta m)
            { *val = m.refl->GetInt64(m.msg, m.field_desc); }
            template <>
                void retrieve_repeated_value(goby::int64* val, int index, PBMeta m)
            { *val = m.refl->GetRepeatedInt64(m.msg, m.field_desc, index); }


            template <>
                void retrieve_empty_value(goby::uint64* val)
            { *val = std::numeric_limits<goby::uint64>::max(); }
            template <>
                void retrieve_single_present_value(goby::uint64* val, PBMeta m)
            { *val = m.refl->GetUInt64(m.msg, m.field_desc); }
            template <>
                void retrieve_repeated_value(goby::uint64* val, int index, PBMeta m)
            { *val = m.refl->GetRepeatedUInt64(m.msg, m.field_desc, index); }


            template <>
                void retrieve_empty_value(double* val)
            { *val = std::numeric_limits<double>::quiet_NaN(); }
            template <>
                void retrieve_single_present_value(double* val, PBMeta m)
            { *val = m.refl->GetDouble(m.msg, m.field_desc); }
            template <>
                void retrieve_repeated_value(double* val, int index, PBMeta m)
            { *val = m.refl->GetRepeatedDouble(m.msg, m.field_desc, index); }

            template <>
                void retrieve_empty_value(float* val)
            { *val = std::numeric_limits<float>::quiet_NaN(); }
            template <>
                void retrieve_single_present_value(float* val, PBMeta m)
            { *val = m.refl->GetFloat(m.msg, m.field_desc); }
            template <>
                void retrieve_repeated_value(float* val, int index, PBMeta m)
            { *val = m.refl->GetRepeatedFloat(m.msg, m.field_desc, index); }


            // used for bool
            template <>
                void retrieve_empty_value(unsigned char* val)
            { *val = std::numeric_limits<unsigned char>::max(); }
            template <>
                void retrieve_single_present_value(unsigned char* val, PBMeta m)
            {
                if(m.field_desc->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_BOOL)
                    *val = m.refl->GetBool(m.msg, m.field_desc);
            }
            template <>
                void retrieve_repeated_value(unsigned char* val, int index, PBMeta m)
            {
                if(m.field_desc->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_BOOL)
                    *val = m.refl->GetRepeatedBool(m.msg, m.field_desc, index);
            }


            template <>
                void retrieve_empty_value(std::string* val)
            { val->clear(); }
            template <>
                void retrieve_single_value(std::string* val, PBMeta m)
            {
                    *val = m.refl->GetString(m.msg, m.field_desc);
                    if(m.field_desc->type() == google::protobuf::FieldDescriptor::TYPE_BYTES)
                        *val = goby::util::hex_encode(*val);                
            }
            template <>
                void retrieve_repeated_value(std::string* val, int index, PBMeta m)
            {
                // TODO: implement if this makes sense
            }

            
        }
    }
}




#endif
