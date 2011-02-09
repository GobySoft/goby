// copyright 2009 t. schneider tes@mit.edu
// 
// this file is part of flex-ostream, a terminal display library
// that provides an ostream with both terminal display and file logging
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

#ifndef FlexOstream20091211H
#define FlexOstream20091211H

#include <iostream>
#include <string>

#include "flex_ostreambuf.h"
#include "logger_manipulators.h"

#include <google/protobuf/text_format.h>
#include <google/protobuf/io/tokenizer.h>

namespace goby
{
    namespace util
    {
        namespace logger_lock
        {
            /// Mutex actions available to the Goby logger (glogger)
            enum LockAction { none, lock };
        }

        /// Forms the basis of the Goby logger: std::ostream derived class for holding the FlexOStreamBuf
        class FlexOstream : public std::ostream
        {
          public:
            /// \name Initialization
            //@{
            /// Add another group to the logger. A group provides related manipulator for categorizing log messages.
            /// For thread safe use use boost::scoped_lock on Logger::mutex
            void add_group(const std::string & name,
                           Colors::Color color = Colors::nocolor,
                           const std::string & description = "");
            
            /// Set the name of the application that the logger is serving.
            void set_name(const std::string & s)
            {
                sb_.name(s);
            }

            void add_stream(const std::string& verbosity, std::ostream* os = 0)
            {
                if(verbosity == "scope" || verbosity == "gui")        
                    add_stream(goby::util::Logger::gui, os);
                else if(verbosity == "quiet")        
                    add_stream(goby::util::Logger::quiet, os);
                else if(verbosity == "terse" || verbosity == "warn")        
                    add_stream(goby::util::Logger::warn, os);
                else if(verbosity == "debug")        
                    add_stream(goby::util::Logger::debug, os);
                else
                    add_stream(goby::util::Logger::verbose, os);
            }
            
            
            /// Attach a stream object (e.g. std::cout, std::ofstream, ...) to the logger with desired verbosity
            void add_stream(Logger::Verbosity verbosity = Logger::verbose, std::ostream* os = 0)
            {
                sb_.add_stream(verbosity, os);
            }            
            //@}

            /// \name Overloaded insert stream operator<<
            //@{
            // overload this function so we can look for the die manipulator
            // and set the die_flag
            // to exit after this line
            std::ostream& operator<<(FlexOstream& (*pf) (FlexOstream&));
            std::ostream& operator<<(std::ostream & (*pf) (std::ostream &));
            
            //provide interfaces to the rest of the types
            std::ostream& operator<< (bool& val )
            { return std::ostream::operator<<(val); }
            std::ostream& operator<< (const short& val )
            { return std::ostream::operator<<(val); }
            std::ostream& operator<< (const unsigned short& val )
            { return std::ostream::operator<<(val); }
            std::ostream& operator<< (const int& val )
            { return std::ostream::operator<<(val); }
            std::ostream& operator<< (const unsigned int& val )
            { return std::ostream::operator<<(val); }
            std::ostream& operator<< (const long& val )
            { return std::ostream::operator<<(val); }
            std::ostream& operator<< (const unsigned long& val )
            { return std::ostream::operator<<(val); }
            std::ostream& operator<< (const float& val )
            { return std::ostream::operator<<(val); }
            std::ostream& operator<< (const double& val )
            { return std::ostream::operator<<(val); }
            std::ostream& operator<< (const long double& val )
            { return std::ostream::operator<<(val); }
            std::ostream& operator<< (std::streambuf* sb )
            { return std::ostream::operator<<(sb); }
            std::ostream& operator<< (std::ios& ( *pf )(std::ios&))
            { return std::ostream::operator<<(pf); }
            std::ostream& operator<< (std::ios_base& ( *pf )(std::ios_base&))
            { return std::ostream::operator<<(pf); }
            //@}

            /// \name Thread safety related
            //@{
            /// Get a reference to the Goby logger mutex for scoped locking
            boost::mutex& mutex()
            { return Logger::mutex; }
            //@}

            void refresh()
            {
                sb_.refresh();
            }
            void set_group(const std::string & s)
            {
                sb_.group_name(s);
            }            
            
          private:            
            bool quiet()
            { return (sb_.is_quiet()); }
            
            
            friend FlexOstream& glogger(logger_lock::LockAction lock_action);

            friend std::ostream& operator<< (FlexOstream& out, char c );
            friend std::ostream& operator<< (FlexOstream& out, signed char c );
            friend std::ostream& operator<< (FlexOstream& out, unsigned char c );
            friend std::ostream& operator<< (FlexOstream& out, const char *s );
            friend std::ostream& operator<< (FlexOstream& out, const signed char* s );
            friend std::ostream& operator<< (FlexOstream& out, const unsigned char* s );

          private:
          FlexOstream() : std::ostream(&sb_) {}
            ~FlexOstream() { }
            FlexOstream(const FlexOstream&);
            FlexOstream& operator = (const FlexOstream&);            

          private:
            static FlexOstream* inst_;
            FlexOStreamBuf sb_;
        };        
        /// \name Logger
        //@{
        /// \brief Access the Goby logger through this function. 
        /// 
        /// For normal (non thread safe use), do not pass any parameters:
        /// glogger() << "some text" << std::endl;
        ///
        /// To group messages, pass the group(group_name) manipulator, where group_name
        /// is a previously defined group (by call to glogger().add_group(Group)).
        /// For example:
        /// glogger() << group("incoming") << "received message foo" << std::endl;
        ///
        /// For thread safe use, use glogger(lock) and then insert the "unlock" manipulator
        /// when relinquishing the lock. The "unlock" manipulator MUST be inserted before
        /// the next call to glogger(lock). \b Nothing must throw exceptions between
        /// glogger(lock)  and unlock.
        /// For example:
        /// glogger(lock) << "my thread is the best" << std::endl << unlock;
        /// \param lock_action logger_lock::lock to lock access to the logger (for thread safety) or logger_lock::none for no mutex action (typical)
        /// \return reference to Goby logger (std::ostream derived class FlexOstream)
        FlexOstream& glogger(logger_lock::LockAction lock_action = logger_lock::none);
        
        inline std::ostream& operator<< (FlexOstream& out, char c )
        { return std::operator<<(out, c); }
        inline std::ostream& operator<< (FlexOstream& out, signed char c )
        { return std::operator<<(out, c); }
        inline std::ostream& operator<< (FlexOstream& out, unsigned char c )
        { return std::operator<<(out, c); }        
        inline std::ostream& operator<< (FlexOstream& out, const char* s )
        { return std::operator<<(out, s); }        
        inline std::ostream& operator<< (FlexOstream& out, const signed char* s )
        { return std::operator<<(out, s); }        
        inline std::ostream& operator<< (FlexOstream& out, const unsigned char* s )
        { return std::operator<<(out, s); }                    
        //@}

    }
}


/// Unlock the Goby logger after a call to glogger(lock)
inline std::ostream& unlock(std::ostream & os)
{
    goby::util::Logger::mutex.unlock();    
    return os;
}

class FlexOStreamErrorCollector : public google::protobuf::io::ErrorCollector
{
  public:
  FlexOStreamErrorCollector(const std::string& original)
      : original_(original),
        has_warnings_(false),
        has_errors_(false)
        { }
    
    void AddError(int line, int column, const std::string& message)
    {
        print_original(line, column);
        goby::util::glogger() << warn << "line: " << line << " col: " << column << " " << message << std::endl;
        has_errors_ = true;
    }
    void AddWarning(int line, int column, const std::string& message)
    {
        print_original(line, column);
        goby::util::glogger() << warn << "line: " << line << " col: " << column << " " << message << std::endl;
        has_warnings_ = true;
    }
    
    void print_original(int line, int column)
    {
        std::stringstream ss(original_);
        std::string line_str;

        //for(int i = 0; i <= line; ++i)
        //    getline(ss, line_str);

        int i = 0;
        while(!getline(ss, line_str).eof())
        {
            if(i == line)
                goby::util::glogger() << goby::util::tcolor::lt_red << "[line " << std::setw(3) << i++ << "]" << line_str << goby::util::tcolor::nocolor << std::endl;
            else
                goby::util::glogger() << "[line " << std::setw(3) << i++ << "]" << line_str << std::endl;       
        }
    }


    bool has_errors() { return has_errors_; }
    bool has_warnings() { return has_warnings_; }
    
    
  private:
    const std::string& original_;
    bool has_warnings_;
    bool has_errors_;
};


#endif
