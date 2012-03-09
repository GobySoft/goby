
#ifndef LIAISONWTTHREAD20110609H
#define LIAISONWTTHREAD20110609H

#include <google/protobuf/compiler/importer.h>
#include <google/protobuf/descriptor.h>

#include <Wt/WEnvironment>
#include <Wt/WApplication>
#include <Wt/WString>
#include <Wt/WMenu>
#include <Wt/WServer>
#include <Wt/WContainerWidget>
#include <Wt/WTimer>

#include "goby/common/zeromq_application_base.h"
#include "goby/common/pubsub_node_wrapper.h"

#include "liaison_config.pb.h"
#include "liaison.h"

namespace goby
{
    namespace common
    {
        class LiaisonWtThread : public Wt::WApplication
        {
          public:
            LiaisonWtThread(const Wt::WEnvironment& env);

            void inbox(MarshallingScheme marshalling_scheme,
                       const std::string& identifier,
                       const void* data,
                       int size,
                       int socket_id);
                
                
          private:
            void add_to_menu(Wt::WMenu* menu, const Wt::WString& name, LiaisonContainer* container);
            void handle_menu_selection(Wt::WMenuItem * item);
            
          private:
            ZeroMQService scope_service_;
            ZeroMQService commander_service_;
            
            
            Wt::WStackedWidget* contents_stack_;
            Wt::WTimer scope_timer_;
            Wt::WTimer commander_timer_;

            
            enum TimerState { ACTIVE = 1, STOPPED = 2, UNKNOWN = 0 };
            TimerState last_scope_timer_state_;
            
        };

        
        inline Wt::WApplication* create_wt_application(const Wt::WEnvironment& env)
        {
            return new LiaisonWtThread(env);
        }

    }
}



#endif
