// copyright 2010 t. schneider tes@mit.edu
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

#ifndef CoreException20101005H
#define CoreException20101005H

#include <exception>
#include <stdexcept>

namespace goby
{
    
    /// \brief simple exception class for goby applications
    class Exception : public std::runtime_error
    {
      public:
      Exception(const std::string& s)
          : std::runtime_error(s)
        { }
        
    };

    ///  \brief  indicates a problem with the runtime command line
    /// or .cfg file configuration (or --help was given)
    class ConfigException : public Exception
    {
      public:
      ConfigException(const std::string& s)
          : Exception(s),
            error_(true)
        { }

        void set_error(bool b) { error_ = b; }
        bool error() { return error_; }
        
      private:
        bool error_;
    };
    
    
}


#endif

