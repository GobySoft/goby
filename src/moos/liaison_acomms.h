// Copyright 2009-2014 Toby Schneider (https://launchpad.net/~tes)
//                     GobySoft, LLC (2013-)
//                     Massachusetts Institute of Technology (2007-2014)
//                     Goby Developers Team (https://launchpad.net/~goby-dev)
// 
//
// This file is part of the Goby Underwater Autonomy Project Libraries
// ("The Goby Libraries").
//
// The Goby Libraries are free software: you can redistribute them and/or modify
// them under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 2.1 of the License, or
// (at your option) any later version.
//
// The Goby Libraries are distributed in the hope that they will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with Goby.  If not, see <http://www.gnu.org/licenses/>.



#ifndef LIAISONACOMMS20140626H
#define LIAISONACOMMS20140626H

#include <Wt/WText>
#include <Wt/WBoxLayout>
#include <Wt/WVBoxLayout>
#include <Wt/WTimer>

#include "goby/common/liaison_container.h"
#include "goby/moos/protobuf/liaison_config.pb.h"

namespace goby
{
    namespace common
    {
        class LiaisonAcomms : public LiaisonContainer
        {
            
          public:
            LiaisonAcomms(const protobuf::LiaisonConfig& cfg, Wt::WContainerWidget* parent = 0);
            
          private:

            void loop();
            
          private:
            
            const protobuf::AcommsConfig& acomms_config_;

        };

    }
}

#endif
