
#ifndef NMEASentence20091211H
#define NMEASentence20091211H

#include <exception>
#include <stdexcept>
#include <vector>
#include <sstream>

#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>

#include "goby/util/as.h"

namespace goby
{
    namespace util
    {
        // simple exception class
        class bad_nmea_sentence : public std::runtime_error
        {
          public:
            bad_nmea_sentence(const std::string& s)
                : std::runtime_error(s)
            { }
        };
        
        
        class NMEASentence : public std::vector<std::string>
        {
          public:
            enum strategy { IGNORE, VALIDATE, REQUIRE };
            
            NMEASentence() {}
            NMEASentence(std::string s, strategy cs_strat = VALIDATE);

            // Bare message, no checksum or \r\n
            std::string message_no_cs() const;

            // Includes checksum, but no \r\n
            std::string message() const;

            // Includes checksum and \r\n
            std::string message_cr_nl() const { return message() + "\r\n"; }

            // first two talker (CC)
            std::string talker_id() const 
            { return empty() ? "" : front().substr(1, 2); }

            // last three (CFG)
            std::string sentence_id() const
            { return empty() ? "" : front().substr(3); }

            template<typename T>
                T as(int i) const { return goby::util::as<T>(at(i)); }
            
            template<typename T>
                void push_back(T t)
            { push_back(goby::util::as<std::string>(t)); }
            
            // necessary when pushing back string "foo,bar" that contain
            // commas
            void push_back(const std::string& str)
            {
                std::vector<std::string> vec;
                boost::split(vec, str, boost::is_any_of(","));
                
                BOOST_FOREACH(const std::string& s, vec)
                    std::vector<std::string>::push_back(s);
            }
    
            
            static unsigned char checksum(const std::string& s);
        };
    }
}

// overloaded <<
inline std::ostream& operator<< (std::ostream& out, const goby::util::NMEASentence& nmea)
{ out << nmea.message(); return out; }

#endif
