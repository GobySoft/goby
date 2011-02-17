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

#ifndef MESSAGE_VAR_HEAD20100317H
#define MESSAGE_VAR_HEAD20100317H

#include "message_var.h"

#include "goby/util/time.h"
#include "goby/util/sci.h"

namespace goby
{
    namespace acomms
    {    
        class DCCLMessageVarHead : public DCCLMessageVarInt
        {
          public:
          DCCLMessageVarHead(const std::string& default_name, int bit_size)
              : DCCLMessageVarInt(pow(2, bit_size)-1, 0),
                bit_size_(bit_size),
                default_name_(default_name)
                { set_name(default_name); }

            int calc_size() const
            { return bit_size_; }        

            int calc_total_size() const
            { return bit_size_; }
            
        
          private:
            void initialize_specific()
            {
                if(default_name_ == name_) source_var_.clear();
            }

            virtual void set_defaults_specific(DCCLMessageVal& v, unsigned modem_id, unsigned id)
            { }
        
            virtual boost::dynamic_bitset<unsigned char> encode_specific(const DCCLMessageVal& v)
            { return boost::dynamic_bitset<unsigned char>(calc_size(), long(v)); }
        
            virtual DCCLMessageVal decode_specific(boost::dynamic_bitset<unsigned char>& b)
            { return DCCLMessageVal(long(b.to_ulong())); }
        
          protected:
            int bit_size_;
            std::string default_name_;
        };
    
        class DCCLMessageVarTime : public DCCLMessageVarHead
        {
          public:
          DCCLMessageVarTime():
            DCCLMessageVarHead(acomms::DCCL_HEADER_NAMES[acomms::HEAD_TIME],
                               acomms::HEAD_TIME_SIZE)
            { }

            void set_defaults_specific(DCCLMessageVal& v, unsigned modem_id, unsigned id)
            {
                v = (!v.empty()) ? v : DCCLMessageVal(util::ptime2unix_double(util::goby_time()));
            }
        
            boost::dynamic_bitset<unsigned char> encode_specific(const DCCLMessageVal& v)
            {
                // trim to time of day
                boost::posix_time::time_duration time_of_day = util::unix_double2ptime(v).time_of_day();
                
                return boost::dynamic_bitset<unsigned char>(calc_size(), long(util::unbiased_round(util::time_duration2double(time_of_day), 0)));    
            }
        
            DCCLMessageVal decode_specific(boost::dynamic_bitset<unsigned char>& b)
            {
                // add the date back in (assumes message sent the same day received)
                DCCLMessageVal v(long(b.to_ulong()));

                using namespace boost::posix_time;
                using namespace boost::gregorian;
            
                ptime now = util::goby_time();
                date day_sent;

                // this logic will break if there is a separation between message sending and
                // message receipt of greater than 1/2 day (twelve hours)
            
                // if message is from part of the day removed from us by 12 hours, we assume it
                // was sent yesterday
                if(abs(now.time_of_day().total_seconds() - double(v)) > hours(12).total_seconds())
                    day_sent = now.date() - days(1);
                else // otherwise figure it was sent today
                    day_sent = now.date();
            
                v.set(double(v) + util::ptime2unix_double(ptime(day_sent,seconds(0))));
                return v;
            }
        };

        class DCCLMessageVarCCLID : public DCCLMessageVarHead
        {
          public:
          DCCLMessageVarCCLID():
            DCCLMessageVarHead(acomms::DCCL_HEADER_NAMES[acomms::HEAD_CCL_ID],
                               acomms::HEAD_CCL_ID_SIZE) { }        

            void set_defaults_specific(DCCLMessageVal& v, unsigned modem_id, unsigned id)
            {
                v = long(acomms::DCCL_CCL_HEADER);
            }        
        };
    
        class DCCLMessageVarDCCLID : public DCCLMessageVarHead
        {
          public:
          DCCLMessageVarDCCLID()
              : DCCLMessageVarHead(acomms::DCCL_HEADER_NAMES[acomms::HEAD_DCCL_ID],
                                   acomms::HEAD_DCCL_ID_SIZE)
            { }
        
            void set_defaults_specific(DCCLMessageVal& v, unsigned modem_id, unsigned id)
            {
                v = (!v.empty()) ? v : DCCLMessageVal(long(id));
            }
        };

        class DCCLMessageVarSrc : public DCCLMessageVarHead
        {
          public:
          DCCLMessageVarSrc()
              : DCCLMessageVarHead(acomms::DCCL_HEADER_NAMES[acomms::HEAD_SRC_ID],
                                   acomms::HEAD_SRC_ID_SIZE)
            { }
        
            void set_defaults_specific(DCCLMessageVal& v, unsigned modem_id, unsigned id)
            {
                v = (!v.empty()) ? v : DCCLMessageVal(long(modem_id));
            }
        };

        class DCCLMessageVarDest : public DCCLMessageVarHead
        {
          public:
          DCCLMessageVarDest():
            DCCLMessageVarHead(acomms::DCCL_HEADER_NAMES[acomms::HEAD_DEST_ID],
                               acomms::HEAD_DEST_ID_SIZE) { }

            void set_defaults_specific(DCCLMessageVal& v, unsigned modem_id, unsigned id)
            {
                v = (!v.empty()) ? v : DCCLMessageVal(long(acomms::BROADCAST_ID));
            }
        };

        class DCCLMessageVarMultiMessageFlag : public DCCLMessageVarHead
        {
          public:
          DCCLMessageVarMultiMessageFlag():
            DCCLMessageVarHead(acomms::DCCL_HEADER_NAMES[acomms::HEAD_MULTIMESSAGE_FLAG],
                               acomms::HEAD_FLAG_SIZE) { }
        
            boost::dynamic_bitset<unsigned char> encode_specific(const DCCLMessageVal& v)
            { return boost::dynamic_bitset<unsigned char>(calc_size(), bool(v)); }
        
            DCCLMessageVal decode_specific(boost::dynamic_bitset<unsigned char>& b)
            { return DCCLMessageVal(bool(b.to_ulong())); }        

        };
    
        class DCCLMessageVarBroadcastFlag : public DCCLMessageVarHead
        {
          public:
          DCCLMessageVarBroadcastFlag():
            DCCLMessageVarHead(acomms::DCCL_HEADER_NAMES[acomms::HEAD_BROADCAST_FLAG],
                               acomms::HEAD_FLAG_SIZE) { }

            boost::dynamic_bitset<unsigned char> encode_specific(const DCCLMessageVal& v)
            { return boost::dynamic_bitset<unsigned char>(calc_size(), bool(v)); }
        
            DCCLMessageVal decode_specific(boost::dynamic_bitset<unsigned char>& b)
            { return DCCLMessageVal(bool(b.to_ulong())); }        

        };

        class DCCLMessageVarUnused : public DCCLMessageVarHead
        {
          public:
          DCCLMessageVarUnused():
            DCCLMessageVarHead(acomms::DCCL_HEADER_NAMES[acomms::HEAD_UNUSED],
                               acomms::HEAD_UNUSED_SIZE) { }


        };

    }
}
#endif
