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

#include "liaison_home.h"

using namespace Wt;


goby::common::LiaisonHome::LiaisonHome()
{

    addWidget(new WText("Welcome to Goby Liaison: an extensible tool for commanding and comprehending this Goby platform."));
    addWidget(new WBreak());
    addWidget(new WText("<i>liaison (n): one that establishes and maintains communication for mutual understanding and cooperation</i>"));
}

