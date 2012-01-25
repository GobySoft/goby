// t. schneider tes@mit.edu 06.05.08
// ocean engineering graudate student - mit / whoi joint program
// massachusetts institute of technology (mit)
// laboratory for autonomous marine sensing systems (lamss)
// 
// this is pTranslator.h, part of pTranslator
//
// see the readme file within this directory for information
// pertaining to usage and purpose of this script.
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

#ifndef pTranslatorH
#define pTranslatorH

#include <google/protobuf/compiler/importer.h>
#include <google/protobuf/descriptor.h>

#include <boost/asio/deadline_timer.hpp>

#include "goby/util/dynamic_protobuf_manager.h"
#include "goby/moos/goby_moos_app.h"
#include "goby/moos/moos_translator.h"

#include "pTranslator_config.pb.h"

extern std::vector<void *> dl_handles;

class CpTranslator : public GobyMOOSApp
{
  public:
    static CpTranslator* get_instance();
    static void delete_instance();
    
  private:
    CpTranslator();
    ~CpTranslator();
    
    void loop();     // from GobyMOOSApp

    void create_on_publish(const CMOOSMsg& trigger_msg, const goby::moos::protobuf::TranslatorEntry& entry);
    void create_on_multiplex_publish(const CMOOSMsg& moos_msg);
    
    
    void create_on_timer(const boost::system::error_code& error,
                         const goby::moos::protobuf::TranslatorEntry& entry,
                         boost::asio::deadline_timer* timer);
    
    void do_translation(const goby::moos::protobuf::TranslatorEntry& entry);
    void do_publish(boost::shared_ptr<google::protobuf::Message> created_message);

    
  private:
    google::protobuf::compiler::DiskSourceTree disk_source_tree_;
    google::protobuf::compiler::SourceTreeDescriptorDatabase source_database_;

    class TranslatorErrorCollector: public google::protobuf::compiler::MultiFileErrorCollector
    {
        void AddError(const std::string & filename, int line, int column, const std::string & message)
        {
            goby::glog.is(goby::util::logger::DIE) &&
                goby::glog << "File: " << filename
                           << " has error (line: " << line << ", column: " << column << "): "
                           << message << std::endl;
        }       
    };
                
    TranslatorErrorCollector error_collector_;

    goby::moos::MOOSTranslator translator_;
    
    
    boost::asio::io_service timer_io_service_;
    boost::asio::io_service::work work_;

    
    std::vector<boost::shared_ptr<boost::asio::deadline_timer> > timers_;
    
    static pTranslatorConfig cfg_;    
    static CpTranslator* inst_;    
};


#endif 
