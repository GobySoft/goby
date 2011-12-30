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

#ifndef LIAISON20110609H
#define LIAISON20110609H

#include "goby/pb/application.h"
#include "liaison_config.pb.h"
#include <Wt/WEnvironment>
#include <Wt/WApplication>
#include <Wt/WString>
#include <Wt/WMenu>
#include <Wt/WServer>
#include <Wt/WContainerWidget>

namespace goby
{
    namespace core
    {
        /* class LiaisonContainerWrapper : public Wt::WContainerWidget */
        /* { */
        /*   public: */
        /*     LiaisonContainerWrapper() */
        /*     { */
        /*         setStyleClass("wrapper"); */
        /*     } */
        /*     virtual ~LiaisonContainerWrapper() { }             */
        /* }; */

            
        class LiaisonContainer : public  Wt::WContainerWidget
        {
          public:
          LiaisonContainer()
              : name_(new Wt::WText())
            {
                setStyleClass("fill");
                /* addWidget(new Wt::WText("<hr/>")); */
                /* addWidget(name_); */
                /* addWidget(new Wt::WText("<hr/>")); */
            }

            void set_name(const Wt::WString& name)
            {
                name_->setText(name);
            }
            
            virtual ~LiaisonContainer() { }

          private:
            Wt::WText* name_;
        };        

        
        class Liaison : public ZeroMQApplicationBase
        {
          public:
            Liaison();
            ~Liaison() { }

            void inbox(MarshallingScheme marshalling_scheme,
                       const std::string& identifier,
                       const void* data,
                       int size,
                       int socket_id);

            enum 
            {
                LIAISON_INTERNAL_PUBLISH_SOCKET = 1,
                LIAISON_INTERNAL_SUBSCRIBE_SOCKET = 2
            };

            static const std::string LIAISON_INTERNAL_PUBLISH_SOCKET_NAME;
            static const std::string LIAISON_INTERNAL_SUBSCRIBE_SOCKET_NAME; 
            

            void loop();

            static boost::shared_ptr<zmq::context_t> zmq_context() { return zmq_context_; }

            friend class LiaisonWtThread;
            friend class LiaisonScope;
          private:
            static protobuf::LiaisonConfig cfg_;
            Wt::WServer wt_server_;
            static boost::shared_ptr<zmq::context_t> zmq_context_;
            ZeroMQService zeromq_service_;
            PubSubNodeWrapperBase pubsub_node_;

            // add a database client
        };

        class LiaisonWtThread : public Wt::WApplication
        {
          public:
            LiaisonWtThread(const Wt::WEnvironment& env);

            void inbox(MarshallingScheme marshalling_scheme,
                       const std::string& identifier,
                       const void* data,
                       int size,
                       int socket_id);
                
            void loop();
                
          private:
            void add_to_menu(Wt::WMenu* menu, const Wt::WString& name, LiaisonContainer* container);
          private:
            Wt::WStackedWidget* contents_stack_;
            Wt::WTimer* timer_;
            
            ZeroMQService zeromq_service_;
        };

        
        inline Wt::WApplication* create_wt_application(const Wt::WEnvironment& env)
        {
            return new LiaisonWtThread(env);
        }

    }
}



#endif
