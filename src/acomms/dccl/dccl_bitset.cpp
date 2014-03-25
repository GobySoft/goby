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


#include "dccl_bitset.h"
#include "dccl.h"

using namespace goby::common::logger;

goby::acomms::Bitset goby::acomms::Bitset::relinquish_bits(size_type num_bits,
                                                           bool final_child)
{
//    glog.is(DEBUG2) && glog  << group(DCCLCodec::glog_decode_group()) << "Asked to relinquish " << num_bits << " from " << this << ": " << *this << std::endl;

    if(final_child || this->size() < num_bits)
    {
        size_type num_parent_bits = (final_child) ? num_bits : num_bits - this->size();
//        glog.is(DEBUG2) && glog  << group(DCCLCodec::glog_decode_group()) << "Need to get " << num_parent_bits << " from parent" << std::endl;

        if(parent_)
        {
            Bitset parent_bits = parent_->relinquish_bits(num_parent_bits, false);
            
//            glog.is(DEBUG2) && glog  << group(DCCLCodec::glog_decode_group()) << "parent_bits: " << parent_bits << std::endl;
            
            append(parent_bits);
        }
    }

    Bitset out;
    if(!final_child)
    {
        for(size_type i = 0; i < num_bits; ++i)
        {
            if(this->empty())
                throw(Exception("Cannot get_more_bits - no more bits to get!"));
            
            out.push_back(this->front());
            this->pop_front();
        }
    }
    return out;
}

