// copyright 2011 t. schneider tes@mit.edu
// 
// this file is part of goby
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


#ifndef VERSION20110304H
#define VERSION20110304H

#include <string>

#define GOBY_VERSION_MAJOR @GOBY_VERSION_MAJOR@
#define GOBY_VERSION_MINOR @GOBY_VERSION_MINOR@
#define GOBY_VERSION_PATCH @GOBY_VERSION_PATCH@

namespace goby
{
    const std::string VERSION_STRING = "@GOBY_VERSION@";
    const std::string VERSION_DATE = "@GOBY_VERSION_DATE@";
}

#endif
