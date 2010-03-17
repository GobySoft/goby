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

#include <ctime>
#include <string>
#include <set>
#include <map>
#include <ostream>
#include <stdexcept>
#include <vector>

#include "acomms/xml/xml_parser.h"

#include "message.h"
#include "message_val.h"

/// \brief contains Dynamic Compact Control Language objects.
/// 
/// Use \code #include <goby/acomms/dccl.h> \endcode to gain access to all these objects.
namespace dccl
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


    
    /// provides an API to the Dynamic CCL Codec.
    class DCCLCodec 
    {
      public:
        /// \name Constructors/Destructor
        //@{         
        /// \brief Instantiate with no XML files.
        DCCLCodec();
        /// \brief Instantiate with a single XML file.
        ///
        /// \param file path to an XML message file (i.e. contains \ref tag_layout and (optionally) \ref tag_publish sections) to parse for use by the codec.
        /// \param schema path (absolute or relative to the XML file path) for the validating schema (message_schema.xsd) (optional).
        DCCLCodec(const std::string& file, const std::string schema = "");
        
        /// \brief Instantiate with a set of XML files.
        ///
        /// \param files set of paths to XML message files to parse for use by the codec.
        /// \param schema path (absolute or relative to the XML file path) for the validating schema (message_schema.xsd) (optional).
        DCCLCodec(const std::set<std::string>& files, const std::string schema = "");

        /// destructor
        ~DCCLCodec() {}
        //@}
        
        /// \name Initialization Methods.
        ///
        /// These methods are intended to be called before doing any work with the class. However,
        /// they may be called at any time as desired.
        //@{         

        /// \brief Add more messages to this instance of the codec.
        ///
        /// \param xml_file path to the xml file to parse and add to this codec.
        /// \param xml_schema path to the message_schema.xsd file to validate XML with. if using a relative path this
        /// must be relative to the directory of the xml_file, not the present working directory. if not provided
        /// no validation is done.
        /// \return returns id of the last message file parsed. note that there can be more than one message in a file
        std::set<unsigned> add_xml_message_file(const std::string& xml_file, const std::string xml_schema = "");

        /// \brief Set the schema used for xml syntax checking.
        /// 
        /// location is relative to the XML file location!
        /// if you have XML files in different places you must pass the
        /// proper relative path (or just use absolute paths)
        /// \param schema location of the message_schema.xsd file
        void set_schema(const std::string& schema) { xml_schema_ = schema; }

        /// \brief Set a passphrase for encrypting all messages with
        /// 
        /// \param passphrase text passphrase
        void set_crypto_passphrase(const std::string& passphrase);
        
        /// \brief Add an algorithm callback for a C++ std::string type that only requires the current value.
        ///
        /// C++ std::string primarily corresponds to DCCL types
        /// \ref tag_hex, \ref tag_string and \ref tag_enum, but can be used with any of the types
        /// through automatic (internal) casting.
        /// \param name name of the algorithm (\xmltag{... algorithm="name"})
        /// \param func has the form void name(std::string& val_to_edit) (see dccl::StrAlgFunction1). can be a function pointer (&name) or
        /// any function object supported by boost::function (http://www.boost.org/doc/libs/1_34_0/doc/html/function.html)        
        void add_str_algorithm(const std::string& name, StrAlgFunction1 func);
        
        /// \brief Add an algorithm callback for a C++ double type that only requires the current value.
        ///
        /// C++ double primarily corresponds to DCCL type
        /// \ref tag_float, but can be used with any of the types
        /// through automatic (internal) casting if they can
        /// be properly represented as a double. 
        /// \param name name of the algorithm (<... algorithm="name"/>)
        /// \param func has the form void name(double& val_to_edit) (see dccl::DblAlgFunction1). can be a function pointer (&name) or
        /// any function object supported by boost::function (http://www.boost.org/doc/libs/1_34_0/doc/html/function.html)
        void add_dbl_algorithm(const std::string& name, DblAlgFunction1 func);
        
        /// \brief Add an algorithm callback for a C++ long int type that only requires the current value.
        ///
        /// C++ long primarily corresponds to DCCL type
        /// \ref tag_int, but can be used with any of the types
        /// through automatic (internal) casting if they can
        /// be properly represented as a long. 
        /// \param name name of the algorithm (<... algorithm="name"/>)
        /// \param func has the form void name(long& val_to_edit) (see dccl::LongAlgFunction1). can be a function pointer (&name) or
        /// any function object supported by boost::function (http://www.boost.org/doc/libs/1_34_0/doc/html/function.html)
        void add_long_algorithm(const std::string& name, LongAlgFunction1 func);
        
        /// \brief Add an algorithm callback  for a C++ bool type that only requires the current value.
        ///
        /// C++ bool primarily corresponds to DCCL type
        /// \ref tag_bool, but can be used with any of the types
        /// through automatic (internal) casting if they can
        /// be properly represented as a bool. 
        /// \param name name of the algorithm (<... algorithm="name"/>)
        /// \param func has the form void name(bool& val_to_edit) (see dccl::BoolAlgFunction1). can be a function pointer (&name) or
        /// any function object supported by boost::function (http://www.boost.org/doc/libs/1_34_0/doc/html/function.html)
        void add_bool_algorithm(const std::string& name, BoolAlgFunction1 func);
        
        /// \brief Add an algorithm callback for a dccl::MessageVal. This allows you to have a different input value (e.g. int) as the output value (e.g. string). 
        ///
        /// \param name name of the algorithm (\xmltag{... algorithm="name"})
        /// \param func has the form void name(dccl::MessageVal& val_to_edit) (see dccl::AdvAlgFunction1). can be a function pointer (&name) or
        /// any function object supported by boost::function (http://www.boost.org/doc/libs/1_34_0/doc/html/function.html)
        void add_generic_algorithm(const std::string& name, AdvAlgFunction1 func);
        
        /// \brief Add an advanced algorithm callback for any DCCL C++ type that may also require knowledge of all the other message variables
        /// and can optionally have additional parameters
        ///
        /// \param name name of the algorithm (\xmltag{... algorithm="name:param1:param2"})
        /// \param func has the form
        /// void name(MessageVal& val_to_edit,
        ///  const std::vector<std::string> params,
        ///  const std::map<std::string,MessageVal>& vals) (see dccl::AdvAlgFunction3). func can be a function pointer (&name) or
        /// any function object supported by boost::function (http://www.boost.org/doc/libs/1_34_0/doc/html/function.html).
        /// \param params (passed to func) a list of colon separated parameters passed by the user in the XML file. param[0] is the name.
        /// \param vals (passed to func) a map of \ref tag_name to current values for all message variables.
        void add_adv_algorithm(const std::string& name, AdvAlgFunction3 func);
        //@}
        
        /// \name Codec functions.
        ///
        /// This is where the real work happens.
        //@{         
        /// \brief Encode a message.
        ///
        /// \param k can either be std::string (the name of the message) or unsigned (the id of the message)
        /// \param hex location for the encoded hexadecimal to be stored. this is suitable for sending to the Micro-Modem
        /// \param m map of std::string (\ref tag_name) to dccl::MessageVal representing the values to encode
        template<typename Key>
            void encode(const Key& k, std::string& hex,
                        const std::map<std::string, MessageVal>& m)
        { encode_private(to_iterator(k), hex, m); }

        /// \brief Decode a message.
        ///
        /// \param k can either be std::string (the name of the message) or unsigned (the id of the message)
        /// \param hex the hexadecimal to be decoded.
        /// \param m map of std::string (\xmltag name) to dccl::MessageVal to store the values to be decoded
        template<typename Key>
            void decode(const Key& k, const std::string& hex,
                        std::map<std::string, MessageVal>& m)
        { decode_private(to_iterator(k), hex, m); }
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
        /// \param msg modem::Message or std::string for encoded message to be stored.
        /// \param vals map of source variable name to dccl::MessageVal values. 
        template<typename Key>
            void pubsub_encode(const Key& k,
                               modem::Message& msg,
                               const std::map<std::string, dccl::MessageVal>& pubsub_vals)
 	{
            std::vector<dccl::Message>::iterator it = to_iterator(k);

            std::map<std::string, dccl::MessageVal> vals;
            // clean the pubsub vals into dccl vals
            // using <src_var/> tag, do casts from double, pull strings from key=value,key=value, etc.
            BOOST_FOREACH(MessageVar& mv, it->layout())
                mv.read_dynamic_vars(vals, pubsub_vals);

            encode_private(it, msg, vals);
            
            // deal with the destination
            it->add_destination(msg, pubsub_vals);
        }

        
        /// \brief Decode a message using formatting specified in \ref tag_publish tags.
        ///
        /// Values will be received in two maps, one of strings and the other of doubles. The \ref tag_publish value will be placed
        /// either based on the "type" parameter of the \ref tag_publish_var tag (e.g. \<publish_var type="long"\>SOMEVAR\</publish_var\> will be placed as a long). If no type parameter is given and the variable is numeric (e.g. "23242.23") it will be considered a double. If not numeric, it will be considered a string.
        ///
        /// \param k can either be std::string (the name of the message) or unsigned (the id of the message)
        /// \param msg modem::Message or std::string to be decode.
        /// \param vals pointer to std::multimap of publish variable name to std::string values.
        template<typename Key>
            void pubsub_decode(const Key& k,
                               const modem::Message& msg,
                               std::multimap<std::string, dccl::MessageVal>& pubsub_vals)
                               
        {
            std::vector<dccl::Message>::iterator it = to_iterator(k);

            std::map<std::string, dccl::MessageVal> vals;
            decode_private(it, msg, vals);

            // go through all the publishes_ and fill in the format strings
            BOOST_FOREACH(Publish& p, it->publishes())
                p.write_publish(vals, it->layout(), pubsub_vals);
        }
        
        
        /// what moos variables do i need to provide to create a message with a call to encode_using_src_vars
        template<typename Key>
            std::set<std::string> src_vars(const Key& k)
        { return to_iterator(k)->src_vars(); }

        /// for a given message name, all MOOS variables (sources, input, destination, trigger)
        template<typename Key>
            std::set<std::string> all_vars(const Key& k)
        { return to_iterator(k)->all_vars(); }

        /// all MOOS variables needed for encoding (includes trigger)
        template<typename Key>
            std::set<std::string> encode_vars(const Key& k)
        { return to_iterator(k)->encode_vars(); }

        /// for a given message name, all MOOS variables for decoding (input)
        template<typename Key>
            std::set<std::string> decode_vars(const Key& k)
        { return to_iterator(k)->decode_vars(); }

        /// what is the syntax of the message i need to provide?
        template<typename Key>
            std::string input_summary(const Key& k)
        { return to_iterator(k)->input_summary(); }
        
        /// returns outgoing moos variable (hexadecimal) 
        template<typename Key>
            std::string get_out_moos_var(const Key& k)
        { return to_iterator(k)->out_var(); }

        /// returns incoming moos variable (hexadecimal) 
        template<typename Key>
            std::string get_in_moos_var(const Key& k)
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

        
        // returns a reference to all Message objects.

        // this is only used if one needs more control than DCCLCodec
        // provides
        std::vector<Message>& messages() {return messages_;}
        

        /// \example libdccl/examples/dccl_simple/dccl_simple.cpp
        /// simple.xml
        /// \verbinclude dccl_simple/simple.xml
        /// dccl_simple.cpp
        
        /// \example libdccl/examples/plusnet/plusnet.cpp
        /// nafcon_command.xml
        /// \verbinclude nafcon_command.xml
        /// nafcon_report.xml
        /// \verbinclude nafcon_report.xml
        /// plusnet.cpp
        
        /// \example libdccl/examples/test/test.cpp
        /// test.xml
        /// \verbinclude test.xml 
        /// test.cpp
        
        /// \example libdccl/examples/two_message/two_message.cpp
        /// two_message.xml
        /// \verbinclude two_message.xml
        /// two_message.cpp

        /// \example acomms/examples/chat/chat.cpp
        
      private:
        std::vector<Message>::const_iterator to_iterator(const std::string& message_name) const;
        std::vector<Message>::iterator to_iterator(const std::string& message_name);
        std::vector<Message>::const_iterator to_iterator(const unsigned& id) const;
        std::vector<Message>::iterator to_iterator(const unsigned& id);        

        // in map not passed by reference because we want to be able to modify it
        void encode_private(std::vector<Message>::iterator it,
                            std::string& out,
                            std::map<std::string, MessageVal> in);

        // in string not passed by reference because we want to be able to modify it
        void decode_private(std::vector<Message>::iterator it,
                            std::string in,
                            std::map<std::string, MessageVal>& out);

        
        void encode_private(std::vector<Message>::iterator it,
                            modem::Message& out_msg,
                            const std::map<std::string, MessageVal>& in);
        
        void decode_private(std::vector<Message>::iterator it,
                            const modem::Message& in_msg,
                            std::map<std::string, MessageVal>& out);
        
        void check_duplicates();
        
        void encrypt(std::string& s, const std::string& nonce);
        void decrypt(std::string& s, const std::string& nonce);
        
      private:
        std::vector<Message> messages_;
        std::map<std::string, size_t>  name2messages_;
        std::map<unsigned, size_t>     id2messages_;

        std::string xml_schema_;
        time_t start_time_;

        std::string crypto_key_;
    };

    /// outputs information about all available messages (same as std::string summary())
    std::ostream& operator<< (std::ostream& out, const DCCLCodec& d);


    
}



#endif
