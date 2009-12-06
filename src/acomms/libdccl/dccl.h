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

#ifndef DCCL_H
#define DCCL_H

#include <ctime>
#include <string>
#include <set>
#include <map>
#include <ostream>
#include <stdexcept>
#include <vector>

#include "message.h"
#include "xml_parser.h"
#include "message_val.h"


/// all Dynamic Compact Control Language objects are in the `dccl` namespace
namespace dccl
{
    /// provides an API to the Dynamic CCL Codec.
    class DCCLCodec 
    {
      public:
        /// \name Constructors/Destructor
        //@{         
        /// instantiate with no XML files
        DCCLCodec();
        /// instantiate with a single XML file
        DCCLCodec(const std::string& file, const std::string schema = "");
        
        /// instantiate with a set of XML files
        DCCLCodec(const std::set<std::string>& files, const std::string schema = "");

        /// destructor
        ~DCCLCodec() {}
        //@}
        
        /// \name Initialization Methods
        ///
        /// These methods are intended to be called before doing any work with the class. However,
        /// they may be called at any time as desired.
        //@{         

        /// \brief add more messages to this instance of the codec
        ///
        /// \param xml_file path to the xml file to parse and add to this codec.
        /// \param xml_schema path to the message_schema.xsd file to validate XML with. if using a relative path this
        /// must be relative to the directory of the xml_file, not the present working directory. if not provided
        /// no validation is done.
        /// \return returns id of the last message file parsed. note that there can be more than one message in a file
        std::set<unsigned> add_xml_message_file(const std::string& xml_file, const std::string xml_schema = "");

        /// \brief set the schema used for xml syntax checking
        /// 
        /// location is relative to the XML file location!
        /// if you have XML files in different places you must pass the
        /// proper relative path (or just use absolute paths)
        /// \param schema location of the message_schema.xsd file
        void set_schema(const std::string schema) { xml_schema_ = schema; }

        /// \brief Add an algorithm callback for a C++ std::string type that only requires the current value.
        ///
        /// C++ std::string primarily corresponds to DCCL types
        /// <string/> and <enum/>, but can be used with any of the types
        /// through automatic (internal) casting.
        /// \param name name of the algorithm (<... algorithm="name"/>)
        /// \param func has the form void name(std::string& val_to_edit). can be a function pointer (&name) or
        /// any function object supported by boost::function (http://www.boost.org/doc/libs/1_34_0/doc/html/function.html)
        template<typename Function>
            void add_str_algorithm(const std::string& name, Function func)
        {
            AlgorithmPerformer * ap = AlgorithmPerformer::getInstance();
            ap -> add_str_algorithm(name, func);
        }

        /// \brief Add an algorithm callback for a C++ double type that only requires the current value.
        ///
        /// C++ double primarily corresponds to DCCL type
        /// <float/>, but can be used with any of the types
        /// through automatic (internal) casting if they can
        /// be properly represented as a double. 
        /// \param name name of the algorithm (<... algorithm="name"/>)
        /// \param func has the form void name(double& val_to_edit). can be a function pointer (&name) or
        /// any function object supported by boost::function (http://www.boost.org/doc/libs/1_34_0/doc/html/function.html)
        template<typename Function>
            void add_dbl_algorithm(const std::string& name, Function func)
        {
            AlgorithmPerformer * ap = AlgorithmPerformer::getInstance();
            ap -> add_dbl_algorithm(name, func);
        }
        
        /// \brief Add an algorithm callback for a C++ long int type that only requires the current value.
        ///
        /// C++ long primarily corresponds to DCCL type
        /// <int/>, but can be used with any of the types
        /// through automatic (internal) casting if they can
        /// be properly represented as a long. 
        /// \param name name of the algorithm (<... algorithm="name"/>)
        /// \param func has the form void name(long& val_to_edit). can be a function pointer (&name) or
        /// any function object supported by boost::function (http://www.boost.org/doc/libs/1_34_0/doc/html/function.html)
        template<typename Function>
            void add_long_algorithm(const std::string& name, Function func)
        {
            AlgorithmPerformer * ap = AlgorithmPerformer::getInstance();
            ap -> add_long_algorithm(name, func);
        }

        /// \brief Add an algorithm callback  for a C++ bool type that only requires the current value.
        ///
        /// C++ bool primarily corresponds to DCCL type
        /// <bool/>, but can be used with any of the types
        /// through automatic (internal) casting if they can
        /// be properly represented as a bool. 
        /// \param name name of the algorithm (<... algorithm="name"/>)
        /// \param func has the form void name(bool& val_to_edit). can be a function pointer (&name) or
        /// any function object supported by boost::function (http://www.boost.org/doc/libs/1_34_0/doc/html/function.html)
        template<typename Function>
            void add_bool_algorithm(const std::string& name, Function func)
        {
            AlgorithmPerformer * ap = AlgorithmPerformer::getInstance();
            ap -> add_bool_algorithm(name, func);
        }

        /// \brief Add an algorithm callback for a dccl::MessageVal. This allows you to have a different input value (e.g. int) as the output value (e.g. string). 
        ///
        /// \param name name of the algorithm (<... algorithm="name"/>)
        /// \param func has the form void name(dccl::MessageVal& val_to_edit). can be a function pointer (&name) or
        /// any function object supported by boost::function (http://www.boost.org/doc/libs/1_34_0/doc/html/function.html)
        template<typename Function>
            void add_generic_algorithm(const std::string& name, Function func)
        {
            AlgorithmPerformer * ap = AlgorithmPerformer::getInstance();
            ap -> add_generic_algorithm(name, func);
        }

        /// \brief Add an advanced algorithm callback for any DCCL C++ type that may also require knowledge of all the other message variables
        /// and can optionally have additional parameters
        ///
        /// \param name name of the algorithm (<... algorithm="name:param1:param2"/>)
        /// \param func has the form
        /// void name(MessageVal& val_to_edit,
        ///  const std::vector<std::string> params,
        ///  const std::map<std::string,MessageVal>& vals). func can be a function pointer (&name) or
        /// any function object supported by boost::function (http://www.boost.org/doc/libs/1_34_0/doc/html/function.html).
        /// \param params (passed to func) a list of colon separated parameters passed by the user in the XML file. param[0] is the name.
        /// \param vals (passed to func) a map of <name/> to current values for all message variables.
        template<typename AdvFunction>
            void add_adv_algorithm(const std::string& name, AdvFunction func)
        {
            AlgorithmPerformer * ap = AlgorithmPerformer::getInstance();
            ap -> add_adv_algorithm(name, func);
        }
        //@}
        
        /// \name Codec functions.
        ///
        /// This is where the real work happens.
        //@{         
        /// \brief Encode a message.
        ///
        /// Values can be passed in on one or more maps of names to values. The types passed (std::string, double, long, bool) should match the DCCL types (<string>, <float>, <int>, <bool>, <enum>) as well
        /// as possible, but all reasonable casts will be made (e.g. std::string("234") is a valid <int/> and double(2.4) could be used
        /// as a <bool/> (true))
        ///
        /// \param k can either be std::string (the name of the message) or unsigned (the id of the message)
        /// \param hex location for the encoded hexadecimal to be stored. this is suitable for sending to the Micro-Modem
        /// \param ms pointer to map of message variable name to std::string values. std::string is preferred for <string/> and <enum/>. optional, pass 0 (null pointer) if not using.
        /// \param md pointer to map of message variable name to double values. double is preferred for <float/>. optional, pass 0 (null pointer) if not using.
        /// \param ml pointer to map of message variable name to double values. long is preferred for <int/>. optional, pass 0 (null pointer) if not using.
        /// \param mb pointer to map of message variable name to bool values. bool is preferred for <bool/>. optional, pass 0 (null pointer) if not using.
        template<typename Key>
            void encode(const Key& k, std::string& hex,
                        const std::map<std::string, std::string>* ms,
                        const std::map<std::string, double>* md,
                        const std::map<std::string, long>* ml,
                        const std::map<std::string, bool>* mb)
        { encode_private(to_iterator(k), hex, ms, md, ml, mb); }        

        /// \brief Decode a message.
        ///
        /// Values will be recieved in  one or more maps of names to values. All reasonable casts will be made and returned in the maps
        /// provided. Thus, <int>234</int> would be returned in ms as "234", md as 234.000, ml as 234, and mb as true.
        ///
        /// \param k can either be std::string (the name of the message) or unsigned (the id of the message)
        /// \param hex the hexadecimal to be decoded.
        /// \param ms pointer to map of message variable name to std::string values. optional, pass 0 (null pointer) if not using.
        /// \param md pointer to map of message variable name to double values. optional, pass 0 (null pointer) if not using.
        /// \param ml pointer to map of message variable name to double values. optional, pass 0 (null pointer) if not using.
        /// \param mb pointer to map of message variable name to bool values. optional, pass 0 (null pointer) if not using.
        template<typename Key>
            void decode(const Key& k, const std::string& hex,
                        std::map<std::string, std::string>* ms,
                        std::map<std::string, double>* md,
                        std::map<std::string, long>* ml,
                        std::map<std::string, bool>* mb)
        { decode_private(to_iterator(k), hex, ms, md, ml, mb); }
        //@}
        
        /// \name Informational Methods
        ///
        //@{         
        /// long summary of a message for a given Key (std::string name or unsigned id)
        /// \param k can either be std::string (the name of the message) or unsigned (the id of the message)
        template<typename Key>
            std::string summary(const Key& k) const
        { return summary(to_iterator(k)); }
        /// long summary of a message for all loaded messages
        std::string summary() const;

        /// brief summary of a message for a given Key (std::string name or unsigned id)
        template<typename Key>
            std::string brief_summary(const Key& k) const
        { return brief_summary(to_iterator(k)); }
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
        { return message_var_names(to_iterator(k)); }
        
        /// \param id message id
        /// \return name of message
        std::string id2name(unsigned id) {return to_iterator(id)->name();}
        /// \param name message name
        /// \return id of message
        unsigned name2id(const std::string& name) {return to_iterator(name)->id();}
        //@}

        
        /////////////////
        // MOOS!
        /////////////////

        /// \name MOOS related methods
        /// \brief Methods written largely to support DCCL in the context of the MOOS autonomy infrastructure (http://www.robots.ox.ac.uk/~mobile/MOOS/wiki/pmwiki.php). The other methods provide a complete interface for encoding and decoding DCCL messages. However, the methods listed here extend the functionality to allow for
        /// <ul>
        /// <li> message creation triggering (a message is encoded on a certain event, either time based or publish based) </li>
        /// <li> message queuing encoded in XML (this is used by pAcommsHandler) </li>
        /// <li> encoding that will parse strings of the form: "key1=value,key2=value,key3=value" </li>
        /// <li> decoding to an arbitrary formatted string (similar concept to printf) </li>
        /// </ul>
        /// These methods may be useful to someone not working with MOOS but interested in any of the features mentioned above.
        
        //@{
        /// \brief Encode a message using <moos_var> tags instead of <name> tags
        ///
        /// Values can be passed in on one or more maps of names to values. The types passed (std::string, double) are typically directly
        /// derived from the MOOS types given to this process (std::string / double) since MOOS only supports these two types.
        /// Casts are made and string parsing of key=value comma delimited fields is performed.
        /// This differs substantially from the behavior of encode above.
        /// For example, take this message variable:
        /// <int><name>myint</name><moos_var>somevar</moos_var></int>
        ///
        /// Using encode_from_moos you can pass *ms["somevar"] = "mystring=foo,blah=dog,myint=32"
        /// or *md["somevar"] = 32.0
        /// and both cases will properly parse out 32 as the value for this field.
        /// In comparison, using encode you would pass *ml["myint"] = 32
        ///
        /// \param k can either be std::string (the name of the message) or unsigned (the id of the message)
        /// \param Msg micromodem::Message or std::string for encoded message to be stored.
        /// \param ms pointer to map of moos variable name to std::string values. 
        /// \param md pointer to map of moos variable name to double values. double is preferred for <float/>.
        template<typename Key, typename Msg>
            void encode_from_src_vars(const Key& k,
                                       Msg& m,
                                       const std::map<std::string, std::string>* ms,
                                       const std::map<std::string, double>* md,
                                       const std::map<std::string, long>* ml = 0,
                                       const std::map<std::string, bool>* mb = 0)

        { encode_private(to_iterator(k), m, ms, md, ml, mb, Message::FROM_MOOS_VAR); }

        /// \brief Decode a message using formatting specified in <publish/> tags.
        ///
        /// Values will be recieved in two maps, one of strings and the other of doubles. The <publish> value will be placed
        /// either based on the "type" parameter of the <moos_var> tag or missing that, on the best guess (numeric values will generally) be
        /// returned in the double map, not the string map).
        ///
        /// \param k can either be std::string (the name of the message) or unsigned (the id of the message)
        /// \param Msg micromodem::Message or std::string to be decode.
        /// \param ms pointer to std::multimap of message variable name to std::string values.
        /// \param md pointer to std::multimap of message variable name to double values.
        template<typename Key, typename Msg>
            void decode_to_publish(const Key& k,
                                   const Msg& m,
                                   std::multimap<std::string, std::string>* ms,
                                   std::multimap<std::string, double>* md,
                                   std::multimap<std::string, long>* ml = 0,
                                   std::multimap<std::string, bool>* mb = 0)
        { decode_private(to_iterator(k), m, ms, md, ml, mb, Message::DO_PUBLISHES); }

        /// \brief Decode a message using formatting specified in <publish/> tags.
        ///
        /// Values will be recieved in two maps, one of strings and the other of doubles. The <publish> value will be placed
        /// either based on the "type" parameter of the <moos_var> tag or missing that, on the best guess (numeric values will generally) be
        /// returned in the double map, not the string map).
        ///
        /// \param k can either be std::string (the name of the message) or unsigned (the id of the message)
        /// \param Msg micromodem::Message or std::string to be decode.
        /// \param ms pointer to std::map of message variable name to std::string values.
        /// \param md pointer to std::map of message variable name to double values.
        template<typename Key, typename Msg>
            void decode_to_publish(const Key& k,
                                   const Msg& m,
                                   std::map<std::string, std::string>* ms,
                                   std::map<std::string, double>* md,
                                   std::map<std::string, long>* ml = 0,
                                   std::map<std::string, bool>* mb = 0)
        { decode_private(to_iterator(k), m, ms, md, ml, mb, Message::DO_PUBLISHES); }


        
        /// what moos variables do i need to provide to create a message with a call to encode_using_src_vars
        template<typename Key>
            std::set<std::string> src_vars(const Key& k)
            { return src_vars(to_iterator(k)); }

        /// for a given message name, all MOOS variables (sources, input, destination, trigger)
        template<typename Key>
            std::set<std::string> all_vars(const Key& k)
            { return all_vars(to_iterator(k)); }

        /// all MOOS variables needed for encoding (includes trigger)
        template<typename Key>
            std::set<std::string> encode_vars(const Key& k)
        { return encode_vars(to_iterator(k)); }

        /// for a given message name, all MOOS variables for decoding (input)
        template<typename Key>
            std::set<std::string> decode_vars(const Key& k)
        { return decode_vars(to_iterator(k)); }

        /// what is the syntax of the message i need to provide?
        template<typename Key>
            std::string input_summary(const Key& k)
        { return input_summary(to_iterator(k)); }
        
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
        
      private:
        std::vector<Message>::const_iterator to_iterator(const std::string& message_name) const
        {
            return name2messages_.count(message_name)
                ? messages_.begin() + name2messages_.find(message_name)->second
                : messages_.end();
        }
        std::vector<Message>::iterator to_iterator(const std::string& message_name)
        {
            return name2messages_.count(message_name)
                ? messages_.begin() + name2messages_.find(message_name)->second
                : messages_.end();
        }
        std::vector<Message>::const_iterator to_iterator(const unsigned& id) const
        {
            return id2messages_.count(id)
                ? messages_.begin() + id2messages_.find(id)->second
                : messages_.end();
        }

        std::vector<Message>::iterator to_iterator(const unsigned& id)
        {
            return id2messages_.count(id)
                ? messages_.begin() + id2messages_.find(id)->second
                : messages_.end();    
        }


        void encode_private(std::vector<Message>::iterator it,
                    micromodem::Message& out_message,
                    const std::map<std::string, std::string>* in_str,
                    const std::map<std::string, double>* in_dbl,
                    const std::map<std::string, long>* in_long,
                    const std::map<std::string, bool>* in_bool,
                    bool from_moos = false)
        {
            if(it != messages_.end())
            {
                it->encode(out_message, in_str, in_dbl, in_long, in_bool, from_moos);
                ++(*it);
            }
            else
            {
                throw std::runtime_error(std::string("DCCL: attempted `encode` on message which is not loaded"));
            }
        }
        
        void encode_private(std::vector<Message>::iterator it,
                    std::string& out_hex,
                    const std::map<std::string, std::string>* in_str,
                    const std::map<std::string, double>* in_dbl,
                    const std::map<std::string, long>* in_long,
                    const std::map<std::string, bool>* in_bool,
                    bool from_moos = false)
        {
            micromodem::Message out_message;
            encode_private(it, out_message, in_str, in_dbl, in_long, in_bool, from_moos);
            out_hex = out_message.serialize(true);
        }
        
        template <typename Map1, typename Map2, typename Map3, typename Map4>
            void decode_private(std::vector<Message>::iterator it,
                        const micromodem::Message& in_message,
                        Map1* out_str,
                        Map2* out_dbl,
                        Map3* out_long,
                        Map4* out_bool,
                        bool do_publishes = false)
        {
            if(it != messages_.end())
                it->decode(in_message, out_str, out_dbl, out_long, out_bool, do_publishes);
            else
                throw std::runtime_error(std::string("DCCL: attempted `decode` on message which is not loaded"));
        }

        template <typename Map1, typename Map2, typename Map3, typename Map4>
            void decode_private(std::vector<Message>::iterator it,
                                const std::string& in_hex,
                                Map1* out_str,
                                Map2* out_dbl,
                                Map3* out_long,
                                Map4* out_bool,
                                bool do_publishes = false)
        {
            micromodem::Message in_message(in_hex);
            decode_private(it, in_message, out_str, out_dbl, out_long, out_bool, do_publishes);
        }

        std::map<std::string, std::string> message_var_names(const std::vector<Message>::const_iterator it) const;        
	std::string summary(const std::vector<Message>::const_iterator it) const;         
        std::string brief_summary(const std::vector<Message>::const_iterator it) const;
        std::set<std::string> src_vars(std::vector<Message>::iterator it);
        std::set<std::string> all_vars(std::vector<Message>::iterator it);
        std::set<std::string> encode_vars(std::vector<Message>::iterator it);
        std::set<std::string> decode_vars(std::vector<Message>::iterator it);
        std::string input_summary(std::vector<Message>::iterator it);

        void check_duplicates();
        
      private:
        std::vector<Message> messages_;
        std::map<std::string, size_t>  name2messages_;
        std::map<unsigned, size_t>     id2messages_;

        std::string xml_schema_;
        time_t start_time_;
        
    };

    /// outputs information about all available messages (same as std::string summary())
    std::ostream& operator<< (std::ostream& out, const DCCLCodec& d);
    
    /// use this for displaying a human readable version of this STL object
    std::ostream& operator<< (std::ostream& out, const std::map<std::string, double>& m);
    /// use this for displaying a human readable version of this STL object
    std::ostream& operator<< (std::ostream& out, const std::map<std::string, std::string>& m);
   /// use this for displaying a human readable version of this STL object
    std::ostream& operator<< (std::ostream& out, const std::map<std::string, long>& m);
    /// use this for displaying a human readable version of this STL object
    std::ostream& operator<< (std::ostream& out, const std::map<std::string, bool>& m);
    /// use this for displaying a human readable version of this STL object
    std::ostream& operator<< (std::ostream& out, const std::set<unsigned>& s);
    /// use this for displaying a human readable version of this STL object
    std::ostream& operator<< (std::ostream& out, const std::set<std::string>& s);

}



#endif
