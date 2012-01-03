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

#ifndef LIAISONCOMMANDER20110609H
#define LIAISONCOMMANDER20110609H

#include <Wt/WText>
#include <Wt/WString>
#include <Wt/WCssDecorationStyle>
#include <Wt/WBorder>
#include <Wt/WColor>
#include <Wt/WComboBox>
#include <Wt/WVBoxLayout>
#include <Wt/WStackedWidget>
#include <Wt/WTreeTableNode>
#include <Wt/WTreeTable>
#include <Wt/WLineEdit>
#include <Wt/WLabel>
#include <Wt/WSpinBox>
#include <Wt/WStringListModel>
#include <Wt/WAbstractListModel>
#include <Wt/WPushButton>

#include "liaison.h"
#include "goby/moos/moos_node.h"

namespace goby
{
    namespace common
    {
        class LiaisonCommander : public LiaisonContainer, public goby::moos::MOOSNode
        {
          public:
            LiaisonCommander(ZeroMQService* service, Wt::WContainerWidget* parent = 0);
            void moos_inbox(CMOOSMsg& msg) { }

          private:
            
          private:
            const protobuf::ProtobufCommanderConfig& pb_commander_config_;
            
            Wt::WVBoxLayout* main_layout_;
            Wt::WStackedWidget* commands_div_;
            
            struct ControlsContainer : Wt::WContainerWidget
            {
                ControlsContainer(const protobuf::ProtobufCommanderConfig& pb_commander_config,
                                  Wt::WStackedWidget* commands_div,
                                  Wt::WContainerWidget* parent = 0);
                void switch_command(int selection_index);

                struct CommandContainer : Wt::WContainerWidget
                {
                    CommandContainer(const protobuf::ProtobufCommanderConfig& pb_commander_config,
                                     boost::shared_ptr<google::protobuf::Message> message);

                    void generate_tree(Wt::WTreeTableNode* parent,
                                       google::protobuf::Message* message);
                    void generate_tree_field(Wt::WTreeTableNode* parent,
                                             google::protobuf::Message* message,
                                             const google::protobuf::FieldDescriptor* field_desc);


                    
                    Wt::WLineEdit* generate_single_line_edit_field(
                        google::protobuf::Message* message,
                        const google::protobuf::FieldDescriptor* field_desc,
                        const std::string& default_value,
                        int index = -1);
                    
                    Wt::WComboBox* generate_combo_box_field(
                        google::protobuf::Message* message,
                        const google::protobuf::FieldDescriptor* field_desc,
                        const std::vector< Wt::WString >& strings,
                        const std::string& default_value,
                        int index = -1);

                    void handle_toggle_single_message(
                        const Wt::WMouseEvent& mouse,
                        google::protobuf::Message* message,
                        const google::protobuf::FieldDescriptor* field_desc,
                        Wt::WPushButton* field,
                        Wt::WTreeTableNode* parent);
                    
                    
                    void handle_line_field_changed(
                        google::protobuf::Message* message,
                        const google::protobuf::FieldDescriptor* field_desc,
                        Wt::WLineEdit* field,
                        int index);
                    
                    void handle_combo_field_changed(
                        google::protobuf::Message* message,
                        const google::protobuf::FieldDescriptor* field_desc,
                        Wt::WComboBox* field,
                        int index);
                    
                    void handle_repeated_size_change(
                        int size,
                        google::protobuf::Message* message,
                        const google::protobuf::FieldDescriptor* field_desc,
                        Wt::WTreeTableNode* parent);
                    
                    boost::shared_ptr<google::protobuf::Message> message_;
                };

                const protobuf::ProtobufCommanderConfig& pb_commander_config_;
                std::map<std::string, int> commands_;
                Wt::WComboBox* command_selection_;
                Wt::WStackedWidget* commands_div_;

            };
            
            ControlsContainer* controls_div_;
        };
    }
}

#endif
