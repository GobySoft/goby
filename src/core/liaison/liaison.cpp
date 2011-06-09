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

#include "liaison.h"

#include <Wt/WText>
#include <Wt/WContainerWidget>

using goby::glog;

goby::core::protobuf::LiaisonConfig goby::core::Liaison::cfg_;

int main(int argc, char* argv[])
{
    return goby::run<goby::core::Liaison>(argc, argv);
}

goby::core::Liaison::Liaison()
    : Application(&cfg_)
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
}

void goby::core::Liaison::loop()
{
}


goby::core::Liaison::LiaisonWtApplication::LiaisonWtApplication(const Wt::WEnvironment& env)
    : Wt::WApplication(env)
{
    setTitle("test");
    root()->addWidget(new Wt::WText("Hello World"));
}
