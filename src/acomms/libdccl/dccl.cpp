// copyright 2009 t. schneider tes@mit.edu
// ocean engineering graudate student - mit / whoi joint program
// massachusetts institute of technology (mit)
// laboratory for autonomous marine sensing systems (lamss)
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

// see dccl.h for comments on individual methods
#include <boost/foreach.hpp>

#include "dccl.h"
#include "message_xml_callbacks.h"

/////////////////////
// public methods (general use)
/////////////////////
dccl::DCCLCodec::DCCLCodec() : start_time_(time(NULL))
{ }
    
dccl::DCCLCodec::DCCLCodec(const std::string& file, const std::string schema) : start_time_(time(NULL))
{ add_xml_message_file(file, schema); }
    
dccl::DCCLCodec::DCCLCodec(const std::set<std::string>& files,
                           const std::string schema) : start_time_(time(NULL))
{
    BOOST_FOREACH(const std::string& s, files)
        add_xml_message_file(s, schema);
}

std::set<unsigned> dccl::DCCLCodec::add_xml_message_file(const std::string& xml_file,
                                               const std::string xml_schema)
{
    size_t begin_size = messages_.size();
            
        
    // Register handlers for XML parsing
    MessageContentHandler content(messages_);
    MessageErrorHandler error;
    // instantiate a parser for the xml message files
    XMLParser parser(content, error);
    // parse(file, [schema])
    if(xml_schema != "")
        xml_schema_ = xml_schema;
        
    parser.parse(xml_file, xml_schema_);

    size_t end_size = messages_.size();
    
    check_duplicates();

    std::set<unsigned> added_ids;
    
    for(size_t i = 0, n = end_size - begin_size; i < n; ++i)
    {
        // map name/id to position in messages_ vector for later use
        size_t new_index = messages_.size()-i-1;
        name2messages_.insert(std::pair<std::string, size_t>(messages_[new_index].name(), new_index));
        id2messages_.insert(std::pair<unsigned, size_t>(messages_[new_index].id(), new_index));
        added_ids.insert(messages_[new_index].id());
    }
    
    return added_ids;
}

std::set<unsigned> dccl::DCCLCodec::all_message_ids()
{
    std::set<unsigned> s;
    BOOST_FOREACH(const Message &msg, messages_)
        s.insert(msg.id());
    return s;
}    
std::set<std::string> dccl::DCCLCodec::all_message_names()
{
    std::set<std::string> s;
    BOOST_FOREACH(const Message &msg, messages_)
        s.insert(msg.name());
    return s;
}

    
std::string dccl::DCCLCodec::summary() const	
{ 
    std::string out;
    for(std::vector<Message>::const_iterator it = messages_.begin(), n = messages_.end(); it != n; ++it)
        out += summary(it);
    return out;
}

std::string dccl::DCCLCodec::brief_summary() const	
{ 
    std::string out;
    for(std::vector<Message>::const_iterator it = messages_.begin(), n = messages_.end(); it != n; ++it)
        out += brief_summary(it);
    return out;
}

    
std::ostream& dccl::operator<< (std::ostream& out, const DCCLCodec& d)
{
    out << d.summary();
    return out;
}

std::ostream& dccl::operator<< (std::ostream& out, const std::map<std::string, double>& m)
{
    for (std::map<std::string, double>::const_iterator it = m.begin(), n = m.end(); it != n; ++it)
        out << "\t" << "key: " << it->first << std::endl
            << "\t" << "value (double): " << it->second << std::endl;    
    return out;
}

std::ostream& dccl::operator<< (std::ostream& out, const std::map<std::string, std::string>& m)
{
    for (std::map<std::string, std::string>::const_iterator it = m.begin(), n = m.end(); it != n; ++it)
        out << "\t" << "key: " << it->first << std::endl
            << "\t" << "value (string): \"" << it->second << "\"" << std::endl;
    return out;
}


std::ostream& dccl::operator<< (std::ostream& out, const std::map<std::string, long>& m)
{
    for (std::map<std::string, long>::const_iterator it = m.begin(), n = m.end(); it != n; ++it)
        out << "\t" << "key: " << it->first << std::endl
            << "\t" << "value (long): " << it->second << std::endl;    
    return out;
}

std::ostream& dccl::operator<< (std::ostream& out, const std::map<std::string, bool>& m)
{
    for (std::map<std::string, bool>::const_iterator it = m.begin(), n = m.end(); it != n; ++it)
        out << "\t" << "key: " << it->first << std::endl
            << "\t" << "value (bool): " << std::boolalpha <<  it->second << std::endl;    
    return out;
}


std::ostream& dccl::operator<< (std::ostream& out, const std::set<std::string>& s)
{
    out << "std::set<std::string>:" << std::endl;
    for (std::set<std::string>::const_iterator it = s.begin(), n = s.end(); it != n; ++it)
        out << (*it) << std::endl;
    return out;
}

std::ostream& dccl::operator<< (std::ostream& out, const std::set<unsigned>& s)
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
bool dccl::DCCLCodec::is_publish_trigger(std::set<unsigned>& id, const std::string& key, const std::string& value)
{
    for (std::vector<dccl::Message>::const_iterator it = messages_.begin(), n = messages_.end(); it != n; ++it)
    {
        if(key == it->trigger_var() && (it->trigger_mandatory() == "" || value.find(it->trigger_mandatory()) != std::string::npos))
            id.insert(it->id());
    }
    return (id.empty()) ? false : true;
}

bool dccl::DCCLCodec::is_time_trigger(std::set<unsigned>& id)
{
    for (std::vector<dccl::Message>::const_iterator it = messages_.begin(), n = messages_.end(); it != n; ++it)
    {
        if(it->trigger_type() == "time" && time(NULL) > (start_time_ + it->trigger_number() * it->trigger_time())) 
            id.insert(it->id());
    }
    return (id.empty()) ? false : true;
}

    
bool dccl::DCCLCodec::is_incoming(unsigned& id, const std::string& key)
{
    for (std::vector<dccl::Message>::const_iterator it = messages_.begin(), n = messages_.end(); it != n; ++it)
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
                                
std::set<std::string> dccl::DCCLCodec::encode_vars(std::vector<Message>::iterator it)
{
    if(it != messages_.end())
        return it->encode_vars();
    else
        throw std::runtime_error(std::string("DCCL: queried `encode_vars` on message which is not loaded"));
}	


std::set<std::string> dccl::DCCLCodec::decode_vars(std::vector<Message>::iterator it)
{
    if(it != messages_.end())
        return it->decode_vars();
    else
        throw std::runtime_error(std::string("DCCL: queried `decode_vars` on message which is not loaded"));
}
    
std::set<std::string> dccl::DCCLCodec::src_vars(std::vector<Message>::iterator it)
{
    if(it != messages_.end())
        return it->src_vars();
    else
        throw std::runtime_error(std::string("DCCL: queried `src_vars` on message which is not loaded"));
}	

std::set<std::string> dccl::DCCLCodec::all_vars(std::vector<Message>::iterator it)
{
    if(it != messages_.end())
        return it->all_vars();
    else
        throw std::runtime_error(std::string("DCCL: queried `all_vars` on message which is not loaded"));
}	

    
std::string dccl::DCCLCodec::input_summary(std::vector<Message>::iterator it)
{
    if(it != messages_.end())
        return it->input_summary();
    else
        throw std::runtime_error(std::string("DCCL: queried `input_summary` on message which is not loaded"));
}

std::string dccl::DCCLCodec::summary(const std::vector<Message>::const_iterator it) const 
{
    if(it != messages_.end())
        return it->get_display();        
    else
        return "invalid message!";
}

std::string dccl::DCCLCodec::brief_summary(const std::vector<Message>::const_iterator it) const 
{
    if(it != messages_.end())
        return it->get_short_display();        
    else
        return "invalid message!";
}

std::map<std::string, std::string> dccl::DCCLCodec::message_var_names(const std::vector<Message>::const_iterator it) const 
{
    if(it != messages_.end())
    {
        return it->message_var_names();
    }
    else
        throw std::runtime_error(std::string("DCCL: queried `message_var_names` on message which is not loaded"));
}


void dccl::DCCLCodec::check_duplicates()
{
    std::map<unsigned, std::vector<Message>::iterator> all_ids;
    for(std::vector<Message>::iterator it = messages_.begin(), n = messages_.end(); it != n; ++it)
    {
        unsigned id = it->id();
            
        std::map<unsigned, std::vector<Message>::iterator>::const_iterator id_it = all_ids.find(id);
        if(id_it != all_ids.end())
            throw std::runtime_error(std::string("DCCL: duplicate variable id " + boost::lexical_cast<std::string>(id) + " specified for " + it->name() + " and " + id_it->second->name()));
            
        all_ids.insert(std::pair<unsigned, std::vector<Message>::iterator>(id, it));
    }
}
