// Copyright 2009-2013 Toby Schneider (https://launchpad.net/~tes)
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

#ifndef pTranslatorH
#define pTranslatorH

#include <google/protobuf/descriptor.h>

#include <boost/asio/deadline_timer.hpp>

#include "goby/util/dynamic_protobuf_manager.h"
#include "goby/moos/goby_moos_app.h"
#include "goby/moos/moos_translator.h"

#include "pTranslator_config.pb.h"

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
    goby::moos::MOOSTranslator translator_;
    
    
    boost::asio::io_service timer_io_service_;
    boost::asio::io_service::work work_;

    
    std::vector<boost::shared_ptr<boost::asio::deadline_timer> > timers_;
    
    static pTranslatorConfig cfg_;    
    static CpTranslator* inst_;    
};


#endif 
