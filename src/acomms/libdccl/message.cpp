// copyright 2008, 2009 t. schneider tes@mit.edu
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

#include <boost/lexical_cast.hpp>
#include <boost/foreach.hpp>

#include "message.h"

#include <crypto++/filters.h>
#include <crypto++/modes.h>
#include <crypto++/aes.h>
#include <crypto++/hex.h>
#include <crypto++/sha.h>


dccl::Message::Message():size_(0),
                         avail_size_(0),
                         trigger_number_(1),    
                         total_bits_(0),
                         id_(0),
                         trigger_time_(0.0),
                         delta_encode_(false),
                         crypto_(false)
{ }

    
// add a new message_var to the current messages vector
void dccl::Message::add_message_var(const std::string& type)
{
    MessageVar m;

    DCCLType dtype = dccl_static;
    if(type == "static")
        dtype = dccl_static;
    else if(type == "int")
        dtype = dccl_int;
    else if(type == "string")
        dtype = dccl_string;
    else if(type == "float")
        dtype = dccl_float;
    else if(type == "enum")
        dtype = dccl_enum;
    else if(type == "bool")
        dtype = dccl_bool;
    else if(type == "hex")
        dtype = dccl_hex;

    m.set_type(dtype);

    layout_.push_back(m);
}

// add a new publish, i.e. a set of parameters to publish
// upon receipt of an incoming (hex) message
void dccl::Message::add_publish()
{
    Publish p;
    publishes_.push_back(p);
}

void dccl::Message::set_crypto_passphrase(const std::string& s)
{
    using namespace CryptoPP;
    SHA256 hash;
    StringSource unused (s, true, new HashFilter(hash, new StringSink(crypto_key_)));
    crypto_ = true;
}

// a number of tasks to perform after reading in an entire <message> from
// the xml file
void dccl::Message::preprocess()
{
    total_bits_ = 0;
    // iterate over layout_
    BOOST_FOREACH(MessageVar& mv, layout_)
    {
        mv.initialize(name_, trigger_var_, delta_encode_);
        // calculate total bits for the message from the bits for each message_var
        total_bits_ += mv.calc_size();
    }
    
    // 'size' is the <size> tag in BYTES
    if(total_bits_ > bytes2bits(avail_size_))
    {
        throw std::runtime_error(std::string("DCCL: " + get_display() + "the message [" + name_ + "] will not fit within specified size. remove parameters, tighten bounds, or increase allowed size. details of the offending message are printed above."));
    }

    // iterate over publishes_
    BOOST_FOREACH(Publish& p, publishes_)
    {
        p.initialize(layout_);
    }


    // set incoming_var / outgoing_var if not set
    if(in_var_ == "")
        in_var_ = "IN_" + boost::to_upper_copy(name_) + "_HEX_" + boost::lexical_cast<std::string>(size_) + "B";
    if(out_var_ == "")
        out_var_ = "OUT_" + boost::to_upper_copy(name_) + "_HEX_" + boost::lexical_cast<std::string>(size_) + "B";
            
    
}

    
// a long visual display of all the parameters for a Message
std::string dccl::Message::get_display() const
{
    const unsigned int num_stars = 20;

    bool is_moos = !trigger_type_.empty();
        
    std::stringstream ss;
    ss << std::string(num_stars, '*') << std::endl;
    ss << "message " << id_ << ": {" << name_ << "}" << std::endl;

    if(is_moos)
    {
        ss << "trigger_type: {" << trigger_type_ << "}" << std::endl;
    
    
        if(trigger_type_ == "publish")
        {
            ss << "trigger_var: {" << trigger_var_ << "}";
            if (trigger_mandatory_ != "")
                ss << " must contain string \"" << trigger_mandatory_ << "\"";
            ss << std::endl;
        }
        else if(trigger_type_ == "time")
        {
            ss << "trigger_time: {" << trigger_time_ << "}" << std::endl;
        }
    
        ss << "outgoing_hex_var: {" << out_var_ << "}" << std::endl;
        ss << "incoming_hex_var: {" << in_var_ << "}" << std::endl;
    }
    
    ss << "requested size {bytes} [bits]: {" << requested_bytes() << "} [" << requested_bits() << "]" << std::endl;
    ss << "actual size {bytes} [bits]: {" << used_bytes() << "} [" << used_bits() << "]" << std::endl;
    
    ss << ">>>> LAYOUT (message_vars) <<<<" << std::endl;

    BOOST_FOREACH(const MessageVar& mv, layout_)
        ss << mv;
    
    if(is_moos)
    {

        ss << ">>>> PUBLISHES <<<<" << std::endl;
        
        BOOST_FOREACH(const Publish& p, publishes_)
            ss << p;
    }
    
    ss << std::string(num_stars, '*') << std::endl;
        
    return ss.str();
}

// a much shorter rundown of the Message parameters
std::string dccl::Message::get_short_display() const
{
    std::stringstream ss;

    ss << name_ <<  ": ";

    bool is_moos = !trigger_type_.empty();
    if(is_moos)
    {
        ss << "trig: ";
        
        if(trigger_type_ == "publish")
            ss << trigger_var_;
        else if(trigger_type_ == "time")
            ss << trigger_time_ << "s";
        ss << " | out: " << out_var_;
        ss << " | in: " << in_var_ << " | ";
    }

    ss << "size: {" << used_bytes() << "/" << requested_bytes() << "B} [" <<  used_bits() << "/" << requested_bits() << "b] | message var N: " << layout_.size() << std::endl;

    return ss.str();
}

std::map<std::string, std::string> dccl::Message::message_var_names() const
{
    std::map<std::string, std::string> s;
    BOOST_FOREACH(const MessageVar& mv, layout_)
        s.insert(std::pair<std::string, std::string>(mv.name(), type_to_string(mv.type())));
    return s;
}

std::set<std::string> dccl::Message::encode_vars()
{
    std::set<std::string> s = src_vars();
    if(trigger_type_ == "publish")
        s.insert(trigger_var_);
    return s;
}

std::set<std::string> dccl::Message::decode_vars()
{
    std::set<std::string> s;
    s.insert(in_var_);
    return s;
}
    
std::set<std::string> dccl::Message::all_vars()
{
    std::set<std::string> s_enc = encode_vars();
    std::set<std::string> s_dec = decode_vars();

    std::set<std::string>& s = s_enc;        
    s.insert(s_dec.begin(), s_dec.end());
        
    return s;
}
    
std::set<std::string> dccl::Message::src_vars()
{
    std::set<std::string> s;
    BOOST_FOREACH(MessageVar& mv, layout_)
        s.insert(mv.source_var());

    if(!dest_var_.empty())
        s.insert(dest_var_);
        
    return s;
}

//display a summary of what input values you need
std::string dccl::Message::input_summary()
{
    std::map<std::string, std::string> m;
    BOOST_FOREACH(MessageVar& mv, layout_)
    {
        DCCLType type = mv.type();
        std::string view_type;            

        if(type == dccl_static)
            view_type = mv.static_val();
        else if(type == dccl_enum)
        {
            view_type = "{";

            unsigned j = 0;
            BOOST_FOREACH(const std::string& s, mv.enums())
            {
                if(j) view_type += "|";
                view_type += s;
                ++j;
            }
                
            view_type += "}";
        }
        else if(type == dccl_bool)
        {
            view_type = "{true|false}";
        }
        else if(type == dccl_string)
        {
            view_type = "{string[";
            view_type += boost::lexical_cast<std::string>(mv.max_length());
            view_type += "]}";
        }
        else if(type == dccl_hex)
        {
            view_type = "{hex[";
            view_type += boost::lexical_cast<std::string>(mv.max_length());
            view_type += "]}";
        }
        else if(type == dccl_int)
        {
            view_type += "{int";
            view_type += "[" + boost::lexical_cast<std::string>(mv.min()) + "," + boost::lexical_cast<std::string>(mv.max()) + "]";
            view_type += "}";
        }
        else if(type == dccl_float)
        {
            view_type += "{float";
            std::stringstream min_ss, max_ss;
            min_ss << std::fixed << std::setprecision(mv.precision()) << mv.min();
            max_ss << std::fixed << std::setprecision(mv.precision()) << mv.max();                
            view_type += "[" + min_ss.str() + "," + max_ss.str() + "]";
            view_type += "}";
        }
            
        m[mv.source_var()] += mv.name() + (std::string)"=" + view_type + (std::string)",";
    }

    std::string s;
    typedef std::pair<std::string, std::string> P;
    BOOST_FOREACH(const P& p, m)
        s += "[variable] " + p.first + " [contents] " + p.second + "\n";
        
    return s;
}



void dccl::Message::encode(std::string& out, const std::map<std::string, MessageVal>& in)
{
    // make a copy because we need to modify this (algorithms, etc.)
    std::map<std::string, MessageVal> vals = in;

    boost::dynamic_bitset<unsigned char> bits(bytes2bits(used_bytes_no_head())); // actual size rounded up to closest byte

   // 1. encode each variable
    for (std::vector<MessageVar>::iterator it = layout_.begin(), n = layout_.end(); it != n; ++it)
        it->var_encode(vals, bits);

    // 2. convert bitset to string
    assemble(out, bits);

    // 3. encrypt
    if(crypto_) encrypt(out);

    // 4. add on the header
    out = std::string(1, acomms_util::DCCL_CCL_HEADER) + std::string(1, id_) + out;
    
    // 5. hex encode
    hex_encode(out);
}
    
void dccl::Message::decode(const std::string& in_const, std::map<std::string, MessageVal>& out)
{
    std::string in = in_const;
    // 5. hex decode
    hex_decode(in);

    // 4. remove the header
    in = in.substr(acomms_util::NUM_HEADER_BYTES);
    
    // 3. decrypt
    if(crypto_) decrypt(in);
    
    boost::dynamic_bitset<unsigned char> bits(bytes2bits(used_bytes_no_head()));
    // 2. convert string to bitset
    disassemble(in, bits);    
    
    // 1. pull the bits off the message in the reverse that they were put on
    for (std::vector<MessageVar>::reverse_iterator it = layout_.rbegin(), n = layout_.rend(); it != n; ++it)
        it->var_decode(out, bits);
}


void dccl::Message::assemble(std::string& out, const boost::dynamic_bitset<unsigned char>& bits)
{
    // resize the string to fit the bitset
    out.resize(bits.num_blocks());

    // bitset to string
    to_block_range(bits, out.rbegin()); 


    // strip all the ending zeros
    out.resize(out.find_last_not_of(char(0))+1);
}

void dccl::Message::disassemble(std::string& in, boost::dynamic_bitset<unsigned char>& bits)
{
    // copy because we may need to tack some stuff
    size_t in_bytes = in.length();

    if(in_bytes < used_bytes()) // message is too short, add zeroes
        in += std::string(used_bytes_no_head()-in_bytes, 0);
    else if(in_bytes > used_bytes()) // message is too long, truncate
        in = in.substr(0, used_bytes_no_head());

    from_block_range(in.rbegin(), in.rend(), bits);
}


void dccl::Message::hex_encode(std::string& s)
{
    using namespace CryptoPP;
    
    std::string out;
    const bool uppercase = false;
    HexEncoder hex(new StringSink(out), uppercase);
    hex.Put((byte*)s.c_str(), s.size());
    hex.MessageEnd();
    s = out;
}

void dccl::Message::hex_decode(std::string& s)
{
    using namespace CryptoPP;

    std::string out;
    HexDecoder hex(new StringSink(out));
    hex.Put((byte*)s.c_str(), s.size());
    hex.MessageEnd();
    s = out;
}


void dccl::Message::encrypt(std::string& s)
{
    using namespace CryptoPP;

    std::string nonce = "FIXTHISHACK";
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

void dccl::Message::decrypt(std::string& s)
{
    using namespace CryptoPP;

    std::string nonce = "FIXTHISHACK";
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
    
void dccl::Message::add_destination(modem::Message& out_message,
                                    const std::map<std::string, dccl::MessageVal>& in)
{
    if(!dest_var_.empty())
    {        
        MessageVar mv;
        mv.set_source_key(dest_key_);
        mv.set_name("destination");
            
        const std::map<std::string, dccl::MessageVal>::const_iterator it =
            in.find(dest_var_);

        if(it != in.end())
        {
            MessageVal val = it->second;
            
            switch(val.type())
            {
                case cpp_string: 
                    val = mv.parse_string_val(val);
                    break;
                    
                default:
                    break;
            }
            out_message.set_dest(int(val));
        }
        else
        {
            throw std::runtime_error(std::string("DCCL: for message [" + name_ + "]: destination requested but not provided."));
        }
    }
}


    
// overloaded <<
std::ostream& dccl::operator<< (std::ostream& out, const Message& message)
{
    out << message.get_display();
    return out;
}    
