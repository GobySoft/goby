// Copyright 2009-2012 Toby Schneider (https://launchpad.net/~tes)
//                     Massachusetts Institute of Technology (2007-)
//                     Woods Hole Oceanographic Institution (2007-)
//                     Goby Developers Team (https://launchpad.net/~goby-dev)
// 
//
// This file is part of the Goby Underwater Autonomy Project Binaries
// ("The Goby Binaries").
//
// The Goby Binaries are free software: you can redistribute them and/or modify
// them under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// The Goby Binaries are distributed in the hope that they will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Goby.  If not, see <http://www.gnu.org/licenses/>.

#include <boost/filesystem.hpp>


#include <Wt/WText>
#include <Wt/WHBoxLayout>
#include <Wt/WVBoxLayout>
#include <Wt/WStackedWidget>
#include <Wt/WImage>
#include <Wt/WAnchor>

#include "goby/common/time.h"
#include "goby/util/dynamic_protobuf_manager.h"

#include "liaison_wt_thread.h"
#include "liaison.h"

using goby::glog;
using namespace Wt;    
using namespace goby::common::logger_lock;
using namespace goby::common::logger;

#if BOOST_FILESYSTEM_VERSION == 3
namespace bf = boost::filesystem3;
#else
namespace bf = boost::filesystem;
#endif


goby::common::protobuf::LiaisonConfig goby::common::Liaison::cfg_;
boost::shared_ptr<zmq::context_t> goby::common::Liaison::zmq_context_(new zmq::context_t(1));
const std::string goby::common::Liaison::LIAISON_INTERNAL_PUBLISH_SOCKET_NAME = "liaison_internal_publish_socket"; 
const std::string goby::common::Liaison::LIAISON_INTERNAL_SUBSCRIBE_SOCKET_NAME = "liaison_internal_subscribe_socket"; 

int main(int argc, char* argv[])
{
    int return_value = goby::run<goby::common::Liaison>(argc, argv);
    goby::util::DynamicProtobufManager::protobuf_shutdown();    
    return return_value;
}

goby::common::Liaison::Liaison()
    : ZeroMQApplicationBase(&zeromq_service_, &cfg_),
      zeromq_service_(zmq_context_),
      pubsub_node_(&zeromq_service_, cfg_.base().pubsub_config())
{


    // load all shared libraries
    for(int i = 0, n = cfg_.load_shared_library_size(); i < n; ++i)
    {
        glog.is(VERBOSE) &&
            glog << "Loading shared library: " << cfg_.load_shared_library(i) << std::endl;
        
        void* handle = goby::util::DynamicProtobufManager::load_from_shared_lib(
            cfg_.load_shared_library(i));
        
        if(!handle)
        {
            glog.is(DIE) && glog << "Failed ... check path provided or add to /etc/ld.so.conf "
                         << "or LD_LIBRARY_PATH" << std::endl;
        }
    }
    
    
    // load all .proto files
    goby::util::DynamicProtobufManager::enable_compilation();
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
        std::string doc_root;
        
        if(cfg_.has_docroot())
            doc_root = cfg_.docroot();
        else if(bf::exists(bf::path(GOBY_LIAISON_COMPILED_DOCROOT)))
            doc_root = GOBY_LIAISON_COMPILED_DOCROOT;            
        else if(bf::exists(bf::path(GOBY_LIAISON_INSTALLED_DOCROOT)))
            doc_root = GOBY_LIAISON_INSTALLED_DOCROOT;
        else
            throw(std::runtime_error("No valid docroot found for Goby Liaison. Set docroot to the valid path to what is normally /usr/share/goby/liaison"));
        
        // create a set of fake argc / argv for Wt::WServer
        std::vector<std::string> wt_argv_vec;  
        std::string str = cfg_.base().app_name() + " --docroot " + doc_root + " --http-port " + goby::util::as<std::string>(cfg_.http_port()) + " --http-address " + cfg_.http_address();
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
    boost::filesystem::path bpath = bf::complete(path);
    bpath.normalize();

    glog.is(VERBOSE) &&
        glog << "Loading protobuf file: " << bpath << std::endl;

    
    if(!goby::util::DynamicProtobufManager::user_descriptor_pool().FindFileByName(bpath.string()))
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
