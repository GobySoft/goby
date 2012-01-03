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

#include "goby/util/dynamic_protobuf_manager.h"

#include "liaison_commander.h"

using namespace Wt;
using namespace goby::util::logger;
using goby::util::logger_lock::lock;
using goby::glog;

const std::string MESSAGE_INCLUDE_TEXT = "include";
const std::string MESSAGE_REMOVE_TEXT = "remove";


goby::common::LiaisonCommander::LiaisonCommander(ZeroMQService* service,
                                                 Wt::WContainerWidget* parent)
    : LiaisonContainer(parent),
      MOOSNode(service),
      pb_commander_config_(Liaison::cfg_.GetExtension(protobuf::pb_commander_config)),
      main_layout_(new Wt::WVBoxLayout(this)),
      commands_div_(new WStackedWidget),
      controls_div_(new ControlsContainer(pb_commander_config_, commands_div_))
{
    main_layout_->addWidget(controls_div_);
    main_layout_->addWidget(commands_div_);
    main_layout_->addStretch(1);

    
    // send(CMOOSMsg(MOOS_NOTIFY, "FOO", "BAR"), Liaison::LIAISON_INTERNAL_SUBSCRIBE_SOCKET);
}


goby::common::LiaisonCommander::ControlsContainer::ControlsContainer(
    const protobuf::ProtobufCommanderConfig& pb_commander_config,
    Wt::WStackedWidget* commands_div,
    Wt::WContainerWidget* parent /*=0*/)
    : pb_commander_config_(pb_commander_config),
      command_selection_(new WComboBox(this)),
      commands_div_(commands_div)
{
    
    command_selection_->addItem("(Select a command message)");

    
    for(int i = 0, n = pb_commander_config.load_protobuf_name_size(); i < n; ++i)
    {
        const google::protobuf::Descriptor* desc =
            goby::util::DynamicProtobufManager::descriptor_pool().FindMessageTypeByName(
                pb_commander_config.load_protobuf_name(i));

        if(!desc)
            glog.is(WARN, lock) && glog << "Could not find protobuf name " << pb_commander_config.load_protobuf_name(i) << " to load for Protobuf Commander (configuration line `load_protobuf_name`)" << std::endl << unlock;
        else
            command_selection_->addItem(pb_commander_config.load_protobuf_name(i));
    }    
    
    command_selection_->activated().connect(this, &ControlsContainer::switch_command);
}

void goby::common::LiaisonCommander::ControlsContainer::switch_command(int selection_index)
{
    if(selection_index == 0)
        return;

    std::string protobuf_name = command_selection_->itemText(selection_index).narrow();

    if(!commands_.count(protobuf_name))
    {
        CommandContainer* new_command = new CommandContainer(pb_commander_config_,
            goby::util::DynamicProtobufManager::new_protobuf_message(protobuf_name));
        commands_div_->addWidget(new_command);
        // index of the newly added widget
        commands_[protobuf_name] = commands_div_->count() - 1;
    }
    commands_div_->setCurrentIndex(commands_[protobuf_name]);
}

goby::common::LiaisonCommander::ControlsContainer::CommandContainer::CommandContainer(
    const protobuf::ProtobufCommanderConfig& pb_commander_config,
    boost::shared_ptr<google::protobuf::Message> message)
    : message_(message)
{
    const google::protobuf::Descriptor* desc = message_->GetDescriptor();
    
    Wt::WTreeTable *treeTable = new Wt::WTreeTable(this);

    treeTable->addColumn("Value", pb_commander_config.value_width_pixels());
    treeTable->addColumn("Modify", pb_commander_config.modify_width_pixels());

    // Create and set the root node
    Wt::WTreeTableNode *root = new Wt::WTreeTableNode(desc->name());
    root->setImagePack("resources/");
    treeTable->setTreeRoot(root, "Field");

    generate_tree(root, message_.get());


    root->expand();
}

void goby::common::LiaisonCommander::ControlsContainer::CommandContainer::generate_tree(
    Wt::WTreeTableNode* parent,
    google::protobuf::Message* message)
{
    const google::protobuf::Descriptor* desc = message->GetDescriptor();
    
    for(int i = 0, n = desc->field_count(); i < n; ++i)
        generate_tree_field(parent, message, desc->field(i));
    
    std::vector<const google::protobuf::FieldDescriptor*> extensions;
    goby::util::DynamicProtobufManager::descriptor_pool().FindAllExtensions(desc, &extensions);
    for(int i = 0, n = extensions.size(); i < n; ++i)
        generate_tree_field(parent, message, extensions[i]);

}

void goby::common::LiaisonCommander::ControlsContainer::CommandContainer::generate_tree_field(
    Wt::WTreeTableNode* parent,
    google::protobuf::Message* message,
    const google::protobuf::FieldDescriptor* field_desc)
{
    Wt::WTreeTableNode* node = new Wt::WTreeTableNode(field_desc->name(), 0, parent);
    Wt::WInteractWidget* value_field = 0;
    Wt::WInteractWidget* modify_field = 0;
    if(field_desc->is_repeated())
    {
        Wt::WContainerWidget* div = new Wt::WContainerWidget;
//        WLabel* label = new WLabel(": ", div); 
        WSpinBox* spin_box = new WSpinBox(div);
        spin_box->setTextSize(3);
        //       label->setBuddy(spin_box);
        spin_box->setRange(0, std::numeric_limits<int>::max());
        spin_box->setSingleStep(1);

        spin_box->valueChanged().connect(
            boost::bind(&CommandContainer::handle_repeated_size_change,
                        this, _1, message, field_desc, node));
        
        modify_field = div; 
    }    
    else
    {
        switch(field_desc->cpp_type())
        {
            case google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE:
            {   
                Wt::WContainerWidget* div = new Wt::WContainerWidget;
                WPushButton* button = new WPushButton(MESSAGE_INCLUDE_TEXT, div); 

                button->clicked().connect(
                    boost::bind(&CommandContainer::handle_toggle_single_message,
                                this, _1, message, field_desc, button, node));
                
                modify_field = div;
            }
            break;
                
            case google::protobuf::FieldDescriptor::CPPTYPE_INT32:
                value_field = generate_single_line_edit_field(
                    message,
                    field_desc,
                    goby::util::as<std::string>(field_desc->default_value_int32()));
                break;
                    
            case google::protobuf::FieldDescriptor::CPPTYPE_INT64:
                value_field = generate_single_line_edit_field(
                    message,
                    field_desc,
                    goby::util::as<std::string>(field_desc->default_value_int64()));
                break;

            case google::protobuf::FieldDescriptor::CPPTYPE_UINT32:
                value_field = generate_single_line_edit_field(
                    message,
                    field_desc,
                    goby::util::as<std::string>(field_desc->default_value_uint32()));
                break;

            case google::protobuf::FieldDescriptor::CPPTYPE_UINT64:
                value_field = generate_single_line_edit_field(
                    message,
                    field_desc,
                    goby::util::as<std::string>(field_desc->default_value_uint64()));
                break;

            
            case google::protobuf::FieldDescriptor::CPPTYPE_STRING:
                value_field = generate_single_line_edit_field(
                    message,
                    field_desc,
                    goby::util::as<std::string>(field_desc->default_value_string()));
                break;
                
            case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT:
                value_field = generate_single_line_edit_field(
                    message,
                    field_desc,
                    goby::util::as<std::string>(field_desc->default_value_float()));
                break;
                
            case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE:
                value_field = generate_single_line_edit_field(
                    message,
                    field_desc,
                    goby::util::as<std::string>(field_desc->default_value_double()));
                break;

                        
            case google::protobuf::FieldDescriptor::CPPTYPE_BOOL:
            {
                std::vector<WString> strings;
                strings.push_back("true");
                strings.push_back("false");
                
                value_field = generate_combo_box_field(
                    message,
                    field_desc,
                    strings,
                    goby::util::as<std::string>(field_desc->default_value_bool())); 
            }
            break;

                
            case google::protobuf::FieldDescriptor::CPPTYPE_ENUM:
            {
                std::vector<WString> strings;

                const google::protobuf::EnumDescriptor* enum_desc = field_desc->enum_type();
                
                for(int i = 0, n = enum_desc->value_count(); i < n; ++i)
                    strings.push_back(enum_desc->value(i)->name());                
                
                value_field = generate_combo_box_field(
                    message,
                    field_desc,
                    strings,
                    goby::util::as<std::string>(field_desc->default_value_enum()->name()));
            } 
            break;
            
        }
        
    }
    if(value_field)
        node->setColumnWidget(1, value_field);

    if(modify_field)
        node->setColumnWidget(2, modify_field);
}

void goby::common::LiaisonCommander::ControlsContainer::CommandContainer::handle_line_field_changed(google::protobuf::Message* message, const google::protobuf::FieldDescriptor* field_desc, Wt::WLineEdit* field, int index)
{
    std::string value = field->text().narrow();

    const google::protobuf::Reflection* refl = message->GetReflection();
    
    
    if(value.empty())
        refl->ClearField(message, field_desc);
    else
    {
        switch(field_desc->cpp_type())
        {
            case google::protobuf::FieldDescriptor::CPPTYPE_INT32:
                field_desc->is_repeated() ?
                    refl->SetRepeatedInt32(message, field_desc, index, goby::util::as<int32>(value)) :
                    refl->SetInt32(message, field_desc, goby::util::as<int32>(value));
                break;
                    
            case google::protobuf::FieldDescriptor::CPPTYPE_INT64:
                field_desc->is_repeated() ?
                    refl->SetRepeatedInt64(message, field_desc, index, goby::util::as<int64>(value)) :
                    refl->SetInt64(message, field_desc, goby::util::as<int64>(value));
                break;

            case google::protobuf::FieldDescriptor::CPPTYPE_UINT32:
                field_desc->is_repeated() ?
                    refl->SetRepeatedUInt32(message, field_desc, index, goby::util::as<uint32>(value)) :
                    refl->SetUInt32(message, field_desc, goby::util::as<uint32>(value));
                break;
                    
            case google::protobuf::FieldDescriptor::CPPTYPE_UINT64:
                field_desc->is_repeated() ?
                    refl->SetRepeatedUInt64(message, field_desc, index, goby::util::as<uint64>(value)) :
                    refl->SetUInt64(message, field_desc, goby::util::as<uint64>(value));
                break;
                    
            case google::protobuf::FieldDescriptor::CPPTYPE_STRING:
                field_desc->is_repeated() ?
                    refl->SetRepeatedString(message, field_desc, index, value) :
                    refl->SetString(message, field_desc, value);
                break;                    
                
            case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT:
                field_desc->is_repeated() ?
                    refl->SetRepeatedFloat(message, field_desc, index, goby::util::as<float>(value)) :
                    refl->SetFloat(message, field_desc, goby::util::as<float>(value));
                break;
                
            case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE:
                field_desc->is_repeated() ?
                    refl->SetRepeatedDouble(message, field_desc, index, goby::util::as<double>(value)) :
                    refl->SetDouble(message, field_desc, goby::util::as<double>(value));
                break;

            default:
                break;
        }
    }
    
    glog.is(DEBUG1, lock) && glog << "The message is: " << message_->DebugString() <<  std::endl << unlock;
}


void goby::common::LiaisonCommander::ControlsContainer::CommandContainer::handle_combo_field_changed(google::protobuf::Message* message, const google::protobuf::FieldDescriptor* field_desc, Wt::WComboBox* field, int index)
{
    const google::protobuf::Reflection* refl = message->GetReflection();
    
    
    if(field->currentIndex() == 0)
        refl->ClearField(message, field_desc);
    else
    {
        std::string value = field->currentText().narrow();

        switch(field_desc->cpp_type())
        {
            case google::protobuf::FieldDescriptor::CPPTYPE_BOOL:
                field_desc->is_repeated() ?
                    refl->SetRepeatedBool(message, field_desc, index, goby::util::as<bool>(value)) :
                    refl->SetBool(message, field_desc, goby::util::as<bool>(value));
                break;
                    
            case google::protobuf::FieldDescriptor::CPPTYPE_ENUM:
                field_desc->is_repeated() ?
                    refl->SetRepeatedEnum(message, field_desc, index, field_desc->enum_type()->FindValueByName(value)) :
                    refl->SetEnum(message, field_desc, field_desc->enum_type()->FindValueByName(value));
                break;

            default:
                break;    
        }
    }
    glog.is(DEBUG1, lock) && glog << "The message is: " << message_->DebugString() <<  std::endl << unlock;
}

                    

Wt::WLineEdit* goby::common::LiaisonCommander::ControlsContainer::CommandContainer::generate_single_line_edit_field(
    google::protobuf::Message* message,
    const google::protobuf::FieldDescriptor* field_desc,
    const std::string& default_value,
    int index /*= -1*/)
{
    Wt::WLineEdit* line_edit = new Wt::WLineEdit();
    if(field_desc->has_default_value() || field_desc->is_repeated())
        line_edit->setEmptyText(default_value);
    
    line_edit->changed().connect(boost::bind(&CommandContainer::handle_line_field_changed,
                                             this, message, field_desc, line_edit, index));
    return line_edit;
}

 Wt::WComboBox* goby::common::LiaisonCommander::ControlsContainer::CommandContainer::generate_combo_box_field(
    google::protobuf::Message* message,
    const google::protobuf::FieldDescriptor* field_desc,
    const std::vector< Wt::WString >& strings,
    const std::string& default_value,
    int index /*= -1*/)
{
    WComboBox* combo_box = new WComboBox;
    Wt::WStringListModel* model = new Wt::WStringListModel(strings, this); 

    if(field_desc->has_default_value())
        model->insertString(0, "(default: " + default_value + ")");
    else
        model->insertString(0, "");

    combo_box->setModel(model);
    
    combo_box->changed().connect(
        boost::bind(&CommandContainer::handle_combo_field_changed,
                    this, message, field_desc, combo_box, index));

    return combo_box;
}


void goby::common::LiaisonCommander::ControlsContainer::CommandContainer::handle_repeated_size_change(
    int desired_size,
    google::protobuf::Message* message,
    const google::protobuf::FieldDescriptor* field_desc,
    Wt::WTreeTableNode* parent)
{
    const google::protobuf::Reflection* refl = message->GetReflection();
    
    // add nodes
    while(desired_size > static_cast<int>(parent->childNodes().size()))
    {
        int index = parent->childNodes().size();
        Wt::WTreeTableNode* node =
            new Wt::WTreeTableNode("index: " + goby::util::as<std::string>(index), 0, parent);
        Wt::WLineEdit* line_edit = 0;
        
        switch(field_desc->cpp_type())
        {
            case google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE:
                generate_tree(node,
                              refl->AddMessage(message, field_desc));
                break;

            case google::protobuf::FieldDescriptor::CPPTYPE_INT32:
                refl->AddInt32(message, field_desc, field_desc->default_value_int32());
                line_edit = generate_single_line_edit_field(
                    message,
                    field_desc,
                    goby::util::as<std::string>(field_desc->default_value_int32()),
                    index);
                break;
                
            case google::protobuf::FieldDescriptor::CPPTYPE_INT64:
                refl->AddInt64(message, field_desc, field_desc->default_value_int64());
                line_edit = generate_single_line_edit_field(
                    message,
                    field_desc,
                    goby::util::as<std::string>(field_desc->default_value_int64()),
                    index);
                break;
                
            case google::protobuf::FieldDescriptor::CPPTYPE_UINT32:
                refl->AddUInt32(message, field_desc, field_desc->default_value_uint32());
                line_edit = generate_single_line_edit_field(
                    message,
                    field_desc,
                    goby::util::as<std::string>(field_desc->default_value_uint32()),
                    index);
                break;

            case google::protobuf::FieldDescriptor::CPPTYPE_UINT64:
                refl->AddUInt64(message, field_desc, field_desc->default_value_uint64());
                line_edit = generate_single_line_edit_field(
                    message,
                    field_desc,
                    goby::util::as<std::string>(field_desc->default_value_uint64()),
                    index);
                break;
                
            case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT:
                refl->AddFloat(message, field_desc, field_desc->default_value_float());
                line_edit = generate_single_line_edit_field(
                    message,
                    field_desc,
                    goby::util::as<std::string>(field_desc->default_value_float()),
                    index);
                break;
                
            case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE:
                refl->AddDouble(message, field_desc, field_desc->default_value_double());
                line_edit = generate_single_line_edit_field(
                    message,
                    field_desc,
                    goby::util::as<std::string>(field_desc->default_value_double()),
                    index);
                break;
                    
            case google::protobuf::FieldDescriptor::CPPTYPE_STRING:
                refl->AddString(message, field_desc, field_desc->default_value_string());
                line_edit = generate_single_line_edit_field(
                    message,
                    field_desc,
                    goby::util::as<std::string>(field_desc->default_value_string()),
                    index);
                break;
                
            case google::protobuf::FieldDescriptor::CPPTYPE_ENUM:
                refl->AddEnum(message, field_desc, field_desc->default_value_enum());
                break;
        
            case google::protobuf::FieldDescriptor::CPPTYPE_BOOL:
                refl->AddBool(message, field_desc, field_desc->default_value_bool());
                break;
                
        }

        if(line_edit)
            node->setColumnWidget(1, line_edit);
        parent->expand();
        
    }

    // remove nodes
    while(desired_size < static_cast<int>(parent->childNodes().size()))
    {
        parent->removeChildNode(parent->childNodes().back());
        refl->RemoveLast(message, field_desc);
    }    
}


void goby::common::LiaisonCommander::ControlsContainer::CommandContainer::handle_toggle_single_message(
    const Wt::WMouseEvent& mouse,
    google::protobuf::Message* message,
    const google::protobuf::FieldDescriptor* field_desc,
    Wt::WPushButton* button,
    Wt::WTreeTableNode* parent)
{
    if(button->text() == MESSAGE_INCLUDE_TEXT)
    {
        generate_tree(parent,
                      message->GetReflection()->MutableMessage(message, field_desc));
        
        parent->expand();
        
        button->setText(MESSAGE_REMOVE_TEXT);
    }
    else
    {
        const std::vector< WTreeNode * > children = parent->childNodes();
        message->GetReflection()->ClearField(message, field_desc);
        for(int i = 0, n = children.size(); i < n; ++i)
            parent->removeChildNode(children[i]);        
        
        button->setText(MESSAGE_INCLUDE_TEXT);
    }
    
}
