// Copyright 2009-2013 Toby Schneider (https://launchpad.net/~tes)
//                     Massachusetts Institute of Technology (2007-)
//                     Woods Hole Oceanographic Institution (2007-)
//                     Goby Developers Team (https://launchpad.net/~goby-dev)
// 
//
// This file is part of the Goby Underwater Autonomy Project Libraries
// ("The Goby Libraries").
//
// The Goby Libraries are free software: you can redistribute them and/or modify
// them under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// The Goby Libraries are distributed in the hope that they will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with Goby.  If not, see <http://www.gnu.org/licenses/>.

#include "dccl_field_codec_arithmetic.h"
#include "dccl_field_codec_manager.h"

// shared library load

extern "C"
{
    void goby_dccl_load(goby::acomms::DCCLCodec* dccl)
    {
        using namespace goby::acomms;
        using namespace goby;
        
        DCCLFieldCodecManager::add<DCCLArithmeticFieldCodec<int32> >("_arithmetic");
        DCCLFieldCodecManager::add<DCCLArithmeticFieldCodec<int64> >("_arithmetic");
        DCCLFieldCodecManager::add<DCCLArithmeticFieldCodec<uint32> >("_arithmetic");
        DCCLFieldCodecManager::add<DCCLArithmeticFieldCodec<uint64> >("_arithmetic");
        DCCLFieldCodecManager::add<DCCLArithmeticFieldCodec<double> >("_arithmetic");
        DCCLFieldCodecManager::add<DCCLArithmeticFieldCodec<float> >("_arithmetic");
        DCCLFieldCodecManager::add<DCCLArithmeticFieldCodec<bool> >("_arithmetic");
        DCCLFieldCodecManager::add<DCCLArithmeticFieldCodec<const google::protobuf::EnumValueDescriptor*> >("_arithmetic");
    }
}


using goby::glog;
using namespace goby::common::logger;

std::map<std::string, goby::acomms::Model> goby::acomms::ModelManager::arithmetic_models_;
const goby::acomms::Model::symbol_type goby::acomms::Model::OUT_OF_RANGE_SYMBOL;
const goby::acomms::Model::symbol_type goby::acomms::Model::EOF_SYMBOL;
const goby::acomms::Model::symbol_type goby::acomms::Model::MIN_SYMBOL;
const int goby::acomms::Model::CODE_VALUE_BITS;
const int goby::acomms::Model::FREQUENCY_BITS;
const goby::acomms::Model::freq_type goby::acomms::Model::MAX_FREQUENCY;
std::map<std::string, std::map<std::string, goby::acomms::Bitset> > goby::acomms::Model::last_bits_map;

goby::acomms::Model::symbol_type goby::acomms::Model::value_to_symbol(value_type value) const
{
    if(value < *user_model_.value_bound().begin() || value > *(user_model_.value_bound().end()-1))
        return Model::OUT_OF_RANGE_SYMBOL;
    

    google::protobuf::RepeatedField<double>::const_iterator upper_it =
        std::upper_bound(user_model_.value_bound().begin(),
                         user_model_.value_bound().end(),
                         value);

        
    google::protobuf::RepeatedField<double>::const_iterator lower_it = 
        (upper_it == user_model_.value_bound().begin()) ? upper_it : upper_it - 1;
        
    double lower_diff = std::abs((*lower_it)*(*lower_it) - value*value);
    double upper_diff = std::abs((*upper_it)*(*upper_it) - value*value);

//    std::cout << "value: " << value << std::endl;
//    std::cout << "lower_value: " << *lower_it << std::endl;
//    std::cout << "upper_value: " << *upper_it << std::endl;
    
    symbol_type symbol = ((lower_diff < upper_diff) ? lower_it : upper_it)
        - user_model_.value_bound().begin();

    
    return symbol;
                          
}
              
                  
goby::acomms::Model::value_type goby::acomms::Model::symbol_to_value(symbol_type symbol) const
{

    if(symbol == EOF_SYMBOL)
        throw(DCCLException("EOF symbol has no value."));
                          
    value_type value = (symbol == Model::OUT_OF_RANGE_SYMBOL) ?
        std::numeric_limits<value_type>::quiet_NaN() :
        user_model_.value_bound(symbol);

    return value;
}
              

std::pair<goby::acomms::Model::freq_type, goby::acomms::Model::freq_type> goby::acomms::Model::symbol_to_cumulative_freq(symbol_type symbol, ModelState state) const
{
    const boost::bimap<symbol_type, freq_type>& c_freqs = (state == ENCODER) ?
        encoder_cumulative_freqs_ :
        decoder_cumulative_freqs_;

    boost::bimap<symbol_type, freq_type>::left_map::const_iterator c_freq_it =
        c_freqs.left.find(symbol);
    std::pair<freq_type, freq_type> c_freq_range;
    c_freq_range.second = c_freq_it->second;
    if(c_freq_it == c_freqs.left.begin())
    {
        c_freq_range.first  = 0;
    }
    else
    {
        c_freq_it--;
        c_freq_range.first = c_freq_it->second;
    }
    return c_freq_range;
                          
}

std::pair<goby::acomms::Model::symbol_type, goby::acomms::Model::symbol_type> goby::acomms::Model::cumulative_freq_to_symbol(std::pair<freq_type, freq_type> c_freq_pair,  ModelState state) const
{

    const boost::bimap<symbol_type, freq_type>& c_freqs = (state == ENCODER) ?
        encoder_cumulative_freqs_ :
        decoder_cumulative_freqs_;
    
    std::pair<symbol_type, symbol_type> symbol_pair;
    
    // find the symbol for which the cumulative frequency is greater than
    // e.g.
    // symbol: 0   freq: 10   c_freq: 10 [0 ... 10)
    // symbol: 1   freq: 15   c_freq: 25 [10 ... 25)
    // symbol: 2   freq: 10   c_freq: 35 [25 ... 35)
    // searching for c_freq of 30 should return symbol 2     
    // searching for c_freq of 10 should return symbol 1
    symbol_pair.first = c_freqs.right.upper_bound(c_freq_pair.first)->second;
    
    
    if(symbol_pair.first == c_freqs.left.rbegin()->first)
        symbol_pair.second = symbol_pair.first; // last symbol can't be ambiguous on the low end
    else if(c_freqs.left.find(symbol_pair.first)->second > c_freq_pair.second)
        symbol_pair.second = symbol_pair.first; // unambiguously this symbol
    else
        symbol_pair.second = symbol_pair.first + 1;


    
    return symbol_pair;
}


void goby::acomms::Model::update_model(symbol_type symbol, ModelState state)
{
    if(!user_model_.is_adaptive())
        return;

    boost::bimap<symbol_type, freq_type>& c_freqs = (state == ENCODER) ?
        encoder_cumulative_freqs_ :
        decoder_cumulative_freqs_;

    if(glog.is(DEBUG3))
    {
        glog << "Model was: " << std::endl;
        for(symbol_type i = MIN_SYMBOL, n = max_symbol(); i <= n; ++i)
        {
            boost::bimap<symbol_type, freq_type>::left_iterator it =
                c_freqs.left.find(i);
            if(it != c_freqs.left.end())
                glog << "Symbol: " << it->first << ", c_freq: " << it->second << std::endl;
        }
    }
    
                
    for(symbol_type i = max_symbol(), n = symbol; i >= n; --i)
    {
        boost::bimap<symbol_type, freq_type>::left_iterator it =
            c_freqs.left.find(i);
        if(it != c_freqs.left.end())
            c_freqs.left.replace_data(it, it->second + 1);
    }

    if(glog.is(DEBUG3))
    {
        glog << "Model is now: " << std::endl;
        for(symbol_type i = MIN_SYMBOL, n = max_symbol(); i <= n; ++i)
        {
            boost::bimap<symbol_type, freq_type>::left_iterator it =
                c_freqs.left.find(i);
            if(it != c_freqs.left.end())
                glog << "Symbol: " << it->first << ", c_freq: " << it->second << std::endl;
        }
    }
    
    glog.is(DEBUG3) && glog << "total freq: " << total_freq(state) << std::endl;
                
}
