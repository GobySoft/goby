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


#include <Wt/WGroupBox>
#include <Wt/WLineEdit>
#include <Wt/WBreak>
#include <Wt/WPushButton>
#include <Wt/WComboBox>
#include <Wt/WContainerWidget>
#include <Wt/WLabel>

#include <goby/util/as.h>

// TEMP
#include "goby/acomms/protobuf/network_ack.pb.h"

#include "liaison_acomms.h"

using namespace Wt;

goby::common::LiaisonAcomms::LiaisonAcomms(const protobuf::LiaisonConfig& cfg, Wt::WContainerWidget* parent)
    : LiaisonContainer(parent),
      acomms_config_(cfg.GetExtension(protobuf::acomms_config))
{    
    WGroupBox* dccl_box = new Wt::WGroupBox("DCCL", this);    
    new WLabel("Message: ", dccl_box);
    WComboBox* dccl_combo = new WComboBox(dccl_box);

    dccl_.load<goby::acomms::protobuf::NetworkAck>();
    
    dccl_combo->addItem("(Choose a loaded message)");
    dccl_combo->addItem(goby::acomms::protobuf::NetworkAck::descriptor()->full_name());
    dccl_combo->activate().connect(this, &LiaisonAcomms::dccl_select);
    
    dccl_analyze_ = new WPushButton("Analyze", dccl_box);
    dccl_analyze_->clicked().connect(this, &LiaisonAcomms::dccl_analyze);

    new WBreak(dccl_box);
    
    dccl_analyze_text_ = new WText("Foobar", dccl_box);
    dccl_analyze_text_->hide();
    
    
    WGroupBox* queue_box = new Wt::WGroupBox("Queue", this);
    WGroupBox* amac_box = new Wt::WGroupBox("AMAC", this);
    WGroupBox* driver_box = new Wt::WGroupBox("ModemDriver", this);
    
    set_name("MOOSAcomms");
}

void goby::common::LiaisonAcomms::loop()
{
}



void goby::common::LiaisonAcomms::dccl_analyze(const WMouseEvent& event)
{
    if(dccl_analyze_text_->isHidden())
    {
        dccl_analyze_->setText("Hide Analysis");
        dccl_analyze_text_->show();
    }
    else
    {
        dccl_analyze_->setText("Analyze");
        dccl_analyze_text_->hide();
    }
}

void goby::common::LiaisonAcomms::dccl_select(int index)
{
}
