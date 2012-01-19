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

#include <dlfcn.h>

#include <Wt/WText>
#include <Wt/WHBoxLayout>
#include <Wt/WVBoxLayout>
#include <Wt/WStackedWidget>
#include <Wt/WImage>
#include <Wt/WAnchor>

#include "goby/util/time.h"
#include "goby/util/dynamic_protobuf_manager.h"

#include "liaison_wt_thread.h"
#include "liaison_scope.h"
#include "liaison_home.h"
#include "liaison_commander.h"

using goby::glog;
using namespace Wt;    
using namespace goby::util::logger_lock;
using namespace goby::util::logger;

goby::common::LiaisonWtThread::LiaisonWtThread(const Wt::WEnvironment& env)
    : Wt::WApplication(env),
      scope_service_(Liaison::zmq_context()),
      commander_service_(Liaison::zmq_context()),
      last_scope_timer_state_(UNKNOWN)
{    

    
//    zeromq_service_.connect_inbox_slot(&LiaisonWtThread::inbox, this);


    Wt::WString title_text("goby liaison: " + Liaison::cfg_.base().platform_name());
    setTitle(title_text);

    useStyleSheet(std::string("css/fonts.css?" + util::goby_file_timestamp()));
    useStyleSheet(std::string("css/liaison.css?" + util::goby_file_timestamp()));
//    setCssTheme("");
    

    root()->setId("main");
    
    /*
     * Set up the title
     */
    WContainerWidget* header_div = new WContainerWidget(root());
    header_div->setId("header");
    
    WImage* goby_lp_image = new WImage("images/goby-lp.png");
    WImage* goby_logo = new WImage("images/gobysoft_logo_dot_org_small.png");

    
    WAnchor* goby_lp_image_a = new WAnchor("https://launchpad.net/goby", goby_lp_image, header_div);
    WText* header = new WText(title_text, header_div);
    header->setId("header");

    WAnchor* goby_logo_a = new WAnchor("http://gobysoft.org/#/software/goby", goby_logo, header_div);
    goby_lp_image_a->setId("lp_logo");
    goby_logo_a->setId("goby_logo");
    goby_lp_image_a->setStyleClass("no_ul");
    goby_logo_a->setStyleClass("no_ul");
    goby_lp_image_a->setTarget(TargetNewWindow);
    goby_logo_a->setTarget(TargetNewWindow);
    

    new WText("<hr/>", root());

    
    WContainerWidget* menu_div = new WContainerWidget(root());
    menu_div->setStyleClass("menu");

    WContainerWidget* contents_div = new WContainerWidget(root());
    contents_div->setId("contents");
    contents_stack_ = new WStackedWidget(contents_div);
    contents_stack_->setStyleClass("fill");
    
    /*
     * Setup the menu
     */
    WMenu *menu = new WMenu(contents_stack_, Vertical, menu_div);
    menu->setRenderAsList(true);
    menu->setStyleClass("menu");
    menu->setInternalPathEnabled();
    menu->setInternalBasePath("/");
    
    add_to_menu(menu, "Home", new LiaisonHome);

    LiaisonCommander* commander = new LiaisonCommander(&commander_service_);
    commander_timer_.setInterval(1/Liaison::cfg_.update_freq()*1.0e3);
    commander_timer_.timeout().connect(commander, &LiaisonCommander::loop);
    
    add_to_menu(menu, "Commander", commander); 

    LiaisonScope* scope = new LiaisonScope(&scope_service_, &scope_timer_);
    scope_timer_.setInterval(1/Liaison::cfg_.update_freq()*1.0e3);
    scope_timer_.timeout().connect(scope, &LiaisonScope::loop);

    add_to_menu(menu, "Scope", scope);

    if(!Liaison::cfg_.start_paused())
        scope_timer_.start();
    
    menu->itemSelected().connect(this, &LiaisonWtThread::handle_menu_selection);


    handle_menu_selection(menu->currentItem());
}

void goby::common::LiaisonWtThread::add_to_menu(WMenu* menu, const WString& name,
                                                LiaisonContainer* container)
{
    menu->addItem(name, container);
    container->set_name(name);
}

void goby::common::LiaisonWtThread::handle_menu_selection(Wt::WMenuItem * item)
{    
    std::cout << "Item selected: " << item->text() << std::endl;
    std::cout << "Timer state: " <<  last_scope_timer_state_ << std::endl;

    if(item->text() == "Commander")
        commander_timer_.start();
    else
        commander_timer_.stop();
    
    if(item->text() == "Scope")
    {
        if(last_scope_timer_state_ == ACTIVE)
            scope_timer_.start();
        last_scope_timer_state_ = UNKNOWN;
    }
    else
    {
        if(last_scope_timer_state_ == UNKNOWN)
        {
            last_scope_timer_state_ = scope_timer_.isActive() ? ACTIVE : STOPPED;
            scope_timer_.stop();
        }
    }
}


void goby::common::LiaisonWtThread::inbox(MarshallingScheme marshalling_scheme,
                                                      const std::string& identifier,
                                                      const void* data,
                                                      int size,
                                                      int socket_id)
{
    glog.is(DEBUG1, lock) && glog << "LiaisonWtThread: got message with identifier: " << identifier << std::endl << unlock;
}

