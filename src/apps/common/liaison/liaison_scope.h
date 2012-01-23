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

#ifndef LIAISONSCOPE20110609H
#define LIAISONSCOPE20110609H

#include <Wt/WText>
#include <Wt/WCssDecorationStyle>
#include <Wt/WBorder>
#include <Wt/WColor>
#include <Wt/WTreeView>
#include <Wt/WStandardItemModel>
#include <Wt/WEvent>
#include <Wt/WSortFilterProxyModel>
#include <Wt/WBoxLayout>
#include <Wt/WVBoxLayout>

#include "goby/moos/moos_node.h"

#include "liaison.h"

namespace Wt
{
    class WStandardItemModel;
}

namespace goby
{
    namespace common
    {
        class LiaisonScope : public LiaisonContainer, public goby::moos::MOOSNode
        {
            
          public:
            LiaisonScope(ZeroMQService* zeromq_service,
                         Wt::WTimer* timer_, Wt::WContainerWidget* parent = 0);
            
            void moos_inbox(CMOOSMsg& msg);
            void update_row(CMOOSMsg& msg, const std::vector<Wt::WStandardItem *>& items);
            std::vector<Wt::WStandardItem *> create_row(CMOOSMsg& msg);
            void attach_pb_rows(const std::vector<Wt::WStandardItem *>& items,
                                CMOOSMsg& msg);
            
            void loop();
            
          private:
            void handle_global_key(Wt::WKeyEvent event);

          private:
            ZeroMQService* zeromq_service_;
            const protobuf::MOOSScopeConfig& moos_scope_config_;

            Wt::WStringListModel* history_model_;
            Wt::WStandardItemModel* model_;
            Wt::WSortFilterProxyModel* proxy_;

            Wt::WVBoxLayout* main_layout_;

            struct ControlsContainer : Wt::WContainerWidget
            {
                ControlsContainer(Wt::WTimer* timer,
                                  Wt::WContainerWidget* parent = 0);

                void handle_play_pause(bool toggle_state);

                
                Wt::WTimer* timer_;

                Wt::WPushButton* play_pause_button_;
                Wt::WText* play_state_;
            };
            
            ControlsContainer* controls_div_;

            struct SubscriptionsContainer : Wt::WContainerWidget
            {
                SubscriptionsContainer(MOOSNode* node,
                                       Wt::WStandardItemModel* model,
                                       Wt::WStringListModel* history_model,
                                       std::map<std::string, int>& msg_map,
                                       Wt::WContainerWidget* parent = 0);

                void handle_add_subscription();
                void handle_remove_subscription(Wt::WPushButton* clicked_anchor);
                void add_subscription(std::string type);

                MOOSNode* node_;
                
                Wt::WStandardItemModel* model_;
                Wt::WStringListModel* history_model_;
                std::map<std::string, int>& msg_map_;
            
                Wt::WText* add_text_;
                Wt::WLineEdit* subscribe_filter_text_;
                Wt::WPushButton* subscribe_filter_button_;
                Wt::WBreak* subscribe_break_;
                Wt::WText* remove_text_;
            };

            SubscriptionsContainer* subscriptions_div_;

            struct HistoryContainer : Wt::WContainerWidget
            {
                HistoryContainer(MOOSNode* node,
                                 Wt::WVBoxLayout* main_layout,
                                 Wt::WAbstractItemModel* model,
                                 const protobuf::MOOSScopeConfig& moos_scope_config,
                                 Wt::WContainerWidget* parent = 0);

                void handle_add_history();
                void handle_remove_history(std::string type);
                void add_history(const goby::common::protobuf::MOOSScopeConfig::HistoryConfig& config);
                void toggle_history_plot(Wt::WWidget* plot);


                struct MVC
                {
                    std::string key;
                    Wt::WContainerWidget* container;
                    Wt::WStandardItemModel* model;
                    Wt::WTreeView* tree;
                    Wt::WSortFilterProxyModel* proxy;
                };
            

                MOOSNode* node_;
                Wt::WVBoxLayout* main_layout_;
                
                const protobuf::MOOSScopeConfig& moos_scope_config_;
                std::map<std::string, MVC> history_models_;                
                Wt::WText* hr_;
                Wt::WText* add_text_;
                Wt::WComboBox* history_box_;
                Wt::WPushButton* history_button_;
            };
            
            HistoryContainer* history_header_div_;

            struct RegexFilterContainer : Wt::WContainerWidget
            {
                RegexFilterContainer(
                    Wt::WStandardItemModel* model,
                    Wt::WSortFilterProxyModel* proxy,
                    const protobuf::MOOSScopeConfig& moos_scope_config,
                    Wt::WContainerWidget* parent = 0);

                void handle_set_regex_filter();
                void handle_clear_regex_filter();
                void toggle_regex_examples_table();
                
                Wt::WStandardItemModel* model_;
                Wt::WSortFilterProxyModel* proxy_;
                
                Wt::WText* hr_;
                Wt::WText* set_text_;
                Wt::WComboBox* regex_column_select_;
                Wt::WText* expression_text_;
                Wt::WLineEdit* regex_filter_text_;
                Wt::WPushButton* regex_filter_button_;
                Wt::WPushButton* regex_filter_clear_;
                Wt::WPushButton* regex_filter_examples_;

                Wt::WBreak* break_;
                Wt::WTable* regex_examples_table_;

            };
            
            RegexFilterContainer* regex_filter_div_;


            Wt::WTreeView* scope_tree_view_;

            // maps CMOOSMsg::GetKey into row
            std::map<std::string, int> msg_map_;

            WContainerWidget* bottom_fill_;

            
            
        
        };

      class LiaisonScopeMOOSTreeView : public Wt::WTreeView
        {
          public:
            LiaisonScopeMOOSTreeView(const protobuf::MOOSScopeConfig& moos_scope_config, Wt::WContainerWidget* parent = 0);

          private:
            
            //           void handle_double_click(const Wt::WModelIndex& index, const Wt::WMouseEvent& event);

            
        };

        class LiaisonScopeMOOSModel : public Wt::WStandardItemModel
        {
          public:
            LiaisonScopeMOOSModel(const protobuf::MOOSScopeConfig& moos_scope_config, Wt::WContainerWidget* parent = 0); 
        };


    }
}

#endif
