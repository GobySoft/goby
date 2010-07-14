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

#include "goby/util/gtime.h"
#include "goby/util/sci.h"
namespace goby
{
namespace dccl
{    
    class MessageVarHead : public MessageVarInt
    {
      public:
      MessageVarHead(const std::string& default_name, int bit_size)
          : MessageVarInt(pow(2, bit_size)-1, 0),
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

        virtual void set_defaults_specific(MessageVal& v, unsigned modem_id, unsigned id)
        { }
        
        virtual boost::dynamic_bitset<unsigned char> encode_specific(const MessageVal& v)
        { return boost::dynamic_bitset<unsigned char>(calc_size(), long(v)); }
        
        virtual MessageVal decode_specific(boost::dynamic_bitset<unsigned char>& b)
        { return MessageVal(long(b.to_ulong())); }
        
      protected:
        int bit_size_;
        std::string default_name_;
    };
    
    class MessageVarTime : public MessageVarHead
    {
      public:
      MessageVarTime():
        MessageVarHead(acomms::DCCL_HEADER_NAMES[acomms::head_time],
                       acomms::head_time_size)
        { }

        void set_defaults_specific(MessageVal& v, unsigned modem_id, unsigned id)
        {
            double d;
            v = (!v.empty() && v.get(d)) ? v : MessageVal(gtime::ptime2unix_double(gtime::now()));
        }
        
        boost::dynamic_bitset<unsigned char> encode_specific(const MessageVal& v)
        {
            // trim to time of day
            boost::posix_time::time_duration time_of_day = gtime::unix_double2ptime(v).time_of_day();
            
            return boost::dynamic_bitset<unsigned char>(calc_size(), long(sci::unbiased_round(gtime::time_duration2double(time_of_day), 0)));    
        }
        
        MessageVal decode_specific(boost::dynamic_bitset<unsigned char>& b)
        {
            // add the date back in (assumes message sent the same day received)
            MessageVal v(long(b.to_ulong()));

            using namespace boost::posix_time;
            using namespace boost::gregorian;
            
            ptime now = gtime::now();
            date day_sent;

            // this logic will break if there is a separation between message sending and
            // message receipt of greater than 1/2 day (twelve hours)
            
            // if message is from part of the day removed from us by 12 hours, we assume it
            // was sent yesterday
            if(abs(now.time_of_day().total_seconds() - double(v)) > hours(12).total_seconds())
                day_sent = now.date() - days(1);
            else // otherwise figure it was sent today
                day_sent = now.date();
            
            v.set(double(v) + gtime::ptime2unix_double(ptime(day_sent,seconds(0))));
            return v;
        }
    };

    class MessageVarCCLID : public MessageVarHead
    {
      public:
      MessageVarCCLID():
        MessageVarHead(acomms::DCCL_HEADER_NAMES[acomms::head_ccl_id],
               acomms::head_ccl_id_size) { }        

        void set_defaults_specific(MessageVal& v, unsigned modem_id, unsigned id)
        {
            v = long(acomms::DCCL_CCL_HEADER);
        }        
    };
    
    class MessageVarDCCLID : public MessageVarHead
    {
      public:
      MessageVarDCCLID()
          : MessageVarHead(acomms::DCCL_HEADER_NAMES[acomms::head_dccl_id],
                           acomms::head_dccl_id_size)
        { }
        
        void set_defaults_specific(MessageVal& v, unsigned modem_id, unsigned id)
        {
            long l;
            v = (!v.empty() && v.get(l)) ? v : MessageVal(long(id));
        }
    };

    class MessageVarSrc : public MessageVarHead
    {
      public:
      MessageVarSrc()
          : MessageVarHead(acomms::DCCL_HEADER_NAMES[acomms::head_src_id],
                           acomms::head_src_id_size)
            { }
        
        void set_defaults_specific(MessageVal& v, unsigned modem_id, unsigned id)
        {
            long l;
            v = (!v.empty() && v.get(l)) ? v : MessageVal(long(modem_id));
        }        
    };

    class MessageVarDest : public MessageVarHead
    {
      public:
      MessageVarDest():
        MessageVarHead(acomms::DCCL_HEADER_NAMES[acomms::head_dest_id],
                       acomms::head_dest_id_size) { }

        void set_defaults_specific(MessageVal& v, unsigned modem_id, unsigned id)
        {
            long l;
            v = (!v.empty() && v.get(l)) ? v : MessageVal(long(acomms::BROADCAST_ID));
        }
    };

    class MessageVarMultiMessageFlag : public MessageVarHead
    {
      public:
      MessageVarMultiMessageFlag():
        MessageVarHead(acomms::DCCL_HEADER_NAMES[acomms::head_multimessage_flag],
                       acomms::head_flag_size) { }
        
        boost::dynamic_bitset<unsigned char> encode_specific(const MessageVal& v)
        { return boost::dynamic_bitset<unsigned char>(calc_size(), bool(v)); }
        
        MessageVal decode_specific(boost::dynamic_bitset<unsigned char>& b)
        { return MessageVal(bool(b.to_ulong())); }        

    };
    
    class MessageVarBroadcastFlag : public MessageVarHead
    {
      public:
      MessageVarBroadcastFlag():
        MessageVarHead(acomms::DCCL_HEADER_NAMES[acomms::head_broadcast_flag],
                       acomms::head_flag_size) { }

        boost::dynamic_bitset<unsigned char> encode_specific(const MessageVal& v)
        { return boost::dynamic_bitset<unsigned char>(calc_size(), bool(v)); }
        
        MessageVal decode_specific(boost::dynamic_bitset<unsigned char>& b)
        { return MessageVal(bool(b.to_ulong())); }        

    };

    class MessageVarUnused : public MessageVarHead
    {
      public:
      MessageVarUnused():
        MessageVarHead(acomms::DCCL_HEADER_NAMES[acomms::head_unused],
                       acomms::head_unused_size) { }


    };

}
}
#endif
