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

#ifdef CRYPTOPP_PATH_USES_PLUS_SIGN
#include <crypto++/filters.h>
#include <crypto++/sha.h>
#include <crypto++/modes.h>
#include <crypto++/aes.h>
#else
#include <cryptopp/filters.h>
#include <cryptopp/sha.h>
#include <cryptopp/modes.h>
#include <cryptopp/aes.h>
#endif

#include "dccl.h"
#include "goby/acomms/xml/xml_parser.h"
#include "message_xml_callbacks.h"
#include "goby/util/logger.h"
#include "goby/util/string.h"
#include "goby/protobuf/acomms_proto_helpers.h"

using goby::util::goby_time;
using goby::util::as;

/////////////////////
// public methods (general use)
/////////////////////
goby::acomms::DCCLCodec::DCCLCodec(std::ostream* log /* =0 */)
    : log_(log),
      start_time_(goby_time())
{ }

std::set<unsigned> goby::acomms::DCCLCodec::add_xml_message_file(const std::string& xml_file)
{
    size_t begin_size = messages_.size();
            
        
    // Register handlers for XML parsing
    DCCLMessageContentHandler content(messages_);
    DCCLMessageErrorHandler error;
    // instantiate a parser for the xml message files
    XMLParser parser(content, error);
    // parse(file)

    parser.parse(xml_file);

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

std::set<unsigned> goby::acomms::DCCLCodec::all_message_ids()
{
    std::set<unsigned> s;
    BOOST_FOREACH(const DCCLMessage &msg, messages_)
        s.insert(msg.id());
    return s;
}    
std::set<std::string> goby::acomms::DCCLCodec::all_message_names()
{
    std::set<std::string> s;
    BOOST_FOREACH(const DCCLMessage &msg, messages_)
        s.insert(msg.name());
    return s;
}

    
std::string goby::acomms::DCCLCodec::summary() const	
{ 
    std::string out;
    for(std::vector<DCCLMessage>::const_iterator it = messages_.begin(), n = messages_.end(); it != n; ++it)
        out += it->get_display();
    return out;
}

std::string goby::acomms::DCCLCodec::brief_summary() const	
{ 
    std::string out;
    for(std::vector<DCCLMessage>::const_iterator it = messages_.begin(), n = messages_.end(); it != n; ++it)
        out += it->get_short_display();
    return out;
}

void goby::acomms::DCCLCodec::add_algorithm(const std::string& name, AlgFunction1 func)
{
    DCCLAlgorithmPerformer* ap = DCCLAlgorithmPerformer::getInstance();
    ap -> add_algorithm(name, func);
}

void goby::acomms::DCCLCodec::add_adv_algorithm(const std::string& name, AlgFunction2 func)
{
    DCCLAlgorithmPerformer* ap = DCCLAlgorithmPerformer::getInstance();
    ap -> add_algorithm(name, func);
}

void goby::acomms::DCCLCodec::add_flex_groups(util::FlexOstream* tout)
{
    tout->add_group("dccl_enc", util::Colors::lt_magenta, "encoder messages (goby_dccl)");
    tout->add_group("dccl_dec", util::Colors::lt_blue, "decoder messages (goby_dccl)");
}


std::ostream& goby::acomms::operator<< (std::ostream& out, const DCCLCodec& d)
{
    out << d.summary();
    return out;
}


std::ostream& goby::acomms::operator<< (std::ostream& out, const std::set<std::string>& s)
{
    out << "std::set<std::string>:" << std::endl;
    for (std::set<std::string>::const_iterator it = s.begin(), n = s.end(); it != n; ++it)
        out << (*it) << std::endl;
    return out;
}

std::ostream& goby::acomms::operator<< (std::ostream& out, const std::set<unsigned>& s)
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
bool goby::acomms::DCCLCodec::is_publish_trigger(std::set<unsigned>& id, const std::string& key, const std::string& value)
{
    for (std::vector<DCCLMessage>::const_iterator it = messages_.begin(), n = messages_.end(); it != n; ++it)
    {
        if(key == it->trigger_var() && (it->trigger_mandatory() == "" || value.find(it->trigger_mandatory()) != std::string::npos))
            id.insert(it->id());
    }
    return (id.empty()) ? false : true;
}

bool goby::acomms::DCCLCodec::is_time_trigger(std::set<unsigned>& id)
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

    
bool goby::acomms::DCCLCodec::is_incoming(unsigned& id, const std::string& key)
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


void goby::acomms::DCCLCodec::check_duplicates()
{
    std::map<unsigned, std::vector<DCCLMessage>::iterator> all_ids;
    for(std::vector<DCCLMessage>::iterator it = messages_.begin(), n = messages_.end(); it != n; ++it)
    {
        unsigned id = it->id();
            
        std::map<unsigned, std::vector<DCCLMessage>::iterator>::const_iterator id_it = all_ids.find(id);
        if(id_it != all_ids.end())
            throw DCCLException(std::string("DCCL: duplicate variable id " + as<std::string>(id) + " specified for " + it->name() + " and " + id_it->second->name()));
            
        all_ids.insert(std::pair<unsigned, std::vector<DCCLMessage>::iterator>(id, it));
    }
}

std::vector<goby::acomms::DCCLMessage>::const_iterator goby::acomms::DCCLCodec::to_iterator(const std::string& message_name) const
{
    if(name2messages_.count(message_name))
        return messages_.begin() + name2messages_.find(message_name)->second;
    else
        throw DCCLException(std::string("DCCL: attempted an operation on message [" + message_name + "] which is not loaded"));
}
std::vector<goby::acomms::DCCLMessage>::iterator goby::acomms::DCCLCodec::to_iterator(const std::string& message_name)
{
    if(name2messages_.count(message_name))
        return messages_.begin() + name2messages_.find(message_name)->second;
    else
        throw DCCLException(std::string("DCCL: attempted an operation on message [" + message_name + "] which is not loaded"));
}
std::vector<goby::acomms::DCCLMessage>::const_iterator goby::acomms::DCCLCodec::to_iterator(const unsigned& id) const
{
    if(id2messages_.count(id))
        return messages_.begin() + id2messages_.find(id)->second;
    else
        throw DCCLException(std::string("DCCL: attempted an operation on message [" + as<std::string>(id) + "] which is not loaded"));
}

std::vector<goby::acomms::DCCLMessage>::iterator goby::acomms::DCCLCodec::to_iterator(const unsigned& id)
{
    if(id2messages_.count(id))
        return messages_.begin() + id2messages_.find(id)->second;
    else
        throw DCCLException(std::string("DCCL: attempted an operation on message [" + as<std::string>(id) + "] which is not loaded"));
}


void goby::acomms::DCCLCodec::encode_private(std::vector<DCCLMessage>::iterator it,
                                             std::string& out,
                                             std::map<std::string, std::vector<DCCLMessageVal> > in /* copy */)
{
    if(manip_manager_.has(it->id(), protobuf::MessageFile::NO_ENCODE))
    {
        if(log_) *log_ << group("dccl_enc") << "not encoding DCCL ID: " << it->id() << "; NO_ENCODE manipulator is set" << std::endl;
        throw(DCCLException("NO_ENCODE manipulator set"));
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
    
    // 1. encode parts
    std::string body, head;
    
    it->set_head_defaults(in, cfg_.modem_id());

    it->head_encode(head, in);
    it->body_encode(body, in);
    
    // 2. encrypt
    if(!crypto_key_.empty()) encrypt(body, head);
 
    // 3. join head and body
    out = head + body;

    
    if(log_) *log_ << group("dccl_enc") << "finished encode of " << it->name() <<  std::endl;    
}

std::vector<goby::acomms::DCCLMessage>::iterator goby::acomms::DCCLCodec::decode_private(std::string in,
                                                                                         std::map<std::string, std::vector<DCCLMessageVal> >& out)
{
    std::vector<DCCLMessage>::iterator it = to_iterator(unsigned(DCCLHeaderDecoder(in)[HEAD_DCCL_ID]));

    if(manip_manager_.has(it->id(), protobuf::MessageFile::NO_DECODE))
    {
        if(log_) *log_ << group("dccl_enc") << "not decoding DCCL ID: " << it->id() << "; NO_DECODE manipulator is set" << std::endl;
        throw(DCCLException("NO_DECODE manipulator set"));
    }


    if(log_) *log_ << group("dccl_dec") << "starting decode for " << it->name() << std::endl;        
    
    // clean up any ending junk added by modem
    in.resize(in.find_last_not_of(char(0))+1);
    
    // 3. split body and header (avoid substr::out_of_range)
    std::string body = (DCCL_NUM_HEADER_BYTES < in.size()) ?
        in.substr(DCCL_NUM_HEADER_BYTES) : "";
    std::string head = in.substr(0, DCCL_NUM_HEADER_BYTES);
    
    // 2. decrypt
    if(!crypto_key_.empty()) decrypt(body, head);
    
    // 1. decode parts
    it->head_decode(head, out);
    it->body_decode(body, out);
    
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
        
void goby::acomms::DCCLCodec::encode_private(std::vector<DCCLMessage>::iterator it,
                                             protobuf::ModemDataTransmission* out_msg,
                                             const std::map<std::string, std::vector<DCCLMessageVal> >& in)
{
    std::string out;
    encode_private(it, out, in);

    DCCLHeaderDecoder head_dec(out);

    out_msg->set_data(out);
    
    DCCLMessageVal& t = head_dec[HEAD_TIME];
    DCCLMessageVal& src = head_dec[HEAD_SRC_ID];
    DCCLMessageVal& dest = head_dec[HEAD_DEST_ID];

    out_msg->mutable_base()->set_time(goby::util::as<std::string>(goby::util::unix_double2ptime(double(t))));
    out_msg->mutable_base()->set_src(long(src));
    out_msg->mutable_base()->set_dest(long(dest));

    if(log_) *log_ << group("dccl_enc") << "created encoded message: " << *out_msg << std::endl;
}

std::vector<goby::acomms::DCCLMessage>::iterator goby::acomms::DCCLCodec::decode_private(const protobuf::ModemDataTransmission& in_msg, std::map<std::string,std::vector<DCCLMessageVal> >& out)
{
    if(log_) *log_ << group("dccl_dec") << "using message for decode: " << in_msg << std::endl;

    return decode_private(in_msg.data(), out);
}




void goby::acomms::DCCLCodec::encrypt(std::string& s, const std::string& nonce)
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

void goby::acomms::DCCLCodec::decrypt(std::string& s, const std::string& nonce)
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

void goby::acomms::DCCLCodec::merge_cfg(const protobuf::DCCLConfig& cfg)
{
    cfg_.MergeFrom(cfg);
    process_cfg();
}

void goby::acomms::DCCLCodec::set_cfg(const protobuf::DCCLConfig& cfg)
{
    cfg_.CopyFrom(cfg);
    process_cfg();
}

void goby::acomms::DCCLCodec::process_cfg()
{
    messages_.clear();
    name2messages_.clear();
    id2messages_.clear();
    manip_manager_.clear();

    for(int i = 0, n = cfg_.message_file_size(); i < n; ++i)
    {
        std::set<unsigned> new_ids = add_xml_message_file(cfg_.message_file(i).path());
        BOOST_FOREACH(unsigned new_id, new_ids)
        {
            for(int j = 0, o = cfg_.message_file(i).manipulator_size(); j < o; ++j)
                manip_manager_.add(new_id, cfg_.message_file(i).manipulator(j));
        }
    }
    
    using namespace CryptoPP;
    
    SHA256 hash;
    StringSource unused(cfg_.crypto_passphrase(), true, new HashFilter(hash, new StringSink(crypto_key_)));

    if(log_) *log_ << group("dccl_enc") << "cryptography enabled with given passphrase" << std::endl;
}
