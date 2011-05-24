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

#ifndef DCCL20091211H
#define DCCL20091211H

#include <string>
#include <set>
#include <map>
#include <ostream>
#include <stdexcept>
#include <vector>

#include "goby/util/time.h"
#include "goby/util/logger.h"

#include "goby/core/core_constants.h"
#include "message.h"
#include "message_val.h"
#include "dccl_exception.h"
#include "goby/protobuf/dccl.pb.h"
#include "goby/protobuf/modem_message.pb.h"
#include "goby/acomms/acomms_helpers.h"

/// The global namespace for the Goby project.
namespace goby
{
    namespace util
    { class FlexOstream; }


    /// Objects pertaining to acoustic communications (acomms)
    namespace acomms
    {
        /// use this for displaying a human readable version
        template<typename Value>
            std::ostream& operator<< (std::ostream& out, const std::map<std::string, Value>& m)
        {
            typedef std::pair<std::string, Value> P;
            BOOST_FOREACH(const P& p, m)
            {
                out << "\t" << "key: " << p.first << std::endl
                    << "\t" << "value: " << p.second << std::endl;
            }
            return out;
        }

        template<typename Value>
            std::ostream& operator<< (std::ostream& out, const std::multimap<std::string, Value>& m)
        {
            typedef std::pair<std::string, Value> P;
            BOOST_FOREACH(const P& p, m)
            {
                out << "\t" << "key: " << p.first << std::endl
                    << "\t" << "value: " << p.second << std::endl;
            }
            return out;
        }

    
        /// use this for displaying a human readable version of this STL object
        std::ostream& operator<< (std::ostream& out, const std::set<unsigned>& s);
        /// use this for displaying a human readable version of this STL object
        std::ostream& operator<< (std::ostream& out, const std::set<std::string>& s);

        /// \class DCCLCodec dccl.h goby/acomms/dccl.h
        /// \brief provides an API to the Dynamic CCL Codec.
        /// \ingroup acomms_api
        /// \sa  dccl.proto and modem_message.proto for definition of Google Protocol Buffers messages (namespace goby::acomms::protobuf).

        class DCCLCodec 
        {
          public:
            /// \name Constructors/Destructor
            //@{         
            /// \brief Instantiate optionally with a ostream logger (for human readable output)
            /// \param log std::ostream object or FlexOstream to capture all humanly readable runtime and debug information (optional).
            DCCLCodec(std::ostream* log = 0);

            /// destructor
            ~DCCLCodec() {}
            //@}
        
            /// \name Initialization Methods.
            ///
            /// These methods are intended to be called before doing any work with the class. However,
            /// they may be called at any time as desired.
            //@{

            /// \brief Set (and overwrite completely if present) the current configuration. (protobuf::DCCLConfig defined in dccl.proto)
            void set_cfg(const protobuf::DCCLConfig& cfg);

            /// \brief Set (and merge "repeat" fields) the current configuration. (protobuf::DCCLConfig defined in dccl.proto)
            void merge_cfg(const protobuf::DCCLConfig& cfg);
        
            /// \brief Add an algorithm callback for a MessageVal. The return value is stored back into the input parameter (MessageVal). See test.cpp for an example.
            ///
            /// \param name name of the algorithm (\xmltag{... algorithm="name"})
            /// \param func has the form void name(MessageVal& val_to_edit) (see AdvAlgFunction1). can be a function pointer (&name) or
            /// any function object supported by boost::function (http://www.boost.org/doc/libs/1_34_0/doc/html/function.html)
            void add_algorithm(const std::string& name, AlgFunction1 func);
        
            /// \brief Add an advanced algorithm callback for any DCCL C++ type that may also require knowledge of all the other message variables
            /// and can optionally have additional parameters
            ///
            /// \param name name of the algorithm (\xmltag{... algorithm="name:param1:param2"})
            /// \param func has the form
            /// void name(MessageVal& val_to_edit,
            ///  const std::vector<std::string> params,
            ///  const std::map<std::string,MessageVal>& vals) (see AdvAlgFunction3). func can be a function pointer (&name) or
            /// any function object supported by boost::function (http://www.boost.org/doc/libs/1_34_0/doc/html/function.html).
            /// \param params (passed to func) a list of colon separated parameters passed by the user in the XML file. param[0] is the name.
            /// \param vals (passed to func) a map of \ref tag_name to current values for all message variables.
            void add_adv_algorithm(const std::string& name, AlgFunction2 func);

            /// Registers the group names used for the FlexOstream logger
            static void add_flex_groups(util::FlexOstream* tout);
            
            //@}
        
            /// \name Codec functions.
            ///
            /// This is where the real work happens.
            //@{         
            /// \brief Encode a message.
            ///
            /// \param k can either be std::string (the name of the message) or unsigned (the id of the message)
            /// \param bytes location for the encoded bytes to be stored. this is suitable for sending to the Micro-Modem
            /// \param m map of std::string (\ref tag_name) to a vector of MessageVal representing the values to encode. No fields can be arrays using this call. If fields are arrays, all values but the first in the array will be set to NaN or blank.
            template<typename Key>
                void encode(const Key& k, std::string& bytes,
                            const std::map<std::string, DCCLMessageVal>& m)
            {
                std::map<std::string, std::vector<DCCLMessageVal> > vm;

                typedef std::pair<std::string,DCCLMessageVal> P;
                BOOST_FOREACH(const P& p, m)
                    vm.insert(std::pair<std::string,std::vector<DCCLMessageVal> >(p.first, p.second));
            
                encode_private(to_iterator(k), bytes, vm);
            }

            /// \brief Encode a message.
            ///
            /// \param k can either be std::string (the name of the message) or unsigned (the id of the message)
            /// \param bytes location for the encoded bytes to be stored. this is suitable for sending to the Micro-Modem
            /// \param m map of std::string (\ref tag_name) to a vector of MessageVal representing the values to encode. Fields can be arrays.
            template<typename Key>
                void encode(const Key& k, std::string& bytes,
                            const std::map<std::string, std::vector<DCCLMessageVal> >& m)
            { encode_private(to_iterator(k), bytes, m); }

        
            /// \brief Decode a message.
            ///
            /// \param k can either be std::string (the name of the message) or unsigned (the id of the message
            /// \param bytes the bytes to be decoded.
            /// \param m map of std::string (\ref tag_name) to MessageVal to store the values to be decoded. No fields can be arrays using this call. If fields are arrays, only the first value is returned.
            void decode(const std::string& bytes,
                        std::map<std::string, DCCLMessageVal>& m)
            {
                std::map<std::string, std::vector<DCCLMessageVal> > vm;
                
                decode_private(bytes, vm);
            
                typedef std::pair<std::string,std::vector<DCCLMessageVal> > P;
                BOOST_FOREACH(const P& p, vm)
                    m.insert(std::pair<std::string,DCCLMessageVal>(p.first, DCCLMessageVal(p.second)));
            }
        
            /// \brief Decode a message.
            ///
            /// \param k can either be std::string (the name of the message) or unsigned (the id of the message
            /// \param bytes the bytes to be decoded.
            /// \param m map of std::string (\ref tag_name) to MessageVal to store the values to be decoded
            void decode(const std::string& bytes,
                        std::map<std::string, std::vector<DCCLMessageVal> >& m)
            { decode_private(bytes, m); }
            
        
            //@}
        
            /// \name Informational Methods
            ///
            //@{         
            /// long summary of a message for a given Key (std::string name or unsigned id)
            /// \param k can either be std::string (the name of the message) or unsigned (the id of the message)
            template<typename Key>
                std::string summary(const Key& k) const
            { return to_iterator(k)->get_display(); }
            /// long summary of a message for all loaded messages
            std::string summary() const;

            /// brief summary of a message for a given Key (std::string name or unsigned id)
            template<typename Key>
                std::string brief_summary(const Key& k) const
            { return to_iterator(k)->get_short_display(); }
            /// brief summary of a message for all loaded messages
            std::string brief_summary() const;

            /// how many message are loaded?
            /// \return number of messages loaded
            unsigned message_count() { return messages_.size(); }

            /// \return repeat value (number of copies of the message per encode)
            template<typename Key>
                unsigned get_repeat(const Key& k)
            { return to_iterator(k)->repeat(); }
        
            /// \return set of all message ids loaded
            std::set<unsigned> all_message_ids();
            /// \return set of all message names loaded
            std::set<std::string> all_message_names();
            /// \return map of names to DCCL types needed to encode a given message
            template<typename Key>
                std::map<std::string, std::string> message_var_names(const Key& k) const
            { return to_iterator(k)->message_var_names(); }
        
            /// \param id message id
            /// \return name of message
            std::string id2name(unsigned id) {return to_iterator(id)->name();}
            /// \param name message name
            /// \return id of message
            unsigned name2id(const std::string& name) {return to_iterator(name)->id();}
            
            
            //@}


            /// \name Publish/subscribe architecture related methods
            /// \brief Methods written largely to support DCCL in the context of a publish/subscribe architecture (e.g., see MOOS (http://www.robots.ox.ac.uk/~mobile/MOOS/wiki/pmwiki.php)). The other methods provide a complete interface for encoding and decoding DCCL messages. However, the methods listed here extend the functionality to allow for
            /// <ul>
            /// <li> message creation triggering (a message is encoded on a certain event, either time based or publish based) </li>
            /// <li> encoding that will parse strings of the form: "key1=value,key2=value,key3=value" </li>
            /// <li> decoding to an arbitrarily formatted string (similar concept to printf) </li>
            /// </ul>
            /// These methods will be useful if you are interested in any of the features mentioned above.
        
            //@{
            /// \brief Encode a message using \ref tag_src_var tags instead of \ref tag_name tags
            ///
            /// Values can be passed in on one or more maps of names to values, similar to DCCLCodec::encode. 
            /// Casts are made and string parsing of key=value comma delimited fields is performed.
            /// This differs substantially from the behavior of encode above.
            /// For example, take this message variable:
            /*! \verbatim
              <int>
              <name>myint</name>
              <src_var>somevar</src_var>
              </int> \endverbatim
            */
            ///
            /// Using this method you can pass vals["somevar"] = "mystring=foo,blah=dog,myint=32"
            /// or vals["somevar"] = 32.0
            /// and both cases will properly parse out 32 as the value for this field.
            /// In comparison, using the normal encode you would pass vals["myint"] = 32
            ///
            /// \param k can either be std::string (the name of the message) or unsigned (the id of the message)
            /// \param msg location for the encoded message to be stored (protobuf::ModemDataTransmission defined in modem_message.proto)
            /// \param vals map of source variable name to MessageVal values. 
            template<typename Key>
                void pubsub_encode(const Key& k,
                                   protobuf::ModemDataTransmission* msg,
                                   const std::map<std::string, std::vector<DCCLMessageVal> >& pubsub_vals)
            {
                std::vector<DCCLMessage>::iterator it = to_iterator(k);
                
                if(log_)
                {
                    *log_ << group("dccl_enc") << "starting publish/subscribe encode for " << it->name() << std::endl;
                    *log_ << group("dccl_enc") << "publish/subscribe variables are: " << std::endl;
                    
                    typedef std::pair<std::string, std::vector<DCCLMessageVal> > P;
                    BOOST_FOREACH(const P& p, pubsub_vals)                    
                    {
                        if(!p.first.empty())
                        {
                            BOOST_FOREACH(const DCCLMessageVal& mv, p.second)
                                *log_ << group("dccl_enc") << "\t" << p.first << ": " <<  mv << std::endl;
                        }   
                    }                 
                }

                
                std::map<std::string, std::vector<DCCLMessageVal> > vals;
                // clean the pubsub vals into dccl vals
                // using <src_var/> tag, do casts from double, pull strings from key=value,key=value, etc.
                BOOST_FOREACH(boost::shared_ptr<DCCLMessageVar> mv, it->layout())
                    mv->read_pubsub_vars(vals, pubsub_vals);
                BOOST_FOREACH(boost::shared_ptr<DCCLMessageVar> mv, it->header())
                    mv->read_pubsub_vars(vals, pubsub_vals);
            
                encode_private(it, msg, vals);

                if(log_) *log_ << group("dccl_enc") << "message published to variable: " <<
                            get_outgoing_hex_var(it->id()) << std::endl;
            }

            /// \brief Encode a message using \ref tag_src_var tags instead of \ref tag_name tags
            /// 
            /// Use this version if you do not have vectors of src_var values
            template<typename Key>
                void pubsub_encode(const Key& k,
                                   protobuf::ModemDataTransmission* msg,
                                   const std::map<std::string, DCCLMessageVal>& pubsub_vals)
            {
                std::map<std::string, std::vector<DCCLMessageVal> > vm;

                typedef std::pair<std::string,DCCLMessageVal> P;
                BOOST_FOREACH(const P& p, pubsub_vals)
                    vm.insert(std::pair<std::string,std::vector<DCCLMessageVal> >(p.first, p.second));
            
                pubsub_encode(k, msg, vm);
            }

        
            /// \brief Decode a message using formatting specified in \ref tag_publish tags.
            ///
            /// Values will be received in two maps, one of strings and the other of doubles. The \ref tag_publish value will be placed
            /// either based on the "type" parameter of the \ref tag_publish_var tag (e.g. \<publish_var type="long"\>SOMEVAR\</publish_var\> will be placed as a long). If no type parameter is given and the variable is numeric (e.g. "23242.23") it will be considered a double. If not numeric, it will be considered a string.
            ///
            /// \param k can either be std::string (the name of the message) or unsigned (the id of the message)
            /// \param msg message to be decoded. (protobuf::ModemDataTransmission defined in modem_message.proto)
            /// \param vals pointer to std::multimap of publish variable name to std::string values.
            void pubsub_decode(const protobuf::ModemDataTransmission& msg,
                               std::multimap<std::string, DCCLMessageVal>* pubsub_vals)
                               
            {
                std::map<std::string, std::vector<DCCLMessageVal> > vals;
                std::vector<DCCLMessage>::iterator it = decode_private(msg, vals);
                
                // go through all the publishes_ and fill in the format strings
                BOOST_FOREACH(DCCLPublish& p, it->publishes())
                    p.write_publish(vals, pubsub_vals);

                if(log_)
                {
                    *log_ << group("dccl_dec") << "publish/subscribe variables are: " << std::endl;
                    typedef std::pair<std::string, DCCLMessageVal> P;
                    BOOST_FOREACH(const P& p, *pubsub_vals)
                    {
                        
                        *log_ << group("dccl_dec") << "\t" << p.first << ": " << p.second << std::endl;
                    }                 
                    *log_ << group("dccl_dec") << "finished publish/subscribe decode for " << it->name() << std::endl;
                }
            }
        
        
            /// what moos variables do i need to provide to create a message with a call to encode_using_src_vars
            template<typename Key>
                std::set<std::string> get_pubsub_src_vars(const Key& k)
            { return to_iterator(k)->get_pubsub_src_vars(); }

            /// for a given message name, all architecture variables (sources, input, destination, trigger)
            template<typename Key>
                std::set<std::string> get_pubsub_all_vars(const Key& k)
            { return to_iterator(k)->get_pubsub_all_vars(); }

            /// all architecture variables needed for encoding (includes trigger)
            template<typename Key>
                std::set<std::string> get_pubsub_encode_vars(const Key& k)
            { return to_iterator(k)->get_pubsub_encode_vars(); }

            /// for a given message, all architecture variables for decoding (input)
            template<typename Key>
                std::set<std::string> get_pubsub_decode_vars(const Key& k)
            { return to_iterator(k)->get_pubsub_decode_vars(); }
        
            /// returns outgoing architecture hexadecimal variable
            template<typename Key>
                std::string get_outgoing_hex_var(const Key& k)
            { return to_iterator(k)->out_var(); }

            /// returns incoming architecture hexadecimal variable
            template<typename Key>
                std::string get_incoming_hex_var(const Key& k)
            { return to_iterator(k)->in_var(); }

        
            /// \brief look if key / value are trigger for any loaded messages
            /// if so, store to id and return true
            bool is_publish_trigger(std::set<unsigned>& id, const std::string& key, const std::string& value);

            /// \brief look if the time is right for trigger for any loaded messages
            /// if so, store to id and return true
            bool is_time_trigger(std::set<unsigned>& id);

            /// \brief see if this key is for an incoming message
            /// if so, return id for decoding
            bool is_incoming(unsigned& id, const std::string& key);
            //@}

        
            // returns a reference to all DCCLMessage objects.

            // this is only used if one needs more control than DCCLCodec
            // provides
            std::vector<DCCLMessage>& messages() {return messages_;}

            // grab a reference to the manipulator manager used by the loaded XML messages
            const ManipulatorManager& manip_manager() const { return manip_manager_; }

            /// \example libdccl/dccl_simple/dccl_simple.cpp
            /// simple.xml
            /// \verbinclude dccl_simple/simple.xml
            /// dccl_simple.cpp
            
            /// \example libdccl/plusnet/plusnet.cpp
            /// nafcon_command.xml
            /// \verbinclude nafcon_command.xml
            /// nafcon_report.xml
            /// \verbinclude nafcon_report.xml
            /// plusnet.cpp
        
            /// \example libdccl/test/test.cpp
            /// test.xml
            /// \verbinclude test.xml 
            /// test.cpp
        
            /// \example libdccl/two_message/two_message.cpp
            /// two_message.xml
            /// \verbinclude two_message.xml
            /// two_message.cpp

            /// \example acomms/chat/chat.cpp
        
          private:
            /// \brief Add more messages to this instance of the codec.
            ///
            /// \param xml_file path to the xml file to parse and add to this codec.
            /// \return returns id of the last message file parsed. note that there can be more than one message in a file
            std::set<unsigned> add_xml_message_file(const std::string& xml_file);
            
            std::vector<DCCLMessage>::const_iterator to_iterator(const std::string& message_name) const;
            std::vector<DCCLMessage>::iterator to_iterator(const std::string& message_name);
            std::vector<DCCLMessage>::const_iterator to_iterator(const unsigned& id) const;
            std::vector<DCCLMessage>::iterator to_iterator(const unsigned& id);        

            // in map not passed by reference because we want to be able to modify it
            void encode_private(std::vector<DCCLMessage>::iterator it,
                                std::string& out,
                                std::map<std::string, std::vector<DCCLMessageVal> > in);

            // in string not passed by reference because we want to be able to modify it
            std::vector<DCCLMessage>::iterator decode_private(std::string in,
                                std::map<std::string, std::vector<DCCLMessageVal> >& out);
        
            void encode_private(std::vector<DCCLMessage>::iterator it,
                                protobuf::ModemDataTransmission* out_msg,
                                const std::map<std::string, std::vector<DCCLMessageVal> >& in);
        
            std::vector<DCCLMessage>::iterator decode_private(const protobuf::ModemDataTransmission& in_msg,
                                std::map<std::string, std::vector<DCCLMessageVal> >& out);
        
            void check_duplicates();

        
            void encrypt(std::string& s, const std::string& nonce);
            void decrypt(std::string& s, const std::string& nonce);

            void process_cfg();
            
          private:
            std::ostream* log_;
            
            std::vector<DCCLMessage> messages_;
            std::map<std::string, size_t>  name2messages_;
            std::map<unsigned, size_t>     id2messages_;

            protobuf::DCCLConfig cfg_;

            boost::posix_time::ptime start_time_;

            // SHA256 hash of the crypto passphrase
            std::string crypto_key_;
            
            ManipulatorManager manip_manager_;    
            
        };
        
        /// outputs information about all available messages (same as std::string summary())
        std::ostream& operator<< (std::ostream& out, const DCCLCodec& d);


        class DCCLHeaderEncoder
        {
          public:
            DCCLHeaderEncoder(const std::map<std::string, std::vector<DCCLMessageVal> >& in)
            {
                std::map<std::string, std::vector<DCCLMessageVal> > in_copy = in;
                msg_.head_encode(encoded_, in_copy);
            }
            std::string& str() { return encoded_; }
            
          private:
            DCCLMessage msg_;
            std::string encoded_;        
        };

        class DCCLHeaderDecoder
        {
          public:
            DCCLHeaderDecoder(const std::string& in_orig)
            {
                std::string in = in_orig.substr(0, DCCL_NUM_HEADER_BYTES);
                msg_.head_decode(in, decoded_);
            }   
            std::map<std::string, std::vector<DCCLMessageVal> >& get() { return decoded_; }
            DCCLMessageVal& operator[] (const std::string& s)
            { return decoded_[s][0]; }
            DCCLMessageVal& operator[] (DCCLHeaderPart p)
            { return decoded_[to_str(p)][0]; }

        
          private:
            DCCLMessage msg_;
            std::map<std::string, std::vector<DCCLMessageVal> > decoded_;
        };
    }
}


#endif
