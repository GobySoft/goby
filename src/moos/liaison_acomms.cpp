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

#include <sstream>

#include <Wt/WGroupBox>
#include <Wt/WLineEdit>
#include <Wt/WBreak>
#include <Wt/WPushButton>
#include <Wt/WComboBox>
#include <Wt/WContainerWidget>
#include <Wt/WLabel>
#include <Wt/WFont>
#include <Wt/WTable>
#include <Wt/WCssDecorationStyle>
#include <Wt/WDialog>

#include "goby/util/as.h"
#include "goby/util/dynamic_protobuf_manager.h"

#include "goby/moos/moos_protobuf_helpers.h"

#include "liaison_acomms.h"

const std::string STRIPE_ODD_CLASS = "odd";
const std::string STRIPE_EVEN_CLASS = "even";

using namespace goby::common::logger;
using namespace Wt;

goby::common::LiaisonAcomms::LiaisonAcomms(ZeroMQService* zeromq_service, const protobuf::LiaisonConfig& cfg, Wt::WContainerWidget* parent)
    : LiaisonContainer(parent),
      MOOSNode(zeromq_service),
      zeromq_service_(zeromq_service),
      cfg_(cfg.GetExtension(protobuf::acomms_config)),
      have_acomms_config_(false)
{
    
    protobuf::ZeroMQServiceConfig ipc_sockets;
    protobuf::ZeroMQServiceConfig::Socket* internal_subscribe_socket = ipc_sockets.add_socket();
    internal_subscribe_socket->set_socket_type(protobuf::ZeroMQServiceConfig::Socket::SUBSCRIBE);
    internal_subscribe_socket->set_socket_id(LIAISON_INTERNAL_SUBSCRIBE_SOCKET);
    internal_subscribe_socket->set_transport(protobuf::ZeroMQServiceConfig::Socket::INPROC);
    internal_subscribe_socket->set_connect_or_bind(protobuf::ZeroMQServiceConfig::Socket::CONNECT);
    internal_subscribe_socket->set_socket_name(liaison_internal_publish_socket_name());

    protobuf::ZeroMQServiceConfig::Socket* internal_publish_socket = ipc_sockets.add_socket();
    internal_publish_socket->set_socket_type(protobuf::ZeroMQServiceConfig::Socket::PUBLISH);
    internal_publish_socket->set_socket_id(LIAISON_INTERNAL_PUBLISH_SOCKET);
    internal_publish_socket->set_transport(protobuf::ZeroMQServiceConfig::Socket::INPROC);
    internal_publish_socket->set_connect_or_bind(protobuf::ZeroMQServiceConfig::Socket::CONNECT);
    internal_publish_socket->set_socket_name(liaison_internal_subscribe_socket_name());

    zeromq_service_->merge_cfg(ipc_sockets);

    subscribe("ACOMMS_CONFIG", LIAISON_INTERNAL_SUBSCRIBE_SOCKET);
    subscribe("ACOMMS_QSIZE", LIAISON_INTERNAL_SUBSCRIBE_SOCKET);
    subscribe("ACOMMS_QUEUE_RECEIVE", LIAISON_INTERNAL_SUBSCRIBE_SOCKET);
    subscribe("ACOMMS_MAC_INITIATE_TRANSMISSION", LIAISON_INTERNAL_SUBSCRIBE_SOCKET);
    subscribe("ACOMMS_MODEM_TRANSMIT", LIAISON_INTERNAL_SUBSCRIBE_SOCKET);
    subscribe("ACOMMS_MODEM_RECEIVE", LIAISON_INTERNAL_SUBSCRIBE_SOCKET);


    MOOSNode::send(CMOOSMsg(MOOS_NOTIFY, "ACOMMS_CONFIG_REQUEST", "Request"), LIAISON_INTERNAL_PUBLISH_SOCKET);
    
    timer_.setInterval(1/cfg.update_freq()*1.0e3);
    timer_.timeout().connect(this, &LiaisonAcomms::loop);

    WGroupBox* dccl_box = new Wt::WGroupBox("DCCL", this);    
    new WLabel("Message: ", dccl_box);
    dccl_combo_ = new WComboBox(dccl_box);
    
    dccl_combo_->addItem("(Choose a message)");
    
    dccl_combo_->sactivated().connect(this, &LiaisonAcomms::dccl_select);
    
    dccl_message_ = new WPushButton("Show .proto", dccl_box);
    dccl_message_->clicked().connect(this, &LiaisonAcomms::dccl_message);

    dccl_analyze_ = new WPushButton("Analyze", dccl_box);
    dccl_analyze_->clicked().connect(this, &LiaisonAcomms::dccl_analyze);

    
    new WBreak(dccl_box);
    
    dccl_message_text_ = new WText("", Wt::PlainText, dccl_box);
    WFont mono(Wt::WFont::Monospace);
    dccl_message_text_->decorationStyle().setFont(mono);
    dccl_message_text_->hide();

    new WBreak(dccl_box);
    
    dccl_analyze_text_ = new WText("", Wt::PlainText, dccl_box);
    dccl_analyze_text_->decorationStyle().setFont(mono);
    dccl_analyze_text_->hide();
    
    
    Wt::WGroupBox* queue_box = new Wt::WGroupBox("Queue", this);
    queue_table_ = new Wt::WTable(queue_box);
    queue_table_->setStyleClass("basic_small");
    queue_table_->setHeaderCount(1);
    queue_table_->elementAt(0,0)->addWidget(new WText("DCCL Message"));
    queue_table_->elementAt(0,1)->addWidget(new WText("Queue Size"));
    queue_table_->elementAt(0,2)->addWidget(new WText("Time since Rx"));

    
    WGroupBox* amac_box = new Wt::WGroupBox("AMAC", this);

    WGroupBox* driver_box = new Wt::WGroupBox("ModemDriver", this);
    
    set_name("MOOSAcomms");
}

void goby::common::LiaisonAcomms::loop()
{
    while(zeromq_service_->poll(0)) { }


    int now = goby::common::goby_time<double>();
    for(std::map<dccl::int32, QueueStats>::const_iterator it = queue_stats_.begin(), end = queue_stats_.end(); it != end; ++it)
    {
        if(it->second.last_rx_time > 0)
            it->second.last_rx_time_text->setText(goby::util::as<std::string>(now-it->second.last_rx_time) + " s");
        else
            it->second.last_rx_time_text->setText("Never");

    }
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

void goby::common::LiaisonAcomms::dccl_message(const WMouseEvent& event)
{
    if(dccl_message_text_->isHidden())
    {
        dccl_message_->setText("Hide .proto");
        dccl_message_text_->show();
    }
    else
    {
        dccl_message_->setText("Show .proto");
        dccl_message_text_->hide();
    }
}

void goby::common::LiaisonAcomms::dccl_select(WString msg)
{
    std::string m = msg.narrow();
    m = m.substr(m.find(" ") + 1);
    
    const google::protobuf::Descriptor* desc = goby::util::DynamicProtobufManager::find_descriptor(m);
    if(desc)
    {
        std::stringstream ss;
        boost::mutex::scoped_lock l(dccl_mutex_);
        dccl_.info(desc, &ss);
        dccl_analyze_text_->setText(ss.str());

        std::string proto = desc->DebugString();
        boost::replace_all(proto, "\n", " ");
        boost::replace_all(proto, ";", ";\n");
        boost::replace_all(proto, "} ", "}\n");
        dccl_message_text_->setText(proto);
    }
    else
    {
        dccl_analyze_text_->setText("");
        dccl_message_text_->setText("");
    }
}

void goby::common::LiaisonAcomms::moos_inbox(CMOOSMsg& msg)
{
    using goby::moos::operator<<;

    if(msg.GetKey() == "ACOMMS_CONFIG" && !have_acomms_config_)
    {
        acomms_config_.ParseFromString(dccl::b64_decode(msg.GetString()));
        process_acomms_config();
        have_acomms_config_ = true;
    }
    else if(msg.GetKey() == "ACOMMS_QSIZE")
    {
        goby::acomms::protobuf::QueueSize size;
        parse_for_moos(msg.GetString(), &size);
        
        if(queue_bars_.count(size.dccl_id()))
        {
            queue_bars_[size.dccl_id()]->setValue(size.size());
        }
        
    }
    else if(msg.GetKey() == "ACOMMS_QUEUE_RECEIVE")
    {
        boost::shared_ptr<google::protobuf::Message> dccl_msg = dynamic_parse_for_moos(msg.GetString());
        if(dccl_msg)
        {
            queue_stats_[dccl_.id(dccl_msg->GetDescriptor())].last_rx_time = goby::common::goby_time<double>();
        }
    }
}

void goby::common::LiaisonAcomms::process_acomms_config()
{
    boost::mutex::scoped_lock l(dccl_mutex_);

    // load messages in queue config
    for(int i = 0, n = acomms_config_.queue_cfg().message_entry_size(); i < n; ++i)
    {
        const goby::acomms::protobuf::QueuedMessageEntry& q = acomms_config_.queue_cfg().message_entry(i);
        
        const google::protobuf::Descriptor* desc =
            goby::util::DynamicProtobufManager::find_descriptor(q.protobuf_name());

        if(desc)
        {
            dccl_.load(desc);
            int id = dccl_.id(desc);
            queue_cfg_[id] = i;

            glog.is(DEBUG1) && glog << "Loaded: " << desc << ": " << q.protobuf_name() << std::endl;

            int row = queue_table_->rowCount();
            queue_table_->elementAt(row, 0)->addWidget(new WText(q.protobuf_name()));
            QueueBar* new_bar = new QueueBar;
            queue_table_->elementAt(row, 1)->addWidget(new_bar);
            
            new_bar->setMinimum(0);
            new_bar->setMaximum(q.max_queue());
            
            queue_bars_[id] = new_bar;

            QueueStats new_stats;
            new_stats.last_rx_time_text = new WText(goby::util::as<std::string>(new_stats.last_rx_time));
            queue_table_->elementAt(row, 2)->addWidget(new_stats.last_rx_time_text);
            queue_stats_[id] = new_stats;

            WPushButton* info = new WPushButton("Info");
            info->clicked().connect(boost::bind(&LiaisonAcomms::queue_info, this, _1, id));
            queue_table_->elementAt(row, 3)->addWidget(info);

            WPushButton* flush = new WPushButton("Flush");
            flush->clicked().connect(boost::bind(&LiaisonAcomms::queue_flush, this, _1, id));
            queue_table_->elementAt(row, 4)->addWidget(flush);            

            if(row & 1)
                queue_table_->rowAt(row)->setStyleClass(STRIPE_ODD_CLASS);
            else
                queue_table_->rowAt(row)->setStyleClass(STRIPE_EVEN_CLASS);
        }
        else
        {
            glog.is(WARN) && glog << "Cannot find message: " << q.protobuf_name() << std::endl;
        }
        
    }
    
    // load DCCL messages into drop down box
    for(std::map<dccl::int32, const google::protobuf::Descriptor*>::const_iterator it = dccl_.loaded().begin(), it_end = dccl_.loaded().end(); it != it_end; ++it)
    {
        dccl_combo_->addItem(std::string(goby::util::as<std::string>(it->first) + ": " + it->second->full_name()));
    }


}


Wt::WString goby::common::QueueBar::text() const
{
    return std::string(goby::util::as<std::string>(value()) + "/" + goby::util::as<std::string>(maximum()));
}

void goby::common::LiaisonAcomms::queue_info(const Wt::WMouseEvent& event, int id)
{
    const google::protobuf::Descriptor* desc = 0;
    {
        boost::mutex::scoped_lock l(dccl_mutex_);
        if(dccl_.loaded().count(id))
            desc = dccl_.loaded().find(id)->second;
    }

    if(!desc) return;
    
    glog.is(DEBUG1) && glog << "Queue info for " << desc->full_name() << std::endl;

    WDialog dialog("Queue info for " + desc->full_name());
    WContainerWidget* message_div = new WContainerWidget(dialog.contents());
    new WText(std::string("<pre>" + acomms_config_.queue_cfg().message_entry(queue_cfg_[id]).DebugString()  + "</pre>"), message_div);
     
    message_div->setOverflow(WContainerWidget::OverflowAuto);
    
    WPushButton ok("ok", dialog.contents());

    dialog.rejectWhenEscapePressed();
    ok.clicked().connect(&dialog, &WDialog::accept);

    if (dialog.exec() == WDialog::Accepted)
    { }
}


void goby::common::LiaisonAcomms::queue_flush(const Wt::WMouseEvent& event, int id)
{
    goby::acomms::protobuf::QueueFlush flush;
    flush.set_dccl_id(id);
    std::string serialized;
    serialize_for_moos(&serialized, flush);

    MOOSNode::send(CMOOSMsg(MOOS_NOTIFY, "ACOMMS_FLUSH_QUEUE", serialized), LIAISON_INTERNAL_PUBLISH_SOCKET);
    
}

