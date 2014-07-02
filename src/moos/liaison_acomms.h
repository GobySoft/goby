// Copyright 2009-2014 Toby Schneider (https://launchpad.net/~tes)
//                     GobySoft, LLC (2013-)
//                     Massachusetts Institute of Technology (2007-2014)
//                     Goby Developers Team (https://launchpad.net/~goby-dev)
// 
//
// This file is part of the Goby Underwater Autonomy Project Libraries
// ("The Goby Libraries").
//
// The Goby Libraries are free software: you can redistribute them and/or modify
// them under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 2.1 of the License, or
// (at your option) any later version.
//
// The Goby Libraries are distributed in the hope that they will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with Goby.  If not, see <http://www.gnu.org/licenses/>.



#ifndef LIAISONACOMMS20140626H
#define LIAISONACOMMS20140626H

#include <Wt/WTimer>
#include <Wt/WProgressBar>

#include <dccl.h>

#include "goby/moos/moos_node.h"
#include "goby/common/liaison_container.h"
#include "goby/moos/protobuf/liaison_config.pb.h"
#include "goby/moos/protobuf/pAcommsHandler_config.pb.h"

namespace goby
{
    namespace common
    {
        class QueueBar : public Wt::WProgressBar
        {
          public:
            QueueBar(Wt::WContainerWidget* parent = 0)
                : Wt::WProgressBar(parent)
            { }
            Wt::WString text() const;            
            
        };
        
        
        class LiaisonAcomms : public LiaisonContainer, public goby::moos::MOOSNode
        {
            
          public:
            LiaisonAcomms(ZeroMQService* zeromq_service, const protobuf::LiaisonConfig& cfg, Wt::WContainerWidget* parent = 0);
            
          private:
            void process_acomms_config();
            
            void loop();
            void moos_inbox(CMOOSMsg& msg);
            
            void dccl_analyze(const Wt::WMouseEvent& event);
            void dccl_select(Wt::WString msg); 
            void dccl_message(const Wt::WMouseEvent& event);

            void queue_info(const Wt::WMouseEvent& event, int id);
            void queue_flush(const Wt::WMouseEvent& event, int id);

            
            void focus()
            { timer_.start(); }

            void unfocus()
            { timer_.stop(); }

            
          private:
            boost::mutex dccl_mutex_;
            dccl::Codec dccl_;
            
            Wt::WPushButton* dccl_analyze_;
            Wt::WPushButton* dccl_message_;
            Wt::WText* dccl_analyze_text_;
            Wt::WText* dccl_message_text_;
            Wt::WComboBox* dccl_combo_;
            Wt::WTable* queue_table_;
                
            std::map<dccl::int32, QueueBar*> queue_bars_;

            // maps dccl id onto QueuedMessageEntry index
            std::map<dccl::int32, int> queue_cfg_;
            
                
            struct QueueStats
            {
            QueueStats() : last_rx_time(-1) { }
                
                int last_rx_time;
                Wt::WText* last_rx_time_text;
            };
            std::map<dccl::int32, QueueStats> queue_stats_;
            
            ZeroMQService* zeromq_service_;
            const protobuf::AcommsConfig& cfg_;
            pAcommsHandlerConfig acomms_config_;
            bool have_acomms_config_;
            
            Wt::WTimer timer_;

        };

    }
}

#endif
