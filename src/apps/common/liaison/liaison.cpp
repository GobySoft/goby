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
#include "liaison.h"

using goby::glog;
using namespace Wt;    
using namespace goby::util::logger_lock;
using namespace goby::util::logger;

goby::common::protobuf::LiaisonConfig goby::common::Liaison::cfg_;
boost::shared_ptr<zmq::context_t> goby::common::Liaison::zmq_context_(new zmq::context_t(1));
const std::string goby::common::Liaison::LIAISON_INTERNAL_PUBLISH_SOCKET_NAME = "liaison_internal_publish_socket"; 
const std::string goby::common::Liaison::LIAISON_INTERNAL_SUBSCRIBE_SOCKET_NAME = "liaison_internal_subscribe_socket"; 
std::vector<void *> goby::common::Liaison::dl_handles_;

int main(int argc, char* argv[])
{
    int return_value = goby::run<goby::common::Liaison>(argc, argv);
    goby::util::DynamicProtobufManager::protobuf_shutdown();
    for(std::vector<void *>::const_iterator it = goby::common::Liaison::dl_handles().begin(),
            n = goby::common::Liaison::dl_handles().end(); it != n; ++it)
        dlclose(*it);
    
    return return_value;
}

goby::common::Liaison::Liaison()
    : ZeroMQApplicationBase(&zeromq_service_, &cfg_),
      zeromq_service_(zmq_context_),
      pubsub_node_(&zeromq_service_, cfg_.base().pubsub_config()),
      source_database_(&disk_source_tree_)
{
    source_database_.RecordErrorsTo(&error_collector_);
    disk_source_tree_.MapPath("/", "/");
    goby::util::DynamicProtobufManager::add_database(&source_database_);


    // load all shared libraries
    for(int i = 0, n = cfg_.load_shared_library_size(); i < n; ++i)
    {
        glog.is(VERBOSE) &&
            glog << "Loading shared library: " << cfg_.load_shared_library(i) << std::endl;
        
        void* handle = dlopen(cfg_.load_shared_library(i).c_str(), RTLD_LAZY);
        if(!handle)
        {
            glog.is(DIE) &&
                glog << "Failed ... check path provided or add to /etc/ld.so.conf "
                     << "or LD_LIBRARY_PATH" << std::endl;
        }
        dl_handles_.push_back(handle);
    }
    
    
    // load all .proto files
    for(int i = 0, n = cfg_.load_proto_file_size(); i < n; ++i)
        load_proto_file(cfg_.load_proto_file(i));

    // load all .proto file directories
    for(int i = 0, n = cfg_.load_proto_dir_size(); i < n; ++i)
    {
        boost::filesystem::path current_dir(cfg_.load_proto_dir(i));

        for (boost::filesystem::directory_iterator iter(current_dir), end;
             iter != end;
             ++iter)
        {
#if BOOST_FILESYSTEM_VERSION == 3
            if(iter->path().extension().string() == ".proto")
#else
            if(iter->path().extension() == ".proto")
#endif            
              load_proto_file(iter->path().string());        
        }
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


    protobuf::ZeroMQServiceConfig::Socket* internal_subscribe_socket = ipc_sockets.add_socket();
    internal_subscribe_socket->set_socket_type(protobuf::ZeroMQServiceConfig::Socket::SUBSCRIBE);
    internal_subscribe_socket->set_socket_id(LIAISON_INTERNAL_SUBSCRIBE_SOCKET);
    internal_subscribe_socket->set_transport(protobuf::ZeroMQServiceConfig::Socket::INPROC);
    internal_subscribe_socket->set_connect_or_bind(protobuf::ZeroMQServiceConfig::Socket::BIND);
    internal_subscribe_socket->set_socket_name(LIAISON_INTERNAL_SUBSCRIBE_SOCKET_NAME);
    
    zeromq_service_.merge_cfg(ipc_sockets);
    zeromq_service_.subscribe_all(LIAISON_INTERNAL_SUBSCRIBE_SOCKET);
    
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

}

void goby::common::Liaison::load_proto_file(const std::string& path)
{
    glog.is(VERBOSE) &&
        glog << "Loading protobuf file: " << path << std::endl;

        
    if(!goby::util::DynamicProtobufManager::descriptor_pool().FindFileByName(path))
        glog.is(DIE) &&
            glog << "Failed to load file." << std::endl;
}



void goby::common::Liaison::loop()
{
    // static int i = 0;
    // i++;
    // if(i > (20 * cfg_.base().loop_freq()))
    // {
    //     wt_server_.stop();
    //     quit();
    // }

}

void goby::common::Liaison::inbox(MarshallingScheme marshalling_scheme,
                                const std::string& identifier,
                                const void* data,
                                int size,
                                int socket_id)
{
    glog.is(DEBUG2, lock) && glog << "Liaison: got message with identifier: " << identifier << " from socket: " << socket_id << std::endl << unlock;
    zeromq_service_.send(marshalling_scheme, identifier, data, size, LIAISON_INTERNAL_PUBLISH_SOCKET);
    
    if(socket_id == LIAISON_INTERNAL_SUBSCRIBE_SOCKET)
    {
        glog.is(DEBUG2, lock) && glog << "Sending to pubsub node: " << identifier << std::endl << unlock;
        pubsub_node_.publish(marshalling_scheme, identifier, data, size);
    }
}
