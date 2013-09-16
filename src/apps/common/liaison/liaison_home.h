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

#ifndef LIAISONHOME20110609H
#define LIAISONHOME20110609H

#include <Wt/WText>
#include <Wt/WCssDecorationStyle>
#include <Wt/WBorder>
#include <Wt/WColor>
#include <Wt/WVBoxLayout>

#include "liaison.h"

namespace goby
{
    namespace common
    {
        class LiaisonHome : public LiaisonContainer
        {
          public:
            LiaisonHome(Wt::WContainerWidget* parent = 0);

          private:
            Wt::WVBoxLayout* main_layout_;

        };
    }
}

#endif
