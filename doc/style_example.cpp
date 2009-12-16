// implementation file layout:

// copyright {year} {author name} {author email}
//                  {secondary author} {secondary author email}
// 
// this file is part of {binary or library}, {description}
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

// std
#include <set>

// third party
#include <boost/bind.hpp>

// local to goby
#include "util/flexcout.h"

// local to folder (part of same application / library)
#include "style_example.h"

// global using declaratives: use sparingly, if at all
// do not use `using namespace std`
using namespace nspace;

bool nspace::Class::push_message(micromodem::Message& new_message)
{
    return true;
}
