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


namespace goby
{
    namespace common
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
          LiaisonContainer(Wt::WContainerWidget* parent)
              : Wt::WContainerWidget(parent)
            {
                setStyleClass("fill");
                /* addWidget(new Wt::WText("<hr/>")); */
                /* addWidget(name_); */
                /* addWidget(new Wt::WText("<hr/>")); */
            }

            void set_name(const Wt::WString& name)
            {
                name_.setText(name);
            }
            
            virtual ~LiaisonContainer() { }

          private:
            Wt::WText name_;
        };        


        
        class Liaison : public ZeroMQApplicationBase
        {
          public:
            Liaison();
            ~Liaison() { }

            void inbox(goby::common::MarshallingScheme marshalling_scheme,
                       const std::string& identifier,
                       const void* data,
                       int size,
                       int socket_id);

            static const std::string LIAISON_INTERNAL_PUBLISH_SOCKET_NAME;
            static const std::string LIAISON_INTERNAL_SUBSCRIBE_SOCKET_NAME;

            void loop();

            static boost::shared_ptr<zmq::context_t> zmq_context() { return zmq_context_; }
            static const std::vector<void *>& dl_handles() { return dl_handles_; }

            
            enum 
            {
                LIAISON_INTERNAL_PUBLISH_SOCKET = 1,
                LIAISON_INTERNAL_SUBSCRIBE_SOCKET = 2,
                LIAISON_INTERNAL_COMMANDER_SUBSCRIBE_SOCKET = 3,
                LIAISON_INTERNAL_COMMANDER_PUBLISH_SOCKET = 4,
                LIAISON_INTERNAL_SCOPE_SUBSCRIBE_SOCKET = 5,
                LIAISON_INTERNAL_SCOPE_PUBLISH_SOCKET = 6,  
            };

          private:
            void load_proto_file(const std::string& path);
            
            friend class LiaisonWtThread;
            friend class LiaisonScope;
            friend class LiaisonCommander;
          private:
            static protobuf::LiaisonConfig cfg_;
            Wt::WServer wt_server_;
            static boost::shared_ptr<zmq::context_t> zmq_context_;
            ZeroMQService zeromq_service_;
            PubSubNodeWrapperBase pubsub_node_;

            google::protobuf::compiler::DiskSourceTree disk_source_tree_;
            google::protobuf::compiler::SourceTreeDescriptorDatabase source_database_;
            
            class LiaisonErrorCollector: public google::protobuf::compiler::MultiFileErrorCollector
            {
                void AddError(const std::string & filename, int line, int column, const std::string & message)
                {
                    goby::glog.is(goby::util::logger::DIE, goby::util::logger_lock::lock) &&
                        goby::glog << "File: " << filename
                                   << " has error (line: " << line << ", column: " << column << "): "
                                   << message << std::endl << unlock;
                }       
            };
                
            LiaisonErrorCollector error_collector_;
            static std::vector<void *> dl_handles_;
            
            // add a database client
        };

    }
}



#endif
