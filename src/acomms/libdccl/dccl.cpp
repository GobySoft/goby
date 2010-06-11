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
#include <crypto++/filters.h>
#include <crypto++/sha.h>
#include <crypto++/modes.h>
#include <crypto++/aes.h>

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
        out += it->get_display();
    return out;
}

std::string dccl::DCCLCodec::brief_summary() const	
{ 
    std::string out;
    for(std::vector<Message>::const_iterator it = messages_.begin(), n = messages_.end(); it != n; ++it)
        out += it->get_short_display();
    return out;
}

void dccl::DCCLCodec::add_algorithm(const std::string& name, AlgFunction1 func)
{
    AlgorithmPerformer* ap = AlgorithmPerformer::getInstance();
    ap -> add_algorithm(name, func);
}

void dccl::DCCLCodec::add_adv_algorithm(const std::string& name, AlgFunction3 func)
{
    AlgorithmPerformer* ap = AlgorithmPerformer::getInstance();
    ap -> add_adv_algorithm(name, func);
}


std::ostream& dccl::operator<< (std::ostream& out, const DCCLCodec& d)
{
    out << d.summary();
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

std::vector<dccl::Message>::const_iterator dccl::DCCLCodec::to_iterator(const std::string& message_name) const
{
    if(name2messages_.count(message_name))
        return messages_.begin() + name2messages_.find(message_name)->second;
    else
        throw std::runtime_error(std::string("DCCL: attempted an operation on message [" + message_name + "] which is not loaded"));
}
std::vector<dccl::Message>::iterator dccl::DCCLCodec::to_iterator(const std::string& message_name)
{
    if(name2messages_.count(message_name))
        return messages_.begin() + name2messages_.find(message_name)->second;
    else
        throw std::runtime_error(std::string("DCCL: attempted an operation on message [" + message_name + "] which is not loaded"));
}
std::vector<dccl::Message>::const_iterator dccl::DCCLCodec::to_iterator(const unsigned& id) const
{
    if(id2messages_.count(id))
        return messages_.begin() + id2messages_.find(id)->second;
    else
        throw std::runtime_error(std::string("DCCL: attempted an operation on message [" + boost::lexical_cast<std::string>(id) + "] which is not loaded"));
}

std::vector<dccl::Message>::iterator dccl::DCCLCodec::to_iterator(const unsigned& id)
{
    if(id2messages_.count(id))
        return messages_.begin() + id2messages_.find(id)->second;
    else
        throw std::runtime_error(std::string("DCCL: attempted an operation on message [" + boost::lexical_cast<std::string>(id) + "] which is not loaded"));
}


void dccl::DCCLCodec::encode_private(std::vector<Message>::iterator it,
                                     std::string& out,
                                     std::map<std::string, MessageVal> in)
{
    // 1. encode parts
    std::string body, head;
    it->head_encode(head, in);
    it->body_encode(body, in);
    
    // 2. encrypt
    if(!crypto_key_.empty()) encrypt(body, head);
 
    // 3. join head and body
    out = head + body;

    // 4. hex encode
    hex_encode(out);

    // increment message counter
    ++(*it);
}
        
void dccl::DCCLCodec::decode_private(std::vector<Message>::iterator it,
                                     std::string in,
                                     std::map<std::string, MessageVal>& out)
{
    // 4. hex decode
    hex_decode(in);

    // clean up any ending junk added by modem
    in.resize(in.find_last_not_of(char(0))+1);
    
    // 3. split body and header (avoid substr::out_of_range)
    std::string body = (acomms::NUM_HEADER_BYTES < in.size()) ?
        in.substr(acomms::NUM_HEADER_BYTES) : "";
    std::string head = in.substr(0, acomms::NUM_HEADER_BYTES);
    
    // 2. decrypt
    if(!crypto_key_.empty()) decrypt(body, head);


    
    // 1. decode parts
    it->head_decode(head, out);
    it->body_decode(body, out);

}
        
void dccl::DCCLCodec::encode_private(std::vector<Message>::iterator it,
                                     modem::Message& out_msg,
                                     const std::map<std::string, MessageVal>& in)
{
    std::string out;
    encode_private(it, out, in);

    dccl::DCCLHeaderDecoder head_dec(out);

    out_msg.set_data(out);
    
    MessageVal& t = head_dec[acomms::head_time];
    MessageVal& src = head_dec[acomms::head_src_id];
    MessageVal& dest = head_dec[acomms::head_dest_id];

    out_msg.set_t(long(t));
    out_msg.set_src(long(src));
    out_msg.set_dest(long(dest));
}

void dccl::DCCLCodec::decode_private(std::vector<Message>::iterator it,
                                     const modem::Message& in_msg,
                                     std::map<std::string, MessageVal>& out)
{ decode_private(it, in_msg.data(), out); }


void dccl::DCCLCodec::set_crypto_passphrase(const std::string& s)
{
    using namespace CryptoPP;

    SHA256 hash;
    StringSource unused (s, true, new HashFilter(hash, new StringSink(crypto_key_)));
}


void dccl::DCCLCodec::encrypt(std::string& s, const std::string& nonce)
{
    using namespace CryptoPP;

    std::string iv;
    SHA256 hash;
    StringSource unused(nonce, true, new HashFilter(hash, new StringSink(iv)));
    
    CTR_Mode<AES>::Encryption encryptor;
    encryptor.SetKeyWithIV((byte*)crypto_key_.c_str(), crypto_key_.size(), (byte*)iv.c_str());

    std::string cipher;
    StreamTransformationFilter in(encryptor, new StringSink(cipher));
    in.Put((byte*)s.c_str(), s.size());
    in.MessageEnd();
    s = cipher;
}

void dccl::DCCLCodec::decrypt(std::string& s, const std::string& nonce)
{
    using namespace CryptoPP;

    std::string iv;
    SHA256 hash;
    StringSource unused(nonce, true, new HashFilter(hash, new StringSink(iv)));
    
    CTR_Mode<AES>::Decryption decryptor;    
    decryptor.SetKeyWithIV((byte*)crypto_key_.c_str(), crypto_key_.size(), (byte*)iv.c_str());
    
    std::string recovered;
    StreamTransformationFilter out(decryptor, new StringSink(recovered));
    out.Put((byte*)s.c_str(), s.size());
    out.MessageEnd();
    s = recovered;
}
