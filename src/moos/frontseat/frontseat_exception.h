// Copyright 2009-2013 Toby Schneider (https://launchpad.net/~tes)
//                     Massachusetts Institute of Technology (2007-)
//                     Woods Hole Oceanographic Institution (2007-)
//                     Goby Developers Team (https://launchpad.net/~goby-dev)
// 
//
// This file is part of the Goby Underwater Autonomy Project MOOS Interface Library
// ("The Goby MOOS Library").
//
// The Goby MOOS Library is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// The Goby MOOS Library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Goby.  If not, see <http://www.gnu.org/licenses/>.

#ifndef FrontSeatException20130221H
#define FrontSeatException20130221H

class FrontSeatException : std::runtime_error
{
  public:
    FrontSeatException()
        : std::runtime_error("Unknown FrontSeatException"),
        helm_err_(goby::moos::protobuf::ERROR_HELM_NONE),
        is_helm_error_(false),
        fs_err_(goby::moos::protobuf::ERROR_FRONTSEAT_NONE),
        is_fs_error_(false)
        { }
    
    FrontSeatException(goby::moos::protobuf::HelmError err)
        : std::runtime_error(goby::moos::protobuf::HelmError_Name(err)),
        helm_err_(err),
        is_helm_error_(true),
        fs_err_(goby::moos::protobuf::ERROR_FRONTSEAT_NONE),
        is_fs_error_(false)
    { }
    FrontSeatException(goby::moos::protobuf::FrontSeatError err)
        : std::runtime_error(goby::moos::protobuf::FrontSeatError_Name(err)),
        helm_err_(goby::moos::protobuf::ERROR_HELM_NONE),
        is_helm_error_(false),
        fs_err_(err),
        is_fs_error_(true)

    { }

    goby::moos::protobuf::HelmError helm_err() const { return helm_err_; }
    bool is_helm_error() const { return is_helm_error_; }
    
    goby::moos::protobuf::FrontSeatError fs_err() const { return fs_err_; }
    bool is_fs_error() const { return is_fs_error_; }
    
  private:
    goby::moos::protobuf::HelmError helm_err_;
    bool is_helm_error_;
    
    goby::moos::protobuf::FrontSeatError fs_err_;
    bool is_fs_error_;
};

inline std::ostream& operator<< (std::ostream& os, const FrontSeatException& e)
{
    if(e.is_helm_error())
        os << "Error in the Helm: " << goby::moos::protobuf::HelmError_Name(e.helm_err());
    else if(e.is_fs_error())
        os <<  "Error in the Frontseat: " << goby::moos::protobuf::FrontSeatError_Name(e.fs_err());
    else
        os << "Unknown error.";
    return os;
};

#endif
