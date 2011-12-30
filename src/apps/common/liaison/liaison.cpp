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


#include <Wt/WText>
#include <Wt/WHBoxLayout>
#include <Wt/WVBoxLayout>
#include <Wt/WStackedWidget>
#include <Wt/WImage>
#include <Wt/WAnchor>
#include <Wt/WTimer>

#include "goby/util/time.h"

#include "liaison.h"
#include "liaison_scope.h"
#include "liaison_home.h"

using goby::glog;
using namespace Wt;    
using namespace goby::util::logger_lock;
using namespace goby::util::logger;

goby::common::protobuf::LiaisonConfig goby::common::Liaison::cfg_;
boost::shared_ptr<zmq::context_t> goby::common::Liaison::zmq_context_(new zmq::context_t(1));
const std::string goby::common::Liaison::LIAISON_INTERNAL_PUBLISH_SOCKET_NAME = "liaison_internal_publish_socket"; 
const std::string goby::common::Liaison::LIAISON_INTERNAL_SUBSCRIBE_SOCKET_NAME = "liaison_internal_subscribe_socket"; 

int main(int argc, char* argv[])
{
    return goby::run<goby::common::Liaison>(argc, argv);
}

goby::common::Liaison::Liaison()
    : ZeroMQApplicationBase(&zeromq_service_, &cfg_),
      zeromq_service_(zmq_context_),
      pubsub_node_(&zeromq_service_, cfg_.base().pubsub_config())
{
    try
    {
        // create a set of fake argc / argv for Wt::WServer
        std::vector<std::string> wt_argv_vec;  
        std::string str = cfg_.base().app_name() + " --docroot " + cfg_.docroot() + " --http-port " + goby::util::as<std::string>(cfg_.http_port()) + " --http-address " + cfg_.http_address();
        boost::split(wt_argv_vec, str, boost::is_any_of(" "));
        
        char* wt_argv[wt_argv_vec.size()];
        
        glog << "setting Wt cfg to: ";
        for(int i = 0, n = wt_argv_vec.size(); i < n; ++i)
        {
            wt_argv[i] = new char[wt_argv_vec[i].size() + 1];
            strcpy(wt_argv[i], wt_argv_vec[i].c_str());
            glog << "\t" << wt_argv[i] << std::endl;
        }
        
        wt_server_.setServerConfiguration(wt_argv_vec.size(), wt_argv);

        // delete our fake argv
        for(int i = 0, n = wt_argv_vec.size(); i < n; ++i)
            delete[] wt_argv[i];

        
        wt_server_.addEntryPoint(Wt::Application,
                                 goby::common::create_wt_application);

        if (!wt_server_.start())
        {
            glog.is(DIE) && glog << "Could not start Wt HTTP server." << std::endl;
        }
    }
    catch (Wt::WServer::Exception& e)
    {
        glog.is(DIE) && glog << "Could not start Wt HTTP server. Exception: " << e.what() << std::endl;
    }


    pubsub_node_.subscribe_all();
    zeromq_service_.connect_inbox_slot(&Liaison::inbox, this);

    protobuf::ZeroMQServiceConfig ipc_sockets;
    protobuf::ZeroMQServiceConfig::Socket* internal_publish_socket = ipc_sockets.add_socket();
    internal_publish_socket->set_socket_type(protobuf::ZeroMQServiceConfig::Socket::PUBLISH);
    internal_publish_socket->set_socket_id(LIAISON_INTERNAL_PUBLISH_SOCKET);
    internal_publish_socket->set_transport(protobuf::ZeroMQServiceConfig::Socket::INPROC);
    internal_publish_socket->set_connect_or_bind(protobuf::ZeroMQServiceConfig::Socket::BIND);
    internal_publish_socket->set_socket_name(LIAISON_INTERNAL_PUBLISH_SOCKET_NAME);

    zeromq_service_.merge_cfg(ipc_sockets);
}

void goby::common::Liaison::loop()
{
}

void goby::common::Liaison::inbox(MarshallingScheme marshalling_scheme,
                                const std::string& identifier,
                                const void* data,
                                int size,
                                int socket_id)
{
    glog.is(DEBUG2, lock) && glog << "Liaison: got message with identifier: " << identifier << std::endl << unlock;    
    zeromq_service_.send(marshalling_scheme, identifier, data, size, LIAISON_INTERNAL_PUBLISH_SOCKET);
}


goby::common::LiaisonWtThread::LiaisonWtThread(const Wt::WEnvironment& env)
    : Wt::WApplication(env),
      zeromq_service_(Liaison::zmq_context())
{    
    timer_ = new Wt::WTimer();
    timer_->setInterval(1/Liaison::cfg_.update_freq()*1.0e3);
    timer_->timeout().connect(this, &LiaisonWtThread::loop);
    timer_->start();

//    zeromq_service_.connect_inbox_slot(&LiaisonWtThread::inbox, this);

    protobuf::ZeroMQServiceConfig ipc_sockets;
    protobuf::ZeroMQServiceConfig::Socket* internal_publish_socket = ipc_sockets.add_socket();
    internal_publish_socket->set_socket_type(protobuf::ZeroMQServiceConfig::Socket::SUBSCRIBE);
    internal_publish_socket->set_socket_id(Liaison::LIAISON_INTERNAL_PUBLISH_SOCKET);
    internal_publish_socket->set_transport(protobuf::ZeroMQServiceConfig::Socket::INPROC);
    internal_publish_socket->set_connect_or_bind(protobuf::ZeroMQServiceConfig::Socket::CONNECT);
    internal_publish_socket->set_socket_name(Liaison::LIAISON_INTERNAL_PUBLISH_SOCKET_NAME);

    zeromq_service_.merge_cfg(ipc_sockets);    
//    zeromq_service_.subscribe(MARSHALLING_MOOS, "CMOOSMsg/DB", Liaison::LIAISON_INTERNAL_PUBLISH_SOCKET);



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
    
    add_to_menu(menu, "Home", new LiaisonHome());
    add_to_menu(menu, "Scope", new LiaisonScope(&zeromq_service_, timer_));
//    add_to_menu(menu, "scope2", new LiaisonScope("hello 2"));


    

    
//    WVBoxLayout *vertLayout1 = new WVBoxLayout(root());
//    WHBoxLayout *horizLayout1 = new WHBoxLayout; 
//    WVBoxLayout *vertLayout2 = new WVBoxLayout;
//    WHBoxLayout *horizLayout2 = new WHBoxLayout;

    
//     vertLayout1->addWidget(header, 0, AlignCenter | AlignTop);
//     vertLayout1->addLayout(horizLayout1, 1);
// //    vertLayout1->setResizable(0, true);
// //    vertLayout1->setResizable(1, true);
//     horizLayout1->addWidget(menu);
//     horizLayout1->setResizable(0, true);
//     horizLayout1->addLayout(vertLayout2, 1);
// //    vertLayout2->addWidget(contents_stack_, 0, AlignTop);
//     vertLayout2->addWidget(contents_stack_, 0, AlignJustify);
    
    
//     vertLayout1->addLayout(horizLayout2, 0);
//     horizLayout2->addWidget(goby_lp_image_a, 0, AlignLeft | AlignBottom);
//     horizLayout2->addWidget(goby_logo_a, 0, AlignRight | AlignBottom);

//     root()->resize(WLength::Auto, 600);
//     Wt::WGridLayout* grid_layout = new Wt::WGridLayout(root());
    
//     grid_layout->addWidget(header, 0, 0, 3, 1, AlignCenter | AlignTop);
// //    grid_layout->addWidget(new Wt::WText("Item 0 1"), 0, 1);
//     grid_layout->addWidget(menu, 1, 0);
//     grid_layout->addWidget(contents_stack_, 1,1, 2,1);

}

void goby::common::LiaisonWtThread::add_to_menu(WMenu* menu, const WString& name,
                                                            LiaisonContainer* container)
{
    menu->addItem(name, container);
    container->set_name(name);
}

void goby::common::LiaisonWtThread::loop()
{

    glog.is(DEBUG2, lock) && glog << "LiaisonWtThread: polling" << std::endl << unlock;
    while(zeromq_service_.poll(0))
    { }

    
}

void goby::common::LiaisonWtThread::inbox(MarshallingScheme marshalling_scheme,
                                                      const std::string& identifier,
                                                      const void* data,
                                                      int size,
                                                      int socket_id)
{
    glog.is(DEBUG1, lock) && glog << "LiaisonWtThread: got message with identifier: " << identifier << std::endl << unlock;
    
}

