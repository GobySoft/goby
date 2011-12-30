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

#include <iostream>
#include <fstream>

#include "goby/moos/moos_dbo_helper.h"
#include "goby/moos/moos_node.h"
#include "goby/moos/moos_protobuf_helpers.h"
#include "goby/pb/application_base.h"
#include "goby/pb/dbo/dbo_manager.h"

#include "MOOSGenLib/MOOSGenLibGlobalHelper.h"

#include "alog_to_goby_db_config.pb.h"
#include "moos_types.pb.h"

using goby::moos::operator<<;
using namespace goby::util::logger;


class AlogToGobyDb : public goby::core::ApplicationBase
{
public:
    
    AlogToGobyDb()
        : ApplicationBase(&cfg_)
        {
            goby::moos::MOOSDBOPlugin moos_plugin;
            goby::core::ProtobufDBOPlugin protobuf_plugin;
            
            
            goby::core::DBOManager* dbo_manager = goby::core::DBOManager::get_instance();
            dbo_manager->plugin_factory().add_plugin(&moos_plugin);
            dbo_manager->plugin_factory().add_plugin(&protobuf_plugin);
            if(cfg_.has_table_prefix())
                dbo_manager->set_table_prefix(cfg_.table_prefix());

            dbo_manager->connect(cfg_.goby_log());


            int moos_raw_id = 0;
            for(int moos_log_index = 0, moos_log_end = cfg_.moos_alog_size();
                moos_log_index < moos_log_end; ++moos_log_index)
            {
                const std::string& moos_alog = cfg_.moos_alog(moos_log_index);
                
                if(cfg_.dump_alog())
                {
                    std::ifstream moos_alog_ifstream(moos_alog.c_str());
                    if(!moos_alog_ifstream.is_open())
                        throw(std::runtime_error("Could not open " + moos_alog + " for reading"));

                    // figure out the file parameters
                    std::string buff;
                    // discard three lines
                    for (int i=0; i<3; ++i)
                        std::getline(moos_alog_ifstream, buff);
            
                    // %% LOGSTART               1222876996.22
                    double alog_start_time;
                    moos_alog_ifstream >> buff >> buff >> alog_start_time;


                    std::string line;

                    while(getline(moos_alog_ifstream,line))
                    {
                        if(!(moos_raw_id % 1000))
                            goby::glog.is(VERBOSE) && goby::glog << "parsing raw id (line# from first alog start) " << moos_raw_id << std::endl;

                        std::vector<std::string> parts;
                        boost::split(parts, line, boost::is_any_of(" "), boost::token_compress_on);

                        if(parts.size() < 4)
                            continue;
                        
                        double time_since_file_start =
                            goby::util::as<double>(boost::trim_copy(parts[0]));
                        std::string key = boost::trim_copy(parts[1]);
                        std::string source = boost::trim_copy(parts[2]);
                        std::string s_value = boost::trim_copy(parts[3]);
                        
                        char data_type;
                        double d_value;
                        try
                        {
                            d_value = boost::lexical_cast<double>(s_value);
                            data_type = MOOS_DOUBLE;
                        }
                        catch(boost::bad_lexical_cast&)
                        {
                            data_type = MOOS_STRING;
                        }
                
                        CMOOSMsg msg = (data_type == MOOS_STRING) ?
                            CMOOSMsg(MOOS_NOTIFY, key, s_value, time_since_file_start + alog_start_time) :
                            CMOOSMsg(MOOS_NOTIFY, key, d_value, time_since_file_start + alog_start_time);
                
                        // no setter in CMOOSMsg
                        msg.m_sSrc = source;
                        
                        moos_plugin.add_message(moos_raw_id++, msg);
                    }
                    dbo_manager->commit();
                }
            }
            
            dbo_manager->commit();
            for(int i = 0, n = cfg_.parse_action_size(); i < n; ++i)
            {
                const AlogToGobyDbConfig::ParseAction& action = cfg_.parse_action(i);

                goby::glog.is(VERBOSE) && goby::glog << "Running parse action: " << action.ShortDebugString() << std::endl;
                const google::protobuf::Descriptor* desc = google::protobuf::DescriptorPool::generated_pool()->FindMessageTypeByName(action.protobuf_type_name());                
                
                if(!desc)
                    throw(std::runtime_error("Unknown type " + action.protobuf_type_name() + " are you sure this is compiled in?"));
                
                typedef Wt::Dbo::collection< Wt::Dbo::ptr<std::pair<int, CMOOSMsg> > > Msgs;
                Msgs msgs;
                
                msgs = dbo_manager->session()->find<std::pair<int, CMOOSMsg> >("where "  + action.sql_where_clause() + " order by moosmsg_time ASC");
                goby::glog.is(VERBOSE) && goby::glog << "We have " << msgs.size() << " messages" << std::endl;

                std::map<int, boost::shared_ptr<google::protobuf::Message> > proto_msgs;
                for (Msgs::const_iterator it = msgs.begin(), n = msgs.end(); it != n; ++it)
                {
                    boost::shared_ptr<google::protobuf::Message> msg = goby::util::DynamicProtobufManager::new_protobuf_message(desc);

                    const CMOOSMsg& moos_msg = (**it).second;
                    goby::glog.is(DEBUG1) && goby::glog << moos_msg << std::endl;

                    if(action.is_key_equals_value_string())
                        goby::moos::MOOSTranslation<goby::moos::protobuf::TranslatorEntry::TECHNIQUE_COMMA_SEPARATED_KEY_EQUALS_VALUE_PAIRS>::parse(boost::to_lower_copy(moos_msg.GetString()), msg.get());
                    else
                        goby::moos::MOOSTranslation<goby::moos::protobuf::TranslatorEntry::TECHNIQUE_FORMAT>::parse(moos_msg.GetString(), msg.get(), action.format());
                    
                    goby::glog.is(DEBUG1) && goby::glog << "[[" << msg->GetDescriptor()->full_name() << "]] " << msg->DebugString() << std::endl;
                    proto_msgs[(**it).first] = msg;
                }
                dbo_manager->commit();

                for(std::map<int, boost::shared_ptr<google::protobuf::Message> >::const_iterator it = proto_msgs.begin(), n = proto_msgs.end(); it != n; ++it)
                {
                    protobuf_plugin.add_message(it->first, it->second);
                    goby::glog.is(DEBUG1) && goby::glog << it->second->ShortDebugString() << std::endl;
                    
                    if(!(i % 100))
                        goby::glog.is(VERBOSE) && goby::glog << "." << std::flush;
                }
                dbo_manager->commit();
            }

            google::protobuf::ShutdownProtobufLibrary();
            
            quit();
            
        }

private:
    void iterate() { assert(false); }
    

    void parse(std::string format,
               std::string str,
               google::protobuf::Message* msg)
        {
            const google::protobuf::Descriptor* desc = msg->GetDescriptor();
            const google::protobuf::Reflection* refl = msg->GetReflection();
            boost::to_lower(format);
            std::string lower_str = boost::to_lower_copy(str);

            goby::glog.is(DEBUG1) && goby::glog << "Format: " << format << std::endl;
            goby::glog.is(DEBUG1) && goby::glog << "String: " << str << std::endl;
            goby::glog.is(DEBUG1) && goby::glog << "Lower String: " << lower_str << std::endl;
            
            std::string::const_iterator i = format.begin();
            while (i != format.end())
            {
                if (*i == '%')
                {
                    ++i; // now *i is the conversion specifier
                    std::string specifier;
                    while(*i != '%')
                        specifier += *i++;                    

                    ++i; // now *i is the next separator
                    std::string extract = str.substr(0, lower_str.find(*i));

                    int field_index;
                    try
                    {
                        field_index = boost::lexical_cast<int>(specifier);

                        const google::protobuf::FieldDescriptor* field_desc = desc->FindFieldByNumber(field_index);

                        if(!field_desc)
                            throw(std::runtime_error("Bad field: " + specifier + " not in message " + desc->full_name()));
                            
                        switch(field_desc->cpp_type())
                        {
                            case google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE:
                                break;
                        
                            case google::protobuf::FieldDescriptor::CPPTYPE_INT32:
                                refl->SetInt32(msg, field_desc, goby::util::as<google::protobuf::int32>(extract));
                                break;
                        
                            case google::protobuf::FieldDescriptor::CPPTYPE_INT64:
                                refl->SetInt64(msg, field_desc, goby::util::as<google::protobuf::int64>(extract));                        
                                break;

                            case google::protobuf::FieldDescriptor::CPPTYPE_UINT32:
                                refl->SetUInt32(msg, field_desc, goby::util::as<google::protobuf::uint32>(extract));
                                break;

                            case google::protobuf::FieldDescriptor::CPPTYPE_UINT64:
                                refl->SetUInt64(msg, field_desc, goby::util::as<google::protobuf::uint64>(extract));
                                break;
                        
                            case google::protobuf::FieldDescriptor::CPPTYPE_BOOL:
                                refl->SetBool(msg, field_desc, goby::util::as<bool>(extract));
                                break;
                    
                            case google::protobuf::FieldDescriptor::CPPTYPE_STRING:
                                refl->SetString(msg, field_desc, extract);
                                break;                    
                
                            case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT:
                                refl->SetFloat(msg, field_desc, goby::util::as<float>(extract));
                                break;
                
                            case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE:
                                refl->SetDouble(msg, field_desc, goby::util::as<double>(extract));
                                break;
                
                            case google::protobuf::FieldDescriptor::CPPTYPE_ENUM:
                            {
                                const google::protobuf::EnumValueDescriptor* enum_desc =
                                    refl->GetEnum(*msg, field_desc)->type()->FindValueByName(extract);

                                // try upper case
                                if(!enum_desc)
                                    enum_desc = refl->GetEnum(*msg, field_desc)->type()->FindValueByName(boost::to_upper_copy(extract));
                                // try lower case
                                if(!enum_desc)
                                    enum_desc = refl->GetEnum(*msg, field_desc)->type()->FindValueByName(boost::to_lower_copy(extract));
                                if(enum_desc)
                                    refl->SetEnum(msg, field_desc, enum_desc);
                            }
                            break;
                        }

                    }
                    catch(boost::bad_lexical_cast&)
                    {
                        throw(std::runtime_error("Bad specifier: " + specifier + ", must be an integer. For message: " +  desc->full_name()));
                    }

                    goby::glog.is(DEBUG1) && goby::glog << "field: [" << field_index << "], extract: [" << extract << "]" << std::endl;
                }
                else
                {
                    // if it's not a %, eat!
                    std::string::size_type pos_to_remove = lower_str.find(*i)+1;
                    lower_str.erase(0, pos_to_remove);
                    str.erase(0, pos_to_remove);
                    ++i;
                }
            }
        }
    
    
private:
    static AlogToGobyDbConfig cfg_;  
};

AlogToGobyDbConfig AlogToGobyDb::cfg_;

int main(int argc, char* argv[])
{
    return goby::run<AlogToGobyDb>(argc, argv);
}
