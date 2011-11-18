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

#include <set>
#include <map>
#include <string>
#include "MOOSLIB/MOOSMsg.h"

#include "goby/moos/moos_protobuf_helpers.h"
#include "goby/moos/protobuf/translator.pb.h"
#include "goby/util/dynamic_protobuf_manager.h"


namespace goby
{
    namespace moos
    {
        class MOOSTranslator
        {
          public:
            MOOSTranslator() { }
            MOOSTranslator(const goby::moos::protobuf::TranslatorEntry& entry)
            { add_entry(entry); }
            MOOSTranslator(const std::set<goby::moos::protobuf::TranslatorEntry>& entries)
            { add_entry(entries); }

            void add_entry(const goby::moos::protobuf::TranslatorEntry& entry)
            { dictionary_[entry.protobuf_name()] =  entry; }
            
            void add_entry(const std::set<goby::moos::protobuf::TranslatorEntry>& entries)
            {
                for(std::set<goby::moos::protobuf::TranslatorEntry>::const_iterator it = entries.begin(),
                        n = entries.end();
                    it != n;
                    ++it)
                { add_entry(*it); }
            }            
            
            // ownership of returned pointer goes to caller (must use smart pointer or call delete)
            template<typename GoogleProtobufMessagePointer> //, template <typename, typename> class Map >
                GoogleProtobufMessagePointer moos_to_protobuf(const std::multimap<std::string, CMOOSMsg>& moos_variables, const std::string& protobuf_name);
            
            std::multimap<std::string, CMOOSMsg> protobuf_to_moos(const google::protobuf::Message& protobuf_msg);

            const std::map<std::string, goby::moos::protobuf::TranslatorEntry>&  dictionary() const { return dictionary_; }
            
          private:
            std::map<std::string, goby::moos::protobuf::TranslatorEntry> dictionary_;
        };

        inline std::ostream& operator<<(std::ostream& os, const MOOSTranslator& tl)
        {
            os << "= Begin MOOSTranslator =\n";

            int i = 0;
            for(std::map<std::string, goby::moos::protobuf::TranslatorEntry>::const_iterator it = tl.dictionary().begin(),
                    n = tl.dictionary().end();
                    it != n;
                    ++it)
            {
                os << "== Begin Entry " << i << " ==\n"
                   << it->second.DebugString()
                   << "== End Entry " << i << " ==\n";
                
                ++i;
            }
            
            os << "= End MOOSTranslator =";
            return os;
        }

        namespace protobuf
        {
            bool operator<(const protobuf::TranslatorEntry& a, const protobuf::TranslatorEntry& b)
            {
                return a.protobuf_name() < b.protobuf_name();
            }
        }
        

    }
}



inline std::multimap<std::string, CMOOSMsg> goby::moos::MOOSTranslator::protobuf_to_moos(const google::protobuf::Message& protobuf_msg)
{
    std::map<std::string, goby::moos::protobuf::TranslatorEntry>::const_iterator it =
        dictionary_.find(protobuf_msg.GetDescriptor()->full_name());

    if(it == dictionary_.end())
        throw(std::runtime_error("No TranslatorEntry for Protobuf type: " + protobuf_msg.GetDescriptor()->full_name()));

    const goby::moos::protobuf::TranslatorEntry& entry = it->second;
    
    std::multimap<std::string, CMOOSMsg> moos_msgs;

    for(int i = 0, n = entry.publish_size();
        i < n; ++ i)
    {
        std::string return_string;
        switch(entry.publish(i).technique())
        {
            case protobuf::TranslatorEntry::TECHNIQUE_PROTOBUF_TEXT_FORMAT:
                serialize_for_moos(&return_string, protobuf_msg);
                break;
                
            case protobuf::TranslatorEntry::TECHNIQUE_PROTOBUF_NATIVE_ENCODED:
                protobuf_msg.SerializeToString(&return_string);
                break;

            case protobuf::TranslatorEntry::TECHNIQUE_COMMA_SEPARATED_KEY_EQUALS_VALUE_PAIRS:
                to_moos_comma_equals_string(protobuf_msg, &return_string);
                break;

            case protobuf::TranslatorEntry::TECHNIQUE_FORMAT:
                break;
        }
        moos_msgs.insert(std::make_pair(entry.publish(i).moos_var(),
                                        CMOOSMsg(MOOS_STRING, entry.publish(i).moos_var(), return_string)));
    }
    
    return moos_msgs;
}

template<typename GoogleProtobufMessagePointer>//, template <typename, typename> class Map >
    GoogleProtobufMessagePointer goby::moos::MOOSTranslator::moos_to_protobuf(const std::multimap<std::string, CMOOSMsg>& moos_variables, const std::string& protobuf_name)
{
    std::map<std::string, goby::moos::protobuf::TranslatorEntry>::const_iterator it =
        dictionary_.find(protobuf_name);
    
    if(it == dictionary_.end())
        throw(std::runtime_error("No TranslatorEntry for Protobuf type: " + protobuf_name));

    const goby::moos::protobuf::TranslatorEntry& entry = it->second;

    
    GoogleProtobufMessagePointer msg =
        goby::util::DynamicProtobufManager::new_protobuf_message<GoogleProtobufMessagePointer>(protobuf_name);

    if(&*msg == 0)
        throw(std::runtime_error("Unknown Protobuf type: " + protobuf_name + "; be sure it is compiled in or directly loaded into the goby::util::DynamicProtobufManager."));

    
    for(int i = 0, n = entry.create_size();
        i < n; ++ i)
    {
        std::multimap<std::string, CMOOSMsg>::const_iterator it = moos_variables.find(entry.create(i).moos_var());
        std::string source_string = (it == moos_variables.end()) ? "" : it->second.GetString();
            
        switch(entry.create(i).technique())
        {
            case protobuf::TranslatorEntry::TECHNIQUE_PROTOBUF_TEXT_FORMAT:
                parse_for_moos(source_string, &*msg);
                break;
                
            case protobuf::TranslatorEntry::TECHNIQUE_PROTOBUF_NATIVE_ENCODED:
                msg->ParseFromString(source_string);
                break;

            case protobuf::TranslatorEntry::TECHNIQUE_COMMA_SEPARATED_KEY_EQUALS_VALUE_PAIRS:
                from_moos_comma_equals_string(&*msg, source_string);
                break;

            case protobuf::TranslatorEntry::TECHNIQUE_FORMAT:
                break;
        }
    }

    return msg;
}


