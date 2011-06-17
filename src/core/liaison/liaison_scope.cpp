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

#include <Wt/WStandardItem>
#include <Wt/WPanel>
#include <Wt/WTextArea>
#include <Wt/WAnchor>
#include <Wt/WLineEdit>
#include <Wt/WPushButton>
#include <Wt/WVBoxLayout>
#include <Wt/WSortFilterProxyModel>
#include <Wt/WSelectionBox>
#include <Wt/WTable>
#include <Wt/WTimer>
#include <Wt/Chart/WCartesianChart>
#include <Wt/WDateTime>
#include "liaison.h"

#include "TreeViewExample.h"

using namespace Wt;
using namespace goby::util::logger_lock;


goby::core::LiaisonScope::LiaisonScope(ZeroMQService* service, WTimer* timer)
    : MOOSNode(service),
      moos_scope_config_(Liaison::cfg_.GetExtension(protobuf::moos_scope_config)),
      timer_(timer),
      is_paused_(moos_scope_config_.start_paused()),
      last_main_layout_index_(-1)
//      last_history_layout_index_(-1)
{
    this->resize(WLength::Auto, WLength(100, WLength::Percentage));
    
    service->socket_from_id(Liaison::LIAISON_INTERNAL_PUBLISH_SOCKET).set_global_blackout(boost::posix_time::milliseconds(1/Liaison::cfg_.update_freq()*1e3));
    
    main_layout_ = new Wt::WVBoxLayout(this);

    setStyleClass("scope");

    model_ = new LiaisonScopeMOOSModel(moos_scope_config_, this);
    

    proxy_ = new Wt::WSortFilterProxyModel(this);
    proxy_->setSourceModel(model_);

    Wt::WContainerWidget* controls_div = new Wt::WContainerWidget;
    play_state_ = new Wt::WText(controls_div);
    WPushButton* play_pause_button = new WPushButton("Play/Pause", controls_div);
    handle_play_pause(false);
    
    play_pause_button->clicked().connect(boost::bind(&LiaisonScope::handle_play_pause, this, true));
    
    ++last_main_layout_index_;
    main_layout_->addWidget(controls_div, 0);
    

    
    subscriptions_div_ = new Wt::WContainerWidget;
    new WText(("Add subscription (e.g. NAV* or NAV_X): "), subscriptions_div_);
    subscribe_filter_text_ = new WLineEdit(subscriptions_div_);
    WPushButton* subscribe_filter_button = new WPushButton("Apply", subscriptions_div_);
    new WBreak(subscriptions_div_);
    subscribe_filter_button->clicked().connect(this, &LiaisonScope::handle_add_subscription);
    subscribe_filter_text_->enterPressed().connect(this, &LiaisonScope::handle_add_subscription);

    ++last_main_layout_index_;
    main_layout_->addWidget(subscriptions_div_, 0);
    new WText("Subscriptions (click to remove): ", subscriptions_div_);
    

    WContainerWidget* history_header_div = new WContainerWidget; //(history_div_);
    new WText("<hr />", history_header_div);
    new WText(("Add history for key: "), history_header_div);
//    history_box_ = new WComboBox(history_header_div);
    history_box_ = new WComboBox(history_header_div);
    WPushButton* history_button = new WPushButton("Add", history_header_div);
    history_button->clicked().connect(this, &LiaisonScope::handle_add_history);
    history_box_->setModel(proxy_);
    ++last_main_layout_index_;
    main_layout_->addWidget(history_header_div, 0);

    
    regex_filter_div_ = new Wt::WContainerWidget;
    new WText("<hr />", regex_filter_div_);
    new WText(("Set regex filter: Column: "), regex_filter_div_);
    regex_column_select_ = new Wt::WComboBox(regex_filter_div_);
    for(int i = 0, n = model_->columnCount(); i < n; ++i)
        regex_column_select_->addItem(boost::any_cast<std::string>(model_->headerData(i)));
    regex_column_select_->setCurrentIndex(moos_scope_config_.regex_filter_column());

    new WText((" Expression: "), regex_filter_div_);
    regex_filter_text_ = new WLineEdit(regex_filter_div_);
    regex_filter_text_->setText(moos_scope_config_.regex_filter_expression());

    
    
    WPushButton* regex_filter_button = new WPushButton("Set", regex_filter_div_);
    WPushButton* regex_filter_clear = new WPushButton("Clear", regex_filter_div_);
    WPushButton* regex_filter_examples = new WPushButton("Examples", regex_filter_div_);
    
    regex_filter_button->clicked().connect(this, &LiaisonScope::handle_set_regex_filter);
    regex_filter_clear->clicked().connect(this, &LiaisonScope::handle_clear_regex_filter);
    regex_filter_text_->enterPressed().connect(this, &LiaisonScope::handle_set_regex_filter);
    regex_filter_examples->clicked().connect(this, &LiaisonScope::toggle_regex_examples_table);

    new WBreak(regex_filter_div_);
    regex_examples_table_ = new WTable(regex_filter_div_);
    regex_examples_table_->setHeaderCount(1);
    regex_examples_table_->setStyleClass("basic_small");
    new WText("Expression", regex_examples_table_->elementAt(0,0));
    new WText("Matches", regex_examples_table_->elementAt(0,1));
    new WText(".*", regex_examples_table_->elementAt(1,0));
    new WText("anything", regex_examples_table_->elementAt(1,1));
    new WText(".*_STATUS", regex_examples_table_->elementAt(2,0));
    new WText("fields ending in \"_STATUS\"",regex_examples_table_->elementAt(2,1));
    new WText(".*[^(_STATUS)]",regex_examples_table_->elementAt(3,0));
    new WText("fields <i>not</i> ending in \"_STATUS\"",regex_examples_table_->elementAt(3,1));
    new WText("-?[[:digit:]]*\\.[[:digit:]]*e?-?[[:digit:]]*",regex_examples_table_->elementAt(4,0));
    new WText("a floating point number (e.g. -3.456643e7)", regex_examples_table_->elementAt(4,1));
    regex_examples_table_->hide();
        
    ++last_main_layout_index_;
    main_layout_->addWidget(regex_filter_div_, 0);
    
    
    
    // Create the WTreeView
    
     
    scope_tree_view_ = new LiaisonScopeMOOSTreeView(moos_scope_config_);
    scope_tree_view_->setModel(proxy_);    
    scope_tree_view_->sortByColumn(moos_scope_config_.sort_by_column(),
                                   moos_scope_config_.sort_ascending()? AscendingOrder : DescendingOrder);
    
    
    ++last_main_layout_index_;
    main_layout_->addWidget(scope_tree_view_);
    main_layout_->setResizable(last_main_layout_index_);

    history_div_ = new WContainerWidget;
    
//    history_tree_div_ = new WContainerWidget(history_div_);
//    history_layout_ = new WVBoxLayout(history_tree_div_);

//    history_layout_ = new WVBoxLayout(history_div_);
    
//    ++last_main_layout_index_;
//    main_layout_->addWidget(history_div_);
    
    WContainerWidget* bottom_fill = new WContainerWidget;
    ++last_main_layout_index_;
    main_layout_->addWidget(bottom_fill, -1, AlignTop);
    main_layout_->addStretch(1);
    bottom_fill->resize(WLength::Auto, 100);
    main_layout_->setResizable(last_main_layout_index_-1);
    
    for(int i = 0, n = moos_scope_config_.subscription_size(); i < n; ++i)
        add_subscription(moos_scope_config_.subscription(i));

    for(int i = 0, n = moos_scope_config_.history_size(); i < n; ++i)
        add_history(moos_scope_config_.history(i));
    
    handle_set_regex_filter();


    wApp->globalKeyPressed().connect(this, &LiaisonScope::handle_global_key);
}

void goby::core::LiaisonScope::handle_play_pause(bool toggle_state)
{
    if(toggle_state)
        is_paused_ = !is_paused_;

    is_paused_ ? timer_->stop() : timer_->start();
    play_state_->setText(is_paused_ ? "Paused (any key refreshes). " : "Playing... ");
}


void goby::core::LiaisonScope::handle_add_subscription()
{
    
    add_subscription(subscribe_filter_text_->text().narrow());
    subscribe_filter_text_->setText("");
}

void goby::core::LiaisonScope::add_subscription(std::string type)
{
    boost::trim(type);
    if(type.empty())
        return;
    
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


void goby::core::LiaisonScope::handle_set_regex_filter()
{
    proxy_->setFilterKeyColumn(regex_column_select_->currentIndex());
    proxy_->setFilterRegExp(regex_filter_text_->text());
}


void goby::core::LiaisonScope::handle_clear_regex_filter()
{
    regex_filter_text_->setText(".*");
    handle_set_regex_filter();
}

void goby::core::LiaisonScope::toggle_regex_examples_table()
{
    regex_examples_table_->isHidden() ?
        regex_examples_table_->show() :
        regex_examples_table_->hide();
    
}

void goby::core::LiaisonScope::handle_add_history()
{
    std::string selected_key = history_box_->currentText().narrow();
    goby::core::protobuf::MOOSScopeConfig::HistoryConfig config;
    config.set_key(selected_key);
    add_history(config);
}

void goby::core::LiaisonScope::add_history(const goby::core::protobuf::MOOSScopeConfig::HistoryConfig& config)
{
    const std::string& selected_key = config.key();
    
    if(!history_models_.count(selected_key))
    {
        Wt::WContainerWidget* new_container = new WContainerWidget;

        Wt::WContainerWidget* text_container = new WContainerWidget(new_container);
        new WText("History for  ", text_container);
        WPushButton* remove_history_button = new WPushButton(selected_key, text_container);

        remove_history_button->clicked().connect(
            boost::bind(&LiaisonScope::handle_remove_history, this, selected_key));
        
        new WText(" (click to remove)", text_container);
        new WBreak(text_container);
        WPushButton* toggle_plot_button = new WPushButton("Plot", text_container);

        
        text_container->resize(Wt::WLength::Auto, WLength(4, WLength::FontEm));

        Wt::WStandardItemModel* new_model = new LiaisonScopeMOOSModel(moos_scope_config_,history_div_);
        Wt::WSortFilterProxyModel* new_proxy = new Wt::WSortFilterProxyModel(new_container);
        new_proxy->setSourceModel(new_model);

        
        Chart::WCartesianChart* chart = new Chart::WCartesianChart(new_container);
        toggle_plot_button->clicked().connect(
            boost::bind(&LiaisonScope::toggle_history_plot, this, chart));
        chart->setModel(new_model);    
        chart->setXSeriesColumn(protobuf::MOOSScopeConfig::COLUMN_TIME); 
        Chart::WDataSeries s(protobuf::MOOSScopeConfig::COLUMN_VALUE, Chart::LineSeries);
        chart->addSeries(s);        
        
        chart->setType(Chart::ScatterPlot);
        chart->axis(Chart::XAxis).setScale(Chart::DateTimeScale); 
        chart->axis(Chart::XAxis).setTitle("Time"); 
        chart->axis(Chart::YAxis).setTitle(selected_key); 

        WFont font;
        font.setFamily(WFont::Serif, "Gentium");
        chart->axis(Chart::XAxis).setTitleFont(font); 
        chart->axis(Chart::YAxis).setTitleFont(font); 

        
        // Provide space for the X and Y axis and title. 
        chart->setPlotAreaPadding(80, Left);
        chart->setPlotAreaPadding(40, Top | Bottom);
        chart->setMargin(10, Top | Bottom);            // add margin vertically
        chart->setMargin(WLength::Auto, Left | Right); // center horizontally
        chart->resize(config.plot_width(), config.plot_height());

        if(!config.show_plot())
            chart->hide();
        
        Wt::WTreeView* new_tree = new LiaisonScopeMOOSTreeView(moos_scope_config_, new_container);        
        ++last_main_layout_index_;
        main_layout_->insertWidget(last_main_layout_index_-1, new_container);

        main_layout_->setResizable(last_main_layout_index_-1);

        ++last_main_layout_index_;
        main_layout_->insertWidget(last_main_layout_index_-1, new_tree);
        
        main_layout_->setResizable(last_main_layout_index_-1);
        
        new_tree->setModel(new_proxy);
        MVC& mvc = history_models_[selected_key];
        mvc.key = selected_key;
        mvc.container = new_container;
        mvc.model = new_model;
        mvc.tree = new_tree;
        mvc.proxy = new_proxy;

        MOOSNode::zeromq_service()->socket_from_id(Liaison::LIAISON_INTERNAL_PUBLISH_SOCKET).set_blackout(
            MARSHALLING_MOOS,
            "CMOOSMsg/" + selected_key + "/",
            boost::posix_time::milliseconds(0));

        new_proxy->setFilterRegExp(".*");
        new_tree->sortByColumn(protobuf::MOOSScopeConfig::COLUMN_TIME,
                               DescendingOrder);
    }
}

void goby::core::LiaisonScope::handle_remove_history(std::string key)
{
    glog.is(debug2, lock) && glog << "LiaisonScope: removing history for: " << key << std::endl << unlock;

    
    main_layout_->removeWidget(history_models_[key].container);
    --last_main_layout_index_;
    
    main_layout_->removeWidget(history_models_[key].tree);
    --last_main_layout_index_;

    delete history_models_[key].container;
    delete history_models_[key].tree;
    history_models_.erase(key);
}

void goby::core::LiaisonScope::toggle_history_plot(Wt::WWidget* plot)
{
    if(plot->isHidden())
        plot->show();
    else
        plot->hide();
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

    Wt::WStandardItem* time_item = new Wt::WStandardItem;
    time_item->setData(WDateTime::fromPosixTime(goby::util::unix_double2ptime(msg.GetTime())), DisplayRole);
    items.push_back(time_item);
    
    items.push_back(new Wt::WStandardItem(msg.GetCommunity()));
    items.push_back(new Wt::WStandardItem(msg.m_sSrc));
    items.push_back(new Wt::WStandardItem(msg.GetSourceAux()));
    return items;
    
}

void goby::core::LiaisonScope::handle_global_key(Wt::WKeyEvent event)
{
    while(MOOSNode::zeromq_service()->poll(0))
    { }
}




void goby::core::LiaisonScope::moos_inbox(CMOOSMsg& msg)
{

    using goby::moos::operator<<;
    
    glog.is(debug1, lock) && glog << "LiaisonScope: got message:  " << msg << std::endl << unlock;
    std::map<std::string, int>::iterator it = msg_map_.find(msg.GetKey());
    if(it != msg_map_.end())
    {
        model_->item(it->second, protobuf::MOOSScopeConfig::COLUMN_TYPE)->setText((msg.IsDouble() ? "double" : "string"));
        
        if(msg.IsDouble())
            model_->item(it->second, protobuf::MOOSScopeConfig::COLUMN_VALUE)->setData(msg.GetDouble(), DisplayRole);
        else
            model_->item(it->second, protobuf::MOOSScopeConfig::COLUMN_VALUE)->setData(msg.GetString(), DisplayRole);
        
        model_->item(it->second, protobuf::MOOSScopeConfig::COLUMN_TIME)->setData(WDateTime::fromPosixTime(goby::util::unix_double2ptime(msg.GetTime())), DisplayRole);
        
        model_->item(it->second, protobuf::MOOSScopeConfig::COLUMN_COMMUNITY)->setText(msg.GetCommunity());
        model_->item(it->second, protobuf::MOOSScopeConfig::COLUMN_SOURCE)->setText(msg.m_sSrc);
        model_->item(it->second, protobuf::MOOSScopeConfig::COLUMN_SOURCE_AUX)->setText(msg.GetSourceAux());
    }
    else
    {
        std::vector< WStandardItem * > items = create_row(msg);
        msg_map_.insert(make_pair(msg.GetKey(), model_->rowCount()));
        model_->appendRow(items);
    }
    handle_set_regex_filter();

    std::map<std::string, MVC>::iterator hist_it = history_models_.find(msg.GetKey());
    if(hist_it != history_models_.end())
    {
        hist_it->second.model->appendRow(create_row(msg));
        hist_it->second.proxy->setFilterRegExp(".*");
    }
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



goby::core::LiaisonScopeMOOSTreeView::LiaisonScopeMOOSTreeView(const protobuf::MOOSScopeConfig& moos_scope_config , Wt::WContainerWidget* parent /*= 0*/)
    : WTreeView(parent)
{
    this->setAlternatingRowColors(true);

    this->setColumnWidth(protobuf::MOOSScopeConfig::COLUMN_KEY, moos_scope_config.column_width().key_width());
    this->setColumnWidth(protobuf::MOOSScopeConfig::COLUMN_TYPE, moos_scope_config.column_width().type_width());
    this->setColumnWidth(protobuf::MOOSScopeConfig::COLUMN_VALUE, moos_scope_config.column_width().value_width());
    this->setColumnWidth(protobuf::MOOSScopeConfig::COLUMN_TIME, moos_scope_config.column_width().time_width());
    this->setColumnWidth(protobuf::MOOSScopeConfig::COLUMN_COMMUNITY, moos_scope_config.column_width().community_width());
    this->setColumnWidth(protobuf::MOOSScopeConfig::COLUMN_SOURCE, moos_scope_config.column_width().source_width());
    this->setColumnWidth(protobuf::MOOSScopeConfig::COLUMN_SOURCE_AUX, moos_scope_config.column_width().source_aux_width());

    this->resize(Wt::WLength::Auto,
                 moos_scope_config.scope_height());

    this->setMinimumSize(moos_scope_config.column_width().key_width()+
                         moos_scope_config.column_width().type_width()+
                         moos_scope_config.column_width().value_width()+
                         moos_scope_config.column_width().time_width()+
                         moos_scope_config.column_width().community_width()+
                         moos_scope_config.column_width().source_width()+
                         moos_scope_config.column_width().source_aux_width()+
                         7*(protobuf::MOOSScopeConfig::COLUMN_MAX+1),
                         Wt::WLength::Auto);
}
            

goby::core::LiaisonScopeMOOSModel::LiaisonScopeMOOSModel(const protobuf::MOOSScopeConfig& moos_scope_config, Wt::WContainerWidget* parent /*= 0*/)
    : WStandardItemModel(0, protobuf::MOOSScopeConfig::COLUMN_MAX+1, parent)
{
    this->setHeaderData(protobuf::MOOSScopeConfig::COLUMN_KEY, Horizontal, std::string("Key"));
    this->setHeaderData(protobuf::MOOSScopeConfig::COLUMN_TYPE, Horizontal, std::string("Type"));
    this->setHeaderData(protobuf::MOOSScopeConfig::COLUMN_VALUE, Horizontal, std::string("Value"));
    this->setHeaderData(protobuf::MOOSScopeConfig::COLUMN_TIME, Horizontal, std::string("Time"));
    this->setHeaderData(protobuf::MOOSScopeConfig::COLUMN_COMMUNITY, Horizontal, std::string("Community"));
    this->setHeaderData(protobuf::MOOSScopeConfig::COLUMN_SOURCE, Horizontal, std::string("Source"));
    this->setHeaderData(protobuf::MOOSScopeConfig::COLUMN_SOURCE_AUX, Horizontal, std::string("Source Aux"));


}

