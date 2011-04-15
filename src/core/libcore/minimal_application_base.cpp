// copyright 2010-2011 t. schneider tes@mit.edu
//
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

#include "minimal_application_base.h"


#include "goby/util/logger.h"

#include "goby/core/libcore/configuration_reader.h"

using goby::util::glogger;
using goby::util::as;


int goby::core::MinimalApplicationBase::argc_ = 0;
char** goby::core::MinimalApplicationBase::argv_ = 0;

goby::core::MinimalApplicationBase::MinimalApplicationBase(google::protobuf::Message* cfg /*= 0*/)
    : alive_(true)
{
    //
    // read the configuration
    //
    boost::program_options::options_description od("Allowed options");
    boost::program_options::variables_map var_map;
    try
    {
        std::string application_name;
        ConfigReader::read_cfg(argc_, argv_, cfg, &application_name, &od, &var_map);
        base_cfg_.set_app_name(application_name);

        // extract the AppBaseConfig assuming the user provided it in their configuration
        // .proto file
        if(cfg)
        {
            const google::protobuf::Descriptor* desc = cfg->GetDescriptor();
            for (int i = 0, n = desc->field_count(); i < n; ++i)
            {
                const google::protobuf::FieldDescriptor* field_desc = desc->field(i);
                if(field_desc->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE && field_desc->message_type() == ::AppBaseConfig::descriptor())
                {
                    base_cfg_.MergeFrom(cfg->GetReflection()->GetMessage(*cfg, field_desc));
                }
            }
        }        
        
        // incorporate some parts of the AppBaseConfig that are common
        // with gobyd (e.g. Verbosity)
        ConfigReader::merge_app_base_cfg(&base_cfg_, var_map);
    }
    catch(ConfigException& e)
    {
        // output all the available command line options
        std::cerr << od << "\n";
        if(e.error())
            std::cerr << "Problem parsing command-line configuration: \n"
                      << e.what() << "\n";        
        throw;
    }
    
    // set up the logger
    glogger().set_name(application_name());
    glogger().add_stream(static_cast<util::Logger::Verbosity>(base_cfg_.verbosity()),
                         &std::cout);

    if(!base_cfg_.IsInitialized())
        throw(ConfigException("Invalid base configuration"));


}

goby::core::MinimalApplicationBase::~MinimalApplicationBase()
{
    glogger() << debug <<"MinimalApplicationBase destructing..." << std::endl;    
}

void goby::core::MinimalApplicationBase::__run()
{
    // continue to run while we are alive (quit() has not been called)
    while(alive_)
    {
        try
        {
            iterate();
        }
        catch(std::exception& e)
        {
            glogger() << warn << e.what() << std::endl;
        }
    }
}

