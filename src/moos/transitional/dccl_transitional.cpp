// copyright 2009 t. schneider tes@mit.edu
// 
// this file is part of the Dynamic Compact Control Language (DCCL),
// the goby-acomms codec. goby-acomms is a collection of libraries 
// for acoustic underwater networking
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

#include <boost/foreach.hpp>
#include <boost/filesystem.hpp>

#include "dccl_transitional.h"
#include "message_xml_callbacks.h"
#include "queue_xml_callbacks.h"
#include "goby/common/logger.h"
#include "goby/util/as.h"
#include <google/protobuf/descriptor.pb.h>
#include "goby/common/protobuf/acomms_option_extensions.pb.h"
#include "goby/util/dynamic_protobuf_manager.h"
#include <boost/regex.hpp>

using goby::util::goby_time;
using goby::util::as;           
using goby::acomms::operator<<;
using namespace goby::common::logger;

#if BOOST_FILESYSTEM_VERSION == 3
namespace bf = boost::filesystem3;
#else
namespace bf = boost::filesystem;
#endif

/////////////////////
// public methods (general use)
/////////////////////
goby::transitional::DCCLTransitionalCodec::DCCLTransitionalCodec()
    : log_(0),
      dccl_(goby::acomms::DCCLCodec::get()),
      start_time_(goby_time())
{
}

void goby::transitional::DCCLTransitionalCodec::convert_to_v2_representation(pAcommsHandlerConfig* cfg)
{
    cfg_ = cfg->transitional_cfg();
    
    for(int i = 0, n = cfg->transitional_cfg().message_file_size(); i < n; ++i)
    {
        convert_xml_message_file(cfg->transitional_cfg().message_file(i).path(),
                                 cfg->add_load_proto_file(),
                                 cfg->mutable_translator_entry());
    }
}


void goby::transitional::DCCLTransitionalCodec::convert_xml_message_file(
    const std::string& xml_file,
    std::string* proto_file,
    google::protobuf::RepeatedPtrField<goby::moos::protobuf::TranslatorEntry>* translator_entries)
{
    size_t begin_size = messages_.size();            
        
    // Register handlers for XML parsing
    DCCLMessageContentHandler message_content(messages_);
    DCCLMessageErrorHandler message_error;
    // instantiate a parser for the xml message files
    XMLParser message_parser(message_content, message_error);

    std::vector<goby::transitional::protobuf::QueueConfig> queue_cfg;
    QueueContentHandler queue_content(queue_cfg);
    QueueErrorHandler queue_error;
    // instantiate a parser for the xml message files
    XMLParser queue_parser(queue_content, queue_error);
    
    message_parser.parse(xml_file);
    queue_parser.parse(xml_file);
    
    size_t end_size = messages_.size();
    
    check_duplicates();

    std::vector<unsigned> added_ids;
    std::set<unsigned> set_added_ids;

    boost::filesystem::path xml_file_path(xml_file);


    std::string generated_proto_dir = cfg_.generated_proto_dir();
    if(!generated_proto_dir.empty() && generated_proto_dir[generated_proto_dir.size() - 1] != '/')
        generated_proto_dir += "/";

#if BOOST_FILESYSTEM_VERSION == 3
    std::string xml_path_stem  = xml_file_path.stem().string();
#else
    std::string xml_path_stem  = xml_file_path.stem();
#endif

    boost::filesystem::path proto_file_path( generated_proto_dir + xml_path_stem + ".proto");
    
    for(size_t i = 0, n = end_size - begin_size; i < n; ++i)
    {
        // map name/id to position in messages_ vector for later use
        size_t new_index = messages_.size()-(n-i);
        name2messages_.insert(std::pair<std::string, size_t>(messages_[new_index].name(), new_index));
        id2messages_.insert(std::pair<unsigned, size_t>(messages_[new_index].id(), new_index));
 
        set_added_ids.insert(messages_[new_index].id());
        added_ids.push_back(messages_[new_index].id());
    }

    *proto_file = bf::complete(proto_file_path).string();

    

    std::ofstream fout(proto_file->c_str(), std::ios::out);
    
    if(!fout.is_open())
        throw(goby::acomms::DCCLException("Could not open " + *proto_file + " for writing"));

    fout << "import \"goby/common/protobuf/option_extensions.proto\";" << std::endl;

    for(int i = 0, n = added_ids.size(); i < n; ++i)
    {
        to_iterator(added_ids[i])->write_schema_to_dccl2(&fout, queue_cfg[i]);
    }
    
    fout.close();
    
    const google::protobuf::FileDescriptor* file_desc =
        goby::util::DynamicProtobufManager::descriptor_pool().FindFileByName(*proto_file);    

    if(file_desc)
    {
        glog.is(DEBUG2) && glog << file_desc->DebugString() << std::flush;

        BOOST_FOREACH(unsigned id, added_ids)
        {
            const google::protobuf::Descriptor* desc = file_desc->FindMessageTypeByName(to_iterator(id)->name());

            if(desc)
                dccl_->validate(desc);
            else
            {
                glog << die << "No descriptor with name " << to_iterator(id)->name() << " found!" << std::endl;
            }

            to_iterator(id)->set_descriptor(desc);
        }
    }
    else
    {
        glog << die << "No file_descriptor with name " << *proto_file << " found!" << std::endl;
    }
    
    

    BOOST_FOREACH(unsigned id, added_ids)
    {
        
        goby::moos::protobuf::TranslatorEntry* entry = translator_entries->Add();
        entry->set_use_short_enum(true);
        std::vector<DCCLMessage>::const_iterator msg_it = to_iterator(id);
        entry->set_protobuf_name(msg_it->name());

        if(msg_it->trigger_type() == "time")
        {
            entry->mutable_trigger()->set_type(goby::moos::protobuf::TranslatorEntry::Trigger::TRIGGER_TIME);
            entry->mutable_trigger()->set_period(msg_it->trigger_time());
        }
        else if(msg_it->trigger_type() == "publish")
        {
            entry->mutable_trigger()->set_type(goby::moos::protobuf::TranslatorEntry::Trigger::TRIGGER_PUBLISH);
            entry->mutable_trigger()->set_moos_var(msg_it->trigger_var());
            if(!msg_it->trigger_mandatory().empty())
                entry->mutable_trigger()->set_mandatory_content(msg_it->trigger_mandatory());
            
        }
        
        std::map<std::string, goby::moos::protobuf::TranslatorEntry::CreateParser*> parser_map;
        std::map<std::string, int > sequence_map;
        int max_sequence = 0;
        BOOST_FOREACH(boost::shared_ptr<DCCLMessageVar> var, msg_it->header_const())
        {
            sequence_map[var->name()] = var->sequence_number();
            max_sequence = (var->sequence_number() > max_sequence) ? var->sequence_number() : max_sequence;
            
            fill_create(var, &parser_map, entry);
        }
        BOOST_FOREACH( boost::shared_ptr<DCCLMessageVar> var, msg_it->layout_const())
        {
            sequence_map[var->name()] = var->sequence_number();
            max_sequence = (var->sequence_number() > max_sequence) ? var->sequence_number() : max_sequence;
            
            fill_create(var, &parser_map, entry);
        }

        max_sequence = ((max_sequence + 100)/100)*100;
        
        for(int i = 0, n = msg_it->publishes_const().size(); i < n; ++i)
        {
            const DCCLPublish& publish = msg_it->publishes_const()[i];    

            goby::moos::protobuf::TranslatorEntry::PublishSerializer* serializer = entry->add_publish();
           serializer->set_technique(goby::moos::protobuf::TranslatorEntry::TECHNIQUE_FORMAT);

           // renumber format numbers
           boost::regex pattern ("%([0-9]+)",boost::regex_constants::icase|boost::regex_constants::perl);
           std::string replace ("%_GOBY1TEMP_\\1_");

           std::string new_moos_var = boost::regex_replace(publish.var(), pattern, replace);


           std::string old_format = publish.format();
           std::string new_format = boost::regex_replace(old_format, pattern, replace);

           
           for(int j = 0, m = publish.message_vars().size(); j < m; ++j)
           {
               int replacement_field = sequence_map[publish.message_vars()[j]->name()];
               
               // add any algorithms
               bool added_algorithms = false;
               BOOST_FOREACH(const std::string& algorithm, publish.algorithms()[j])
               {
                   added_algorithms = true;
                   goby::moos::protobuf::TranslatorEntry::PublishSerializer::Algorithm * new_alg = serializer->add_algorithm();
                   std::vector<std::string> algorithm_parts;
                   boost::split(algorithm_parts, algorithm, boost::is_any_of(":"));
                   
                   new_alg->set_name(algorithm_parts[0]);
                   new_alg->set_primary_field(sequence_map[publish.message_vars()[j]->name()]);
                   
                   for(int k = 1, o = algorithm_parts.size(); k < o; ++k)
                       new_alg->add_reference_field(sequence_map[algorithm_parts[k]]);
                   
                   new_alg->set_output_virtual_field(max_sequence);
               }

               if(added_algorithms)
               {
                   replacement_field = max_sequence;
                   ++max_sequence;
               }
                         
               boost::algorithm::replace_all(new_moos_var, "%_GOBY1TEMP_" + goby::util::as<std::string>(j+1) + "_",
                                             "%" + goby::util::as<std::string>(replacement_field));
               boost::algorithm::replace_all(new_format, "%_GOBY1TEMP_" + goby::util::as<std::string>(j+1) + "_",
                                             "%" + goby::util::as<std::string>(replacement_field));
           
           }
 
           serializer->set_moos_var(new_moos_var);
           serializer->set_format(new_format);
           
    
           
        }    
    }
    
}

void goby::transitional::DCCLTransitionalCodec::fill_create(boost::shared_ptr<DCCLMessageVar> var,
                                                            std::map<std::string, goby::moos::protobuf::TranslatorEntry::CreateParser*>* parser_map,
                                                            goby::moos::protobuf::TranslatorEntry* entry)
{
 
    // entry already exists for this type
    if((*parser_map).count(var->source_var()))
    {
        // use key=value parsing
        (*parser_map)[var->source_var()]->set_technique(goby::moos::protobuf::TranslatorEntry::TECHNIQUE_COMMA_SEPARATED_KEY_EQUALS_VALUE_PAIRS);
        (*parser_map)[var->source_var()]->clear_format();
                
 
                
    }
    else if(!var->source_var().empty())
    {
        (*parser_map)[var->source_var()] = entry->add_create();
        (*parser_map)[var->source_var()]->set_format("%" + goby::util::as<std::string>(var->sequence_number()) + "%");
        (*parser_map)[var->source_var()]->set_technique(goby::moos::protobuf::TranslatorEntry::TECHNIQUE_FORMAT);
        (*parser_map)[var->source_var()]->set_moos_var(var->source_var());
    }

    // add any algorithms
    BOOST_FOREACH(const std::string& algorithm, var->algorithms())
    {
        if(var->source_var().empty())
            throw(std::runtime_error("Algorithms without a corresponding source variable are not permitted in Goby v2 --> v1 backwards compatibility as the MOOS Translator does all algorithm conversions in Goby2"));

        
        goby::moos::protobuf::TranslatorEntry::CreateParser::Algorithm * new_alg =
            (*parser_map)[var->source_var()]->add_algorithm();

        if(algorithm.find(':') != std::string::npos)
        {
            throw(std::runtime_error("Algorithms with reference fields (e.g. subtract:timestamp) are not permitted in Goby v2 --> v1 backwards compatibility. Please redesign the XML message to remove such algorithms. Offending algorithm: " + algorithm + " used in variable: " + var->name()));
        }
        
        
        new_alg->set_name(algorithm);
        new_alg->set_primary_field(var->sequence_number());
    }
            
}



std::set<unsigned> goby::transitional::DCCLTransitionalCodec::all_message_ids()
{
    std::set<unsigned> s;
    BOOST_FOREACH(const DCCLMessage &msg, messages_)
        s.insert(msg.id());
    return s;
}    
std::set<std::string> goby::transitional::DCCLTransitionalCodec::all_message_names()
{
    std::set<std::string> s;
    BOOST_FOREACH(const DCCLMessage &msg, messages_)
        s.insert(msg.name());
    return s;
}

    
std::string goby::transitional::DCCLTransitionalCodec::summary() const	
{ 
    std::string out;
    for(std::vector<DCCLMessage>::const_iterator it = messages_.begin(), n = messages_.end(); it != n; ++it)
        out += it->get_display();
    return out;
}

std::string goby::transitional::DCCLTransitionalCodec::brief_summary() const	
{ 
    std::string out;
    for(std::vector<DCCLMessage>::const_iterator it = messages_.begin(), n = messages_.end(); it != n; ++it)
        out += it->get_short_display();
    return out;
}

// void goby::transitional::DCCLTransitionalCodec::add_algorithm(const std::string& name, AlgFunction1 func)
// {
//     DCCLAlgorithmPerformer* ap = DCCLAlgorithmPerformer::getInstance();
//     ap -> add_algorithm(name, func);
// }

// void goby::transitional::DCCLTransitionalCodec::add_adv_algorithm(const std::string& name, AlgFunction2 func)
// {
//     DCCLAlgorithmPerformer* ap = DCCLAlgorithmPerformer::getInstance();
//     ap -> add_adv_algorithm(name, func);
// }

// void goby::transitional::DCCLTransitionalCodec::add_flex_groups(util::FlexOstream* tout)
// {
//     tout->add_group("dccl_enc", common::Colors::lt_magenta, "encoder messages (goby_dccl)");
//     tout->add_group("dccl_dec", common::Colors::lt_blue, "decoder messages (goby_dccl)");
// }


std::ostream& goby::transitional::operator<< (std::ostream& out, const DCCLTransitionalCodec& d)
{
    out << d.summary();
    return out;
}


std::ostream& goby::transitional::operator<< (std::ostream& out, const std::set<std::string>& s)
{
    out << "std::set<std::string>:" << std::endl;
    for (std::set<std::string>::const_iterator it = s.begin(), n = s.end(); it != n; ++it)
        out << (*it) << std::endl;
    return out;
}

std::ostream& goby::transitional::operator<< (std::ostream& out, const std::set<unsigned>& s)
{
    out << "std::set<unsigned>:" << std::endl;
    for (std::set<unsigned>::const_iterator it = s.begin(), n = s.end(); it != n; ++it)
        out << (*it) << std::endl;
    return out;
}


/////////////////////
// public methods (more MOOS specific, but still could be general use)
/////////////////////

// <trigger_var mandatory_content="string"></trigger_var>
// mandatory content is a string that must be contained in the
// *contents* of the trigger publish in order for the trigger
// to be processed. this allows the SAME moos trigger variable
// to be used to trigger creation of different messages based on the
// contents of the trigger message itself.
bool goby::transitional::DCCLTransitionalCodec::is_publish_trigger(std::set<unsigned>& id, const std::string& key, const std::string& value)
{
    for (std::vector<DCCLMessage>::const_iterator it = messages_.begin(), n = messages_.end(); it != n; ++it)
    {
        if(key == it->trigger_var() && (it->trigger_mandatory() == "" || value.find(it->trigger_mandatory()) != std::string::npos))
            id.insert(it->id());
    }
    return (id.empty()) ? false : true;
}

bool goby::transitional::DCCLTransitionalCodec::is_time_trigger(std::set<unsigned>& id)
{
    using boost::posix_time::seconds;
    
    for (std::vector<DCCLMessage>::iterator it = messages_.begin(), n = messages_.end(); it != n; ++it)
    {
        if(it->trigger_type() == "time" &&
           goby_time() > (start_time_ + seconds(it->trigger_number() * it->trigger_time())))
        {
            id.insert(it->id());
            // increment message counter
            ++(*it);
        }        
    }

    return (id.empty()) ? false : true;
}

    
bool goby::transitional::DCCLTransitionalCodec::is_incoming(unsigned& id, const std::string& key)
{
    for (std::vector<DCCLMessage>::const_iterator it = messages_.begin(), n = messages_.end(); it != n; ++it)
    {
        if (key == it->in_var())
        {
            id = it->id();
            return true;
        }
    }
    return false;
}

/////////////////////
// private methods
/////////////////////


void goby::transitional::DCCLTransitionalCodec::check_duplicates()
{
    std::map<unsigned, std::vector<DCCLMessage>::iterator> all_ids;
    for(std::vector<DCCLMessage>::iterator it = messages_.begin(), n = messages_.end(); it != n; ++it)
    {
        unsigned id = it->id();
            
        std::map<unsigned, std::vector<DCCLMessage>::iterator>::const_iterator id_it = all_ids.find(id);
        if(id_it != all_ids.end())
            throw goby::acomms::DCCLException(std::string("DCCL: duplicate variable id " + as<std::string>(id) + " specified for " + it->name() + " and " + id_it->second->name()));
            
        all_ids.insert(std::pair<unsigned, std::vector<DCCLMessage>::iterator>(id, it));
    }
}

std::vector<goby::transitional::DCCLMessage>::const_iterator goby::transitional::DCCLTransitionalCodec::to_iterator(const std::string& message_name) const
{
    if(name2messages_.count(message_name))
        return messages_.begin() + name2messages_.find(message_name)->second;
    else
        throw goby::acomms::DCCLException(std::string("DCCL: attempted an operation on message [" + message_name + "] which is not loaded"));
}
std::vector<goby::transitional::DCCLMessage>::iterator goby::transitional::DCCLTransitionalCodec::to_iterator(const std::string& message_name)
{
    if(name2messages_.count(message_name))
        return messages_.begin() + name2messages_.find(message_name)->second;
    else
        throw goby::acomms::DCCLException(std::string("DCCL: attempted an operation on message [" + message_name + "] which is not loaded"));
}
std::vector<goby::transitional::DCCLMessage>::const_iterator goby::transitional::DCCLTransitionalCodec::to_iterator(const unsigned& id) const
{
    if(id2messages_.count(id))
        return messages_.begin() + id2messages_.find(id)->second;
    else
        throw goby::acomms::DCCLException(std::string("DCCL: attempted an operation on message [" + as<std::string>(id) + "] which is not loaded"));
}

std::vector<goby::transitional::DCCLMessage>::iterator goby::transitional::DCCLTransitionalCodec::to_iterator(const unsigned& id)
{
    if(id2messages_.count(id))
        return messages_.begin() + id2messages_.find(id)->second;
    else
        throw goby::acomms::DCCLException(std::string("DCCL: attempted an operation on message [" + as<std::string>(id) + "] which is not loaded"));
}


void goby::transitional::DCCLTransitionalCodec::encode_private(std::vector<DCCLMessage>::iterator it,
                                                               boost::shared_ptr<google::protobuf::Message>& proto_msg,
                                                               std::map<std::string, std::vector<DCCLMessageVal> > in /* copy */)
{
    if(manip_manager_.has(it->id(), protobuf::MessageFile::NO_ENCODE))
    {
        if(log_) *log_ << group("dccl_enc") << "not encoding DCCL ID: " << it->id() << "; NO_ENCODE manipulator is set" << std::endl;
        throw(goby::acomms::DCCLException("NO_ENCODE manipulator set"));
    }
    
    if(log_)
    {
        *log_ << group("dccl_enc") << "starting encode for " << it->name() << std::endl;        

        typedef std::pair<std::string, std::vector<DCCLMessageVal> > P;
        BOOST_FOREACH(const P& p, in)                    
        {
            if(!p.first.empty())
            {
                BOOST_FOREACH(const DCCLMessageVal& mv, p.second)
                    *log_ << group("dccl_enc") << "\t" << p.first << ": "<< mv << std::endl;
            }
        }
    }
    
    std::string body, head;
    
    it->set_head_defaults(in, cfg_.modem_id());

    proto_msg = goby::util::DynamicProtobufManager::new_protobuf_message(it->descriptor());

    std::map<std::string, std::vector<DCCLMessageVal> > out_vals = in;
    
    it->pre_encode(in, out_vals);
    
    if(log_)
        *log_ << group("dccl_enc") << "MessageVals after pre-encode: " << out_vals << std::endl;


    
    const google::protobuf::Descriptor* desc = proto_msg->GetDescriptor();
    const google::protobuf::Reflection* refl = proto_msg->GetReflection();
    
    for(int i = 0, n = desc->field_count(); i < n; ++i)
    {
        const google::protobuf::FieldDescriptor* field_desc = desc->field(i);
        const std::string& field_name = field_desc->name();
        
        if(field_desc->is_repeated())
        {
            BOOST_FOREACH(const DCCLMessageVal& val, out_vals[field_name])
            {
                if(val.empty())
                    continue;
                
                switch(field_desc->cpp_type())
                {
                    default:
                        break;    
                    
                    case google::protobuf::FieldDescriptor::CPPTYPE_INT32:
                        refl->AddInt32(proto_msg.get(), field_desc, long(val));
                        break;
                        
                    case google::protobuf::FieldDescriptor::CPPTYPE_INT64:
                        refl->AddInt64(proto_msg.get(), field_desc, long(val));
                        break;

                    case google::protobuf::FieldDescriptor::CPPTYPE_UINT32:
                        refl->AddUInt32(proto_msg.get(), field_desc, long(val));
                        break;

                    case google::protobuf::FieldDescriptor::CPPTYPE_UINT64:
                        refl->AddUInt64(proto_msg.get(), field_desc, long(val));
                        break;
                        
                    case google::protobuf::FieldDescriptor::CPPTYPE_BOOL:
                        refl->AddBool(proto_msg.get(), field_desc, bool(val));
                        break;
                    
                    case google::protobuf::FieldDescriptor::CPPTYPE_STRING:
                        refl->AddString(proto_msg.get(), field_desc, std::string(val));
                        break;                    
                
                    case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT:
                        refl->AddFloat(proto_msg.get(), field_desc, double(val));
                        break;
                
                    case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE:
                        refl->AddDouble(proto_msg.get(), field_desc, double(val));
                        break;
                
                    case google::protobuf::FieldDescriptor::CPPTYPE_ENUM:
                    {
                        std::string enum_val_name = boost::to_upper_copy(field_name);
                        enum_val_name += "_" + std::string(val);
                        
                        const google::protobuf::EnumValueDescriptor* enum_desc =
                            field_desc->enum_type()->FindValueByName(enum_val_name);
                        if(!enum_desc)
                            glog.is(WARN) && glog << "invalid enumeration " << enum_val_name <<  " for field " << field_name << std::endl;
                        else
                            refl->AddEnum(proto_msg.get(), field_desc, enum_desc);
                        
                    }
                    break;
                }
                
            }
        }
        else
        {
            const DCCLMessageVal& val = out_vals[field_name].at(0);

            if(!val.empty())
            {
                switch(field_desc->cpp_type())
                {
                    default:
                        break;    
         
                    case google::protobuf::FieldDescriptor::CPPTYPE_INT32:
                        refl->SetInt32(proto_msg.get(), field_desc, long(val));
                        break;
                        
                    case google::protobuf::FieldDescriptor::CPPTYPE_INT64:
                        refl->SetInt64(proto_msg.get(), field_desc, long(val));
                        break;

                    case google::protobuf::FieldDescriptor::CPPTYPE_UINT32:
                        refl->SetUInt32(proto_msg.get(), field_desc, long(val));
                        break;

                    case google::protobuf::FieldDescriptor::CPPTYPE_UINT64:
                        refl->SetUInt64(proto_msg.get(), field_desc, long(val));
                        break;
                        
                    case google::protobuf::FieldDescriptor::CPPTYPE_BOOL:
                        refl->SetBool(proto_msg.get(), field_desc, bool(val));
                        break;
                    
                    case google::protobuf::FieldDescriptor::CPPTYPE_STRING:
                        refl->SetString(proto_msg.get(), field_desc, std::string(val));
                        break;                    
                
                    case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT:
                        refl->SetFloat(proto_msg.get(), field_desc, double(val));
                        break;
                
                    case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE:
                        refl->SetDouble(proto_msg.get(), field_desc, double(val));
                        break;
                        
                    case google::protobuf::FieldDescriptor::CPPTYPE_ENUM:
                    {
                        std::string enum_val_name = boost::to_upper_copy(field_name);
                        enum_val_name += "_" + std::string(val);

                        const google::protobuf::EnumValueDescriptor* enum_desc =
                            field_desc->enum_type()->FindValueByName(enum_val_name);
                        if(!enum_desc)
                            glog.is(WARN) && glog << "invalid enumeration " << enum_val_name <<  " for field " << field_name << std::endl;
                        else
                            refl->SetEnum(proto_msg.get(), field_desc, enum_desc);
                        
                    }
                    break;
                }
            }    
        }
    }
    
    
    
    if(log_)
        *log_ << group("dccl_enc") << "Protobuf message after translation: " << proto_msg->DebugString() << std::endl;
    
    if(log_) *log_ << group("dccl_enc") << "finished encode of " << it->name() <<  std::endl;    
}

std::vector<goby::transitional::DCCLMessage>::iterator goby::transitional::DCCLTransitionalCodec::decode_private(const google::protobuf::Message& proto_msg, std::map<std::string, std::vector<DCCLMessageVal> >& out)
{
    std::vector<DCCLMessage>::iterator it = to_iterator(dccl_->id(proto_msg.GetDescriptor()));

    if(manip_manager_.has(it->id(), protobuf::MessageFile::NO_DECODE))
    {
        if(log_) *log_ << group("dccl_enc") << "not decoding DCCL ID: " << it->id() << "; NO_DECODE manipulator is set" << std::endl;
        throw(goby::acomms::DCCLException("NO_DECODE manipulator set"));
    }


    if(log_) *log_ << group("dccl_dec") << "starting decode for " << it->name() << std::endl;        

    if(log_)
        *log_ << group("dccl_enc") << "Protobuf message after decoding: " << proto_msg.DebugString() << std::endl; // 

    std::map<std::string, std::vector<DCCLMessageVal> > raw_out;

    it->set_head_defaults(raw_out, cfg_.modem_id());


    const google::protobuf::Descriptor* desc = proto_msg.GetDescriptor();
    const google::protobuf::Reflection* refl = proto_msg.GetReflection();
    
    for(int i = 0, n = desc->field_count(); i < n; ++i)
    {
        const google::protobuf::FieldDescriptor* field_desc = desc->field(i);
        const std::string& field_name = field_desc->name();
        
        if(field_desc->is_repeated())
        {
            for(int j = 0, m = refl->FieldSize(proto_msg, field_desc); j < m; ++j)
            {                
                switch(field_desc->cpp_type())
                {
                    default:
                        break;    
                    
                    case google::protobuf::FieldDescriptor::CPPTYPE_INT32:
                        raw_out[field_name].push_back(long(refl->GetRepeatedInt32(proto_msg, field_desc, j)));
                        break;
                        
                    case google::protobuf::FieldDescriptor::CPPTYPE_INT64:
                        raw_out[field_name].push_back(long(refl->GetRepeatedInt64(proto_msg, field_desc, j)));
                        break;

                    case google::protobuf::FieldDescriptor::CPPTYPE_UINT32:
                        raw_out[field_name].push_back(long(refl->GetRepeatedUInt32(proto_msg, field_desc, j)));
                        break;

                    case google::protobuf::FieldDescriptor::CPPTYPE_UINT64:
                        raw_out[field_name].push_back(long(refl->GetRepeatedUInt64(proto_msg, field_desc, j)));
                        break;
                        
                    case google::protobuf::FieldDescriptor::CPPTYPE_BOOL:
                        raw_out[field_name].push_back(bool(refl->GetRepeatedBool(proto_msg, field_desc, j)));
                        break;
                    
                    case google::protobuf::FieldDescriptor::CPPTYPE_STRING:
                        raw_out[field_name].push_back(std::string(refl->GetRepeatedString(proto_msg, field_desc, j)));
                        break;                    
                
                    case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT:
                        raw_out[field_name].push_back(double(refl->GetRepeatedFloat(proto_msg, field_desc, j)));
                        break;
                
                    case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE:
                        raw_out[field_name].push_back(double(refl->GetRepeatedDouble(proto_msg, field_desc, j)));
                        break;
                
                    case google::protobuf::FieldDescriptor::CPPTYPE_ENUM:
                    {
                        std::string enum_val_name = refl->GetRepeatedEnum(proto_msg, field_desc, j)->name();
                        raw_out[field_name].push_back(enum_val_name.substr(enum_val_name.find("_") + 1));
                    }
                    break;
                }
                
            }
        }
        else
        {
            if(refl->HasField(proto_msg, field_desc))
            {
                switch(field_desc->cpp_type())
                {
                    default:
                        break;
                        
                    case google::protobuf::FieldDescriptor::CPPTYPE_INT32:
                        raw_out[field_name].assign(1, long(refl->GetInt32(proto_msg, field_desc)));
                        break;
                        
                    case google::protobuf::FieldDescriptor::CPPTYPE_INT64:
                        raw_out[field_name].assign(1, long(refl->GetInt64(proto_msg, field_desc)));
                        break;

                    case google::protobuf::FieldDescriptor::CPPTYPE_UINT32:
                        raw_out[field_name].assign(1, long(refl->GetUInt32(proto_msg, field_desc)));
                        break;

                    case google::protobuf::FieldDescriptor::CPPTYPE_UINT64:
                        raw_out[field_name].assign(1, long(refl->GetUInt64(proto_msg, field_desc)));
                        break;
                        
                    case google::protobuf::FieldDescriptor::CPPTYPE_BOOL:
                        raw_out[field_name].assign(1, bool(refl->GetBool(proto_msg, field_desc)));
                        break;
                    
                    case google::protobuf::FieldDescriptor::CPPTYPE_STRING:
                        raw_out[field_name].assign(1, std::string(refl->GetString(proto_msg, field_desc)));
                        break;                    
                
                    case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT:
                        raw_out[field_name].assign(1, double(refl->GetFloat(proto_msg, field_desc)));
                        break;
                
                    case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE:
                        raw_out[field_name].assign(1, double(refl->GetDouble(proto_msg, field_desc)));
                        break;
                
                    case google::protobuf::FieldDescriptor::CPPTYPE_ENUM:
                        std::string enum_val_name = refl->GetEnum(proto_msg, field_desc)->name();
                        raw_out[field_name].assign(1, enum_val_name.substr(enum_val_name.find("_") + 1));
                        break;
                }
            }    
        }
    }

    if(log_)
        *log_ << group("dccl_enc") << "MessageVals before post-decode: " << raw_out << std::endl;


    out = raw_out;

    it->post_decode(raw_out, out);
    
    if(log_)
    {
        typedef std::pair<std::string, std::vector<DCCLMessageVal> > P;
        BOOST_FOREACH(const P& p, out)                    
        {
            if(!p.first.empty())
            {
                BOOST_FOREACH(const DCCLMessageVal& mv, p.second)
                    *log_ << group("dccl_dec") << "\t" << p.first << ": "<< mv << std::endl;
            }
        }
        *log_ << group("dccl_dec") << "finished decode of "<< it->name() << std::endl;
    }

    return it;
}




// void goby::transitional::DCCLTransitionalCodec::merge_cfg(const protobuf::DCCLTransitionalConfig& cfg)
// {
//     cfg_.MergeFrom(cfg);
//     process_cfg();
// }

// void goby::transitional::DCCLTransitionalCodec::set_cfg(const protobuf::DCCLTransitionalConfig& cfg)
// {
//     cfg_.CopyFrom(cfg);
//     process_cfg();
// }

// void goby::transitional::DCCLTransitionalCodec::process_cfg()
// {
//     messages_.clear();
//     name2messages_.clear();
//     id2messages_.clear();
//     manip_manager_.clear();

//     for(int i = 0, n = cfg_.message_file_size(); i < n; ++i)
//     {
//         std::set<unsigned> new_ids = add_xml_message_file(cfg_.message_file(i).path());
//         BOOST_FOREACH(unsigned new_id, new_ids)
//         {
//             for(int j = 0, o = cfg_.message_file(i).manipulator_size(); j < o; ++j)
//                 manip_manager_.add(new_id, cfg_.message_file(i).manipulator(j));
//         }
//     }
    
// }

