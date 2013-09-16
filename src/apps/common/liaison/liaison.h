// Copyright 2009-2013 Toby Schneider (https://launchpad.net/~tes)
//                     Massachusetts Institute of Technology (2007-)
//                     Woods Hole Oceanographic Institution (2007-)
//                     Goby Developers Team (https://launchpad.net/~goby-dev)
// 
//
// This file is part of the Goby Underwater Autonomy Project Liaison Module
// ("Goby Liaison").
//
// Goby Liaison is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License Version 2
// as published by the Free Software Foundation.
//
// Goby Liaison is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Goby.  If not, see <http://www.gnu.org/licenses/>.


#ifndef LIAISON20110609H
#define LIAISON20110609H

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
#include "goby/common/liaison_container.h"
#include "goby/common/protobuf/liaison_config.pb.h"

namespace goby
{
    namespace common
    {   
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

            void loop();

            static boost::shared_ptr<zmq::context_t> zmq_context() { return zmq_context_; }

            static std::vector<void *> plugin_handles_;

          private:
            void load_proto_file(const std::string& path);
            
            friend class LiaisonWtThread;
          private:
            static protobuf::LiaisonConfig cfg_;
            Wt::WServer wt_server_;
            static boost::shared_ptr<zmq::context_t> zmq_context_;
            ZeroMQService zeromq_service_;
            PubSubNodeWrapperBase pubsub_node_;

            // add a database client
        };

    }
}



#endif
