// copyright 2009 t. schneider tes@mit.edu
// ocean engineering graudate student - mit / whoi joint program
// massachusetts institute of technology (mit)
// laboratory for autonomous marine sensing systems (lamss)
// 
// this file is part of goby-acomms, a collection of libraries for acoustic underwater networking
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


#ifndef MODEMMESSAGE_H
#define MODEMMESSAGE_H

#include <ctime>
#include <string>
#include <sstream>
#include <iomanip>

#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>

#include "util/tes_utils.h"
#include "acomms/acomms_constants.h"

/// WHOI Micro-Modem specific objects
namespace micromodem
{
    /// \brief Provides a class that represents a message to or from the WHOI Micro-Modem.
    ///
    /// Message is intended to represent all the possible messages from the modem. Thus, depending
    /// on the specific message certain fields may not be used. Also, fields make take on different
    /// meanings depending on the message type. Where this is the case, extra documentation is provided.
    class Message
    {
      public:

        /// \brief Construct a message for or from the Micro-Modem
        /// \param s (optionally) pass a serialized string such as src=3,dest=4,data=CA342BDF ... to initialize
      Message(const std::string & s = ""):
        data_(""),
            src_(0),
            dest_(0),
            rate_(0),
            t_(time(NULL)),
            ack_(false),
            size_(0),
            frame_(1),
            cs_(0),
            data_set_(false),
            src_set_(false),
            dest_set_(false),
            rate_set_(false),
            t_set_(false),
            ack_set_(false),
            size_set_(false),
            frame_set_(false),
            cs_set_(false)
            { if(!s.empty()) unserialize(s); }

        //@{
        
        /// hexadecimal string
        std::string data() const {return data_;} 
        /// time in seconds since UNIX
        double t() const {return t_;}
        /// size in bytes
        double size() const {return size_;}
        /// source modem id
        unsigned src() const {return src_;}
        /// destination modem id
        unsigned dest() const {return dest_;}
        /// data rate (unsigned from 0 (lowest) to 5 (highest))
        unsigned rate() const { return rate_; }
        /// acknowledgement requested
        bool ack() const {return ack_;}
        /// modem frame number
        unsigned frame() const {return frame_;}
        /// checksum (eight bit XOR of Message::data())
        unsigned cs() const {return cs_;}

        //@}
        
        /// is there data?
        bool data_set() const {return data_set_;} 
        /// is there a time?
        bool t_set() const {return t_set_;}
        /// is there a size?
        bool size_set() const {return size_set_;}
        /// is there a source?
        bool src_set() const {return src_set_;}
        /// is there a destination?
        bool dest_set() const {return dest_set_;}
        /// is there a rate?
        bool rate_set() const { return rate_set_; }
        /// is there an ack value?
        bool ack_set() const {return ack_set_;}
        /// is there a frame number?
        bool frame_set() const {return frame_set_;}
        /// is there a checksum?
        bool cs_set() const {return cs_set_;}

        /// is the Message empty (no data)?
        bool empty() const {return !data_set_;}

        /// short snippet summarizing the Message
        std::string snip() const 
        {
            std::stringstream ss;
            
            if(src_set_)   ss << " | src " << src_;
            if(dest_set_)  ss << " | dest " << dest_;
            if(rate_set_)  ss << " | rate " << rate_;
            if(size_set_)  ss << " | size " << size_ << "B";
            if(t_set_)     ss << " | age " << tes_util::sci_round(time(NULL) - t_, 0) << "s";
            if(ack_set_)   ss << " | ack " << std::boolalpha << ack_;
            if(frame_set_) ss << " | frame " << frame_;
            if(cs_set_)    ss << " | *" << std::hex << std::setw(2) << std::setfill('0') << (int)cs_;
            
            if(!ss.str().empty())
                return ss.str().substr(3);
            else
                return "null message";     
        }

        /// full human readable string serialization
        enum { COMPRESS = true, NO_COMPRESS = false };
        std::string serialize(bool compress = NO_COMPRESS) const 
        {
            std::stringstream ss;

            if(dest_ == acomms_util::BROADCAST_ID && compress)
                ss << "," << data_;
            else
            {       
                if(src_set_)   ss << ",src=" << src_;
                if(dest_set_)  ss << ",dest=" << dest_;
                if(rate_set_)  ss << ",rate=" << rate_;
                if(data_set_)  ss << ",data=" << data_;
                if(size_set_)  ss << ",size=" << size_;
                if(t_set_)     ss << ",time=" << t_;
                if(ack_set_)   ss << ",ack=" << std::boolalpha << ack_;
                if(frame_set_) ss << ",frame=" << frame_;
                if(cs_set_)    ss << ",cs=" << std::hex << std::setw(2) << std::setfill('0') << (int)cs_;
            }
        
            if(!ss.str().empty())
                return ss.str().substr(1);
            else
                return "null_message";            
        }    

        /// reverse serialization
        void unserialize(const std::string & s) 
        {
            std::string value;
            std::string lower_s = boost::to_lower_copy(s);

            // case where whole string is just hex
            if(check_hex(lower_s))
                set_data(lower_s);
            else
            {
                if(tes_util::val_from_string(value, lower_s, "src"))
                    set_src(value);
                if(tes_util::val_from_string(value, lower_s, "dest"))
                    set_dest(value);
                if(tes_util::val_from_string(value, lower_s, "rate"))
                    set_rate(value);
                if((tes_util::val_from_string(value, lower_s, "data") || tes_util::val_from_string(value, lower_s, "hexdata")) && check_hex(value))
                    set_data(value);
                if(tes_util::val_from_string(value, lower_s, "time"))
                    set_t(value);
                if(tes_util::val_from_string(value, lower_s, "ack"))
                    set_ack(value);
                if(tes_util::val_from_string(value, lower_s, "frame"))
                    set_frame(value);
            }
        
        }    
    
        /// set the hexadecimal string data
        void set_data(const std::string & data)
        {
            data_ = data;
            calc_cs();
            set_size((double)(data.length()/2));
            data_set_ = true;
        }
        /// set the time
        void set_t(double d)       { t_=d; t_set_ = true; }    
        /// set the size (automatically set by the data size)
        void set_size(double size) { size_= size; size_set_ = true;}    
        /// set the source id
        void set_src(unsigned src)      { src_=src; src_set_ = true; }   
        /// set the destination id
        void set_dest(unsigned dest)    { dest_=dest; dest_set_ = true; }
        /// set the data rate
        void set_rate(unsigned rate)      { rate_=rate; rate_set_ = true; }
        /// set the acknowledgement value
        void set_ack(bool ack)     { ack_=ack; ack_set_ = true; }
        /// set the frame number
        void set_frame(unsigned frame)  { frame_=frame; frame_set_ = true; }

        /// try to set the time with std::string
        void set_t(const std::string & t)
        {
            try { set_t(boost::lexical_cast<double>(t)); }
            catch(boost::bad_lexical_cast & ) { }
        }
        /// try to set the size with std::string
         void set_size(const std::string& size)
        {
            try {  set_size(boost::lexical_cast<double>(size)); }
            catch(boost::bad_lexical_cast & ) { }
        }
        /// try to set the destination id with std::string
         void set_dest(const std::string& dest)   {
            try { set_dest(boost::lexical_cast<unsigned>(dest)); }
            catch(boost::bad_lexical_cast & ) { }
        }
        /// try to set the source id with std::string
         void set_src(const std::string& src)
        {
            try { set_src(boost::lexical_cast<unsigned>(src)); }
            catch(boost::bad_lexical_cast & ) { }
        }
        /// try to set the source id with std::string
         void set_rate(const std::string& rate)
        {
            try { set_rate(boost::lexical_cast<unsigned>(rate)); }
            catch(boost::bad_lexical_cast & ) { }
        }
        /// try to set the acknowledgement value with string ("true" / "false")
        void set_ack(const std::string & ack)
        {
            try { set_ack(tes_util::string2bool(ack)); }
            catch(boost::bad_lexical_cast & ) { }
        }
        /// try to set the frame number with std::string
         void set_frame(const std::string& frame) {
            try { set_frame(boost::lexical_cast<unsigned>(frame));}
            catch(boost::bad_lexical_cast & ) { }
        }

        /// removes the header bytes (CCL and DCCL ids) from the data
        void remove_header()
        {
            try
            {
                data_.erase(0, acomms_util::NUM_HEADER_BYTES*acomms_util::NIBS_IN_BYTE);
                calc_cs();
                set_size((double)(data_.length()/2));
            }
            catch (...) { }
        }
        
        /// string replace inside the hexadecimal data
        void replace_in_data(std::string::size_type start, std::string::size_type length, const std::string & value)
        {
            data_.replace(start, length, value);
            calc_cs();
            set_size((double)(data_.length()/2));
        }
        
        
      private:
        void calc_cs()
        {
            cs_ = 0;
            for(std::string::size_type i = 0, n = data_.length(); i < n; ++i)
                cs_ ^= data_[i];

            cs_set_ = true;
        }    

        bool check_hex(std::string line)
        {
            for (unsigned i = 0; i < line.size(); i++)
            { if(!isxdigit(line[i])) return false; }
            return true;
        }

    
      private:
        std::string data_;
        unsigned src_;    
        unsigned dest_;  //modem_id
        unsigned rate_;
        double t_;
        bool ack_;
        double size_; //in bytes
        unsigned frame_;
        unsigned char cs_;

        bool data_set_;
        bool src_set_;
        bool dest_set_;
        bool rate_set_;
        bool t_set_;
        bool ack_set_;
        bool size_set_;
        bool frame_set_;
        bool cs_set_;    
    };

}

/// STL streams overloaded output operator (for std::cout, etc.)
inline std::ostream & operator<< (std::ostream & out, const micromodem::Message & message)
{ out << message.snip(); return out; }


#endif
