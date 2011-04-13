// copyright 2010-2011 t. schneider tes@mit.edu
// 
// the file is the goby daemon, part of the core goby autonomy system
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

#include "goby/protobuf/app_base_config.pb.h"

#include "minimal_application_base.h"
#include "exception.h"

namespace goby
{
    /// \brief Run a Goby application derived from ProtobufApplicationBase.
    /// blocks caller until ProtobufApplicationBase::__run() returns
    /// \param argc same as int main(int argc, char* argv)
    /// \param argv same as int main(int argc, char* argv)
    /// \return same as int main(int argc, char* argv)
    template<typename App>
        int run(int argc, char* argv[]);

    namespace core
    {
        
        class ProtobufApplicationBase : public MinimalApplicationBase
        {
          protected:
            ProtobufApplicationBase(google::protobuf::Message* cfg = 0);
            virtual ~ProtobufApplicationBase();

            /// \brief Requests a clean (return 0) exit.
            void quit() { alive_ = false; }

            virtual void inbox(const std::string& protobuf_type_name,
                               const void* data,
                               int size) = 0;
            virtual void iterate() = 0;

            /// name of this application (from AppBaseConfig::app_name). E.g. "garmin_gps_g"
            std::string application_name()
            { return base_cfg_.app_name(); }
            /// name of this platform (from AppBaseConfig::platform_name). E.g. "AUV-23" or "unicorn"
            std::string platform_name()
            { return base_cfg_.platform_name(); }
            
            template<typename App>
                friend int ::goby::run(int argc, char* argv[]);

            const AppBaseConfig& base_cfg()
            { return base_cfg_; }
            
          private:

            void inbox(MarshallingScheme marshalling_scheme,
                       const std::string& identifier,
                       const void* data,
                       int size);

            // main loop that exits on disconnect. called by goby::run()
            void __run();
            
            void __set_application_name(const std::string& s)
            { base_cfg_.set_app_name(s); }
            void __set_platform_name(const std::string& s)
            { base_cfg_.set_platform_name(s); }
            

          private:
            // copies of the "real" argc, argv that are used
            // to give ApplicationBase access without requiring the subclasses of
            // ApplicationBase to pass them through their constructors
            static int argc_;
            static char** argv_;

            // configuration relevant to all applications (loop frequency, for example)
            // defined in #include "goby/core/proto/app_base_config.pb.h"
            AppBaseConfig base_cfg_;

            bool alive_;            
        };
    }    
}

template<typename App>
int goby::run(int argc, char* argv[])
{
    // avoid making the user pass these through their Ctor...
    App::argc_ = argc;
    App::argv_ = argv;
    
    try
    {
        App app;
        app.__run();
    }
    catch(goby::ConfigException& e)
    {
        // no further warning as the ApplicationBase Ctor handles this
        return 1;
    }
    catch(std::exception& e)
    {
        // some other exception
        std::cerr << "uncaught exception: " << e.what() << std::endl;
        return 2;
    }

    return 0;
}

