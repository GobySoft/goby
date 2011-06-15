// copyright 2011 t. schneider tes@mit.edu
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

#include "liaison_scope.h"

#include <Wt/WStandardItemModel>
#include <Wt/WStandardItem>
#include <Wt/WTreeView>
#include <Wt/WPanel>
#include <Wt/WTextArea>
#include <Wt/WAnchor>
#include <Wt/WLineEdit>
#include <Wt/WPushButton>
#include <Wt/WVBoxLayout>
#include <Wt/WSortFilterProxyModel>
#include "liaison.h"

#include "TreeViewExample.h"

using namespace Wt;
using namespace goby::util::logger_lock;


goby::core::LiaisonScope::LiaisonScope(ZeroMQService* service)
    : MOOSNode(service)
{
    this->resize(WLength::Auto, WLength(90, WLength::Percentage));

    MOOSNode::set_global_blackout(boost::posix_time::milliseconds(1000));

    Wt::WVBoxLayout *layout = new Wt::WVBoxLayout(this);
    
    setStyleClass("scope");
    model_ = new Wt::WStandardItemModel(0,7, this);
//   Wt::WStandardItem *root = model_->invisibleRootItem();



    subscriptions_div_ = new Wt::WContainerWidget;
    new WText(("Add subscription (e.g. NAV* or NAV_X): "), subscriptions_div_);
    subscribe_filter_text_ = new WLineEdit(subscriptions_div_);
    subscribe_filter_text_->setStyleClass("basic_line_edit");
    WPushButton* subscribe_filter_button = new WPushButton("Apply", subscriptions_div_);
    new WBreak(subscriptions_div_);
    subscribe_filter_button->setStyleClass("basic_push_button");
    subscribe_filter_text_->setStyleClass("basic_line_edit");
    subscribe_filter_button->clicked().connect(this, &LiaisonScope::handle_add_subscription);
    subscribe_filter_text_->enterPressed().connect(this, &LiaisonScope::handle_add_subscription);
    layout->addWidget(subscriptions_div_, 0);
    new WText("Subscriptions (click to remove): ", subscriptions_div_);

        
    model_->setHeaderData(0, Horizontal, std::string("Key"));
    model_->setHeaderData(1, Horizontal, std::string("Type"));
    model_->setHeaderData(2, Horizontal, std::string("Value"));
    model_->setHeaderData(3, Horizontal, std::string("Time"));
    model_->setHeaderData(4, Horizontal, std::string("Community"));
    model_->setHeaderData(5, Horizontal, std::string("Source"));
    model_->setHeaderData(6, Horizontal, std::string("Source Aux"));

    // for (int row = 0; row < topLevelRows; ++row) {
    //     Wt::WStandardItem *topLevel = new Wt::WStandardItem();
    //     topLevel->setText("Item " + boost::lexical_cast<std::string>(row));
    //     for (int row2 = 0; row2 < secondLevelRows; ++row2) {
    //         Wt::WStandardItem *item = new Wt::WStandardItem();
    //         item->setText("Item " + boost::lexical_cast<std::string>(row)
    //                       + ": " + boost::lexical_cast<std::string>(row2));
    //         for (int row3 = 0; row3 < thirdLevelRows; ++row3) {
    //             Wt::WStandardItem *item2 = new Wt::WStandardItem();
    //             item2->setText("Item " + boost::lexical_cast<std::string>(row)
    //                           + ": " + boost::lexical_cast<std::string>(row2)
    //                           + ": " + boost::lexical_cast<std::string>(row3));
    //             item->appendRow(item2);
    //         }
    //         topLevel->appendRow(item);
    //     }
    //     root->appendRow(topLevel);
    // }    

    proxy_ = new Wt::WSortFilterProxyModel(this);
    proxy_->setSourceModel(model_);
    //   proxy_->setDynamicSortFilter(true);
    proxy_->setFilterKeyColumn(0);
    proxy_->setFilterRegExp(".*");
    
    
    // Create the WTreeView
    
     
    scope_tree_view_ = new Wt::WTreeView();
    scope_tree_view_->setModel(proxy_);    
    scope_tree_view_->setAlternatingRowColors(true);

    // scope_tree_view_->setColumnWidth(0, WLength(100));
    // scope_tree_view_->setColumnWidth(1, WLength(40));
    // scope_tree_view_->setColumnWidth(2, WLength(150));
    // scope_tree_view_->setColumnWidth(3, WLength(100));
    // scope_tree_view_->setColumnWidth(4, WLength(60));
    // scope_tree_view_->setColumnWidth(5, WLength(60)); 
    // scope_tree_view_->setColumnWidth(6, WLength(100));
//    scope_tree_view_->setColumn1Fixed(true);
    scope_tree_view_->setColumnWidth(0, WLength(10, WLength::FontEm));
    scope_tree_view_->setColumnWidth(1, WLength(4, WLength::FontEm));
    scope_tree_view_->setColumnWidth(2, WLength(15, WLength::FontEm));
    scope_tree_view_->setColumnWidth(3, WLength(10, WLength::FontEm));
    scope_tree_view_->setColumnWidth(4, WLength(6, WLength::FontEm));
    scope_tree_view_->setColumnWidth(5, WLength(6, WLength::FontEm));
    scope_tree_view_->setColumnWidth(5, WLength(10, WLength::FontEm));
    scope_tree_view_->sortByColumn(0, AscendingOrder);
    scope_tree_view_->resize(WLength::Auto,
                             400);

    layout->addWidget(scope_tree_view_);


//layout->setResizable(0);
    layout->setResizable(1);

    WContainerWidget* container = new WContainerWidget;
    container->resize(WLength::Auto, WLength(100, WLength::Percentage));
    
    layout->addWidget(container, 1);
//    layout->addWidget(new WText("hi"), 1);

    
    //scope_tree_view_->collapsed().connect(this, &LiaisonScope::collapsed);
    //scope_tree_view_->expanded().connect(this, &LiaisonScope::expanded);
    
    if(Liaison::cfg_.HasExtension(protobuf::moos_scope_config))
    {
        const protobuf::MOOSScopeConfig& moos_scope_config = Liaison::cfg_.GetExtension(protobuf::moos_scope_config);
        for(int i = 0, n = moos_scope_config.subscription_size(); i < n; ++i)
            add_subscription(moos_scope_config.subscription(i));
    }
    
}

void goby::core::LiaisonScope::handle_add_subscription()
{
    add_subscription(subscribe_filter_text_->text().narrow());
    subscribe_filter_text_->setText("");
}

void goby::core::LiaisonScope::add_subscription(const std::string& type)
{
    WPushButton* new_button = new WPushButton(subscriptions_div_);
    new_button->setText(type + " ");
    MOOSNode::subscribe(type, Liaison::LIAISON_INTERNAL_PUBLISH_SOCKET);

    new_button->clicked().connect(boost::bind(&LiaisonScope::handle_remove_subscription, this, new_button));
}



void goby::core::LiaisonScope::handle_remove_subscription(WPushButton* clicked_button)
{
    std::string type_name = clicked_button->text().narrow();
    boost::trim(type_name);
    unsigned type_name_size = type_name.size();
    MOOSNode::unsubscribe(clicked_button->text().narrow(), Liaison::LIAISON_INTERNAL_PUBLISH_SOCKET);

    bool has_wildcard_ending = (type_name[type_name_size - 1] == '*');
    if(has_wildcard_ending)
        type_name = type_name.substr(0, type_name_size-1);

    for(int i = model_->rowCount()-1, n = 0; i >= n; --i)
    {
        std::string text_to_match = model_->item(i, 0)->text().narrow();
        boost::trim(text_to_match);
        
        bool remove = false;
        if(has_wildcard_ending && boost::starts_with(text_to_match, type_name))
            remove = true;
        else if(!has_wildcard_ending && boost::equals(text_to_match, type_name))
            remove = true;

        if(remove)
        {            
            msg_map_.erase(text_to_match);
            glog.is(debug1, lock) && glog << "LiaisonScope: removed " << text_to_match << std::endl << unlock;            
            model_->removeRow(i);
            
            // shift down the remaining indices
            for(std::map<std::string, int>::iterator it = msg_map_.begin(),
                    n = msg_map_.end();
                it != n; ++it)
            {
                if(it->second > i)
                    --it->second;
            }            
        }
    }


    subscriptions_div_->removeWidget(clicked_button);
    delete clicked_button; // removeWidget does not delete
}



std::vector< WStandardItem * > goby::core::LiaisonScope::create_row(CMOOSMsg& msg)
{
    std::vector< WStandardItem * > items;
    Wt::WStandardItem* key_item = new Wt::WStandardItem(msg.GetKey());
//    key_item->setFlags(ItemIsEditable);
//    key_item->setRowCount(1);
    items.push_back(key_item);
    
    
    items.push_back(new Wt::WStandardItem((msg.IsDouble() ? "double" : "string")));
    
    Wt::WStandardItem* value_item = new Wt::WStandardItem;
    if(msg.IsDouble())
        value_item->setData(msg.GetDouble(), DisplayRole);
    else
        value_item->setData(msg.GetString(), DisplayRole);
    items.push_back(value_item);

    items.push_back(new Wt::WStandardItem(goby::util::as<std::string>(goby::util::unix_double2ptime(msg.GetTime()))));
    items.push_back(new Wt::WStandardItem(msg.GetCommunity()));
    items.push_back(new Wt::WStandardItem(msg.m_sSrc));
    items.push_back(new Wt::WStandardItem(msg.GetSourceAux()));
    return items;
    
}



void goby::core::LiaisonScope::moos_inbox(CMOOSMsg& msg)
{

    using goby::moos::operator<<;
    
    glog.is(debug1, lock) && glog << "LiaisonScope: got message:  " << msg << std::endl << unlock;
    
    std::map<std::string, int>::iterator it = msg_map_.find(msg.GetKey());
    if(it != msg_map_.end())
    {
        model_->item(it->second, 1)->setText((msg.IsDouble() ? "double" : "string"));
        
        if(msg.IsDouble())
            model_->item(it->second, 2)->setData(msg.GetDouble(), DisplayRole);
        else
            model_->item(it->second, 2)->setData(msg.GetString(), DisplayRole);
        
        model_->item(it->second, 3)->setText(goby::util::as<std::string>(goby::util::unix_double2ptime(msg.GetTime())));
        
        model_->item(it->second, 4)->setText(msg.GetCommunity());
        model_->item(it->second, 5)->setText(msg.m_sSrc);
        model_->item(it->second, 6)->setText(msg.GetSourceAux());
    }
    else
    {
        std::vector< WStandardItem * > items = create_row(msg);
        msg_map_.insert(make_pair(msg.GetKey(), model_->rowCount()));
        model_->appendRow(items);
    }

    
    proxy_->setFilterRegExp(".*");
     
}


// void goby::core::LiaisonScope::expanded(Wt::WModelIndex index)
// {
//     MOOSNode::set_blackout(boost::any_cast<Wt::WString>(index.data()).narrow(),
//                            boost::posix_time::milliseconds(0));
// }

// void goby::core::LiaisonScope::collapsed(Wt::WModelIndex index)
// {
//     MOOSNode::clear_blackout(boost::any_cast<Wt::WString>(index.data()).narrow());    
// }

