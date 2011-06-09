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

#include "goby/core/libcore/application.h"
#include "liaison_config.pb.h"
#include <Wt/WEnvironment>
#include <Wt/WApplication>
#include <Wt/WServer>

namespace goby
{
    namespace core
    {
        class Liaison : public Application
        {
          public:
            Liaison();
            ~Liaison() { }

            class LiaisonWtApplication : public Wt::WApplication
            {
              public:
                LiaisonWtApplication(const Wt::WEnvironment& env);
                
            };
            
            void loop();
          private:
            static protobuf::LiaisonConfig cfg_;
            Wt::WServer wt_server_;
        };

        Wt::WApplication* create_wt_application(const Wt::WEnvironment& env)
        {
            return new Liaison::LiaisonWtApplication(env);
        }


    }
}



#endif
