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

goby::core::protobuf::LiaisonConfig goby::core::Liaison::cfg_;
boost::shared_ptr<zmq::context_t> goby::core::Liaison::zmq_context_(new zmq::context_t(1));
const std::string goby::core::Liaison::LIAISON_INTERNAL_PUBLISH_SOCKET_NAME = "liaison_internal_publish_socket"; 
const std::string goby::core::Liaison::LIAISON_INTERNAL_SUBSCRIBE_SOCKET_NAME = "liaison_internal_subscribe_socket"; 

int main(int argc, char* argv[])
{
    return goby::run<goby::core::Liaison>(argc, argv);
}

goby::core::Liaison::Liaison()
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
                                 goby::core::create_wt_application);

        if (!wt_server_.start())
            glog << die << "Could not start Wt HTTP server." << std::endl;
        

    }
    catch (Wt::WServer::Exception& e)
    {
        glog << die << "Could not start Wt HTTP server. Exception: " << e.what() << std::endl;
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

void goby::core::Liaison::loop()
{
}

void goby::core::Liaison::inbox(MarshallingScheme marshalling_scheme,
                                const std::string& identifier,
                                const void* data,
                                int size,
                                int socket_id)
{
    glog << "Liaison: got message with identifier: " << identifier << std::endl;    
    zeromq_service_.send(marshalling_scheme, identifier, data, size, LIAISON_INTERNAL_PUBLISH_SOCKET);
}


goby::core::LiaisonWtThread::LiaisonWtThread(const Wt::WEnvironment& env)
    : Wt::WApplication(env),
      zeromq_service_(Liaison::zmq_context())
{    
    Wt::WString title_text("goby liaison: " + Liaison::cfg_.base().platform_name());
    setTitle(title_text);

    useStyleSheet(std::string("css/fonts.css?" + util::goby_file_timestamp()));
    useStyleSheet(std::string("css/liaison.css?" + util::goby_file_timestamp()));
//    setCssTheme("");
    

    contents_stack_ = new WStackedWidget();
    // Show scrollbars when needed ...
    contents_stack_->setOverflow(WContainerWidget::OverflowAuto);
    // ... and work around a bug in IE (see setOverflow() documentation)
    contents_stack_->setPositionScheme(Relative);
    contents_stack_->setStyleClass("contents");
    
    /*
     * Setup the menu
     */
    WMenu *menu = new WMenu(contents_stack_, Vertical, 0);
    menu->setRenderAsList(true);
    menu->setStyleClass("menu");
    menu->setInternalPathEnabled();
    menu->setInternalBasePath("/");
    
    add_to_menu(menu, "Home", new LiaisonHome());
    add_to_menu(menu, "Scope", new LiaisonScope(&zeromq_service_));
//    add_to_menu(menu, "scope2", new LiaisonScope("hello 2"));

    
    /*
     * Add it all inside a layout
     */
    WVBoxLayout *vertLayout1 = new WVBoxLayout(root());
    WHBoxLayout *horizLayout1 = new WHBoxLayout; 
    WVBoxLayout *vertLayout2 = new WVBoxLayout;
    WHBoxLayout *horizLayout2 = new WHBoxLayout;

    WText* header = new WText(title_text);
    header->setStyleClass("header");
    
    vertLayout1->addWidget(header, 0, AlignCenter | AlignTop);
    vertLayout1->addLayout(horizLayout1, 1);
    horizLayout1->addWidget(menu);
    horizLayout1->setResizable(0, true);
    horizLayout1->addLayout(vertLayout2, 1);
    vertLayout2->addWidget(contents_stack_);

    WImage* goby_lp_image = new WImage("images/goby-lp.png");
    WImage* goby_logo = new WImage("images/gobysoft_logo_dot_org_small.png");

    WAnchor* goby_lp_image_a = new WAnchor("https://launchpad.net/goby", goby_lp_image);
    WAnchor* goby_logo_a = new WAnchor("http://gobysoft.org/#/software/goby", goby_logo);
    goby_lp_image_a->setStyleClass("no_ul");
    goby_logo_a->setStyleClass("no_ul");
    goby_lp_image_a->setTarget(TargetNewWindow);
    goby_logo_a->setTarget(TargetNewWindow);
    
    vertLayout1->addLayout(horizLayout2, 0);
    horizLayout2->addWidget(goby_lp_image_a, 0, AlignLeft | AlignBottom);
    horizLayout2->addWidget(goby_logo_a, 0, AlignRight | AlignBottom);



    
//    zeromq_service_.connect_inbox_slot(&LiaisonWtThread::inbox, this);

    protobuf::ZeroMQServiceConfig ipc_sockets;
    protobuf::ZeroMQServiceConfig::Socket* internal_publish_socket = ipc_sockets.add_socket();
    internal_publish_socket->set_socket_type(protobuf::ZeroMQServiceConfig::Socket::SUBSCRIBE);
    internal_publish_socket->set_socket_id(Liaison::LIAISON_INTERNAL_PUBLISH_SOCKET);
    internal_publish_socket->set_transport(protobuf::ZeroMQServiceConfig::Socket::INPROC);
    internal_publish_socket->set_connect_or_bind(protobuf::ZeroMQServiceConfig::Socket::CONNECT);
    internal_publish_socket->set_socket_name(Liaison::LIAISON_INTERNAL_PUBLISH_SOCKET_NAME);

    zeromq_service_.merge_cfg(ipc_sockets);    
    zeromq_service_.subscribe_all(Liaison::LIAISON_INTERNAL_PUBLISH_SOCKET);

    Wt::WTimer *timer = new Wt::WTimer();
    timer->setInterval(100);
    timer->timeout().connect(this, &LiaisonWtThread::loop);
    timer->start();
}

void goby::core::LiaisonWtThread::add_to_menu(WMenu* menu, const WString& name,
                                                            LiaisonContainer* container)
{
    menu->addItem(name, container);
    container->set_name(name);
}

void goby::core::LiaisonWtThread::loop()
{
    using namespace goby::util::logger_lock;

    glog.is(debug2, lock) && glog << "LiaisonWtThread: polling" << std::endl << unlock;
    while(zeromq_service_.poll(0))
    { }    
}

void goby::core::LiaisonWtThread::inbox(MarshallingScheme marshalling_scheme,
                                                      const std::string& identifier,
                                                      const void* data,
                                                      int size,
                                                      int socket_id)
{
    using namespace goby::util::logger_lock;
    
    glog.is(debug1, lock) && glog << "LiaisonWtThread: got message with identifier: " << identifier << std::endl << unlock;
    
}

