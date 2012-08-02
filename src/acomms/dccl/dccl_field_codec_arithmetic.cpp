// Copyright 2009-2012 Toby Schneider (https://launchpad.net/~tes)
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

std::map<std::string, goby::acomms::Model> goby::acomms::ModelManager::arithmetic_models_;
const goby::acomms::Model::symbol_type goby::acomms::Model::OUT_OF_RANGE_SYMBOL;
const goby::acomms::Model::symbol_type goby::acomms::Model::EOF_SYMBOL;
const goby::acomms::Model::symbol_type goby::acomms::Model::MIN_SYMBOL;
const int goby::acomms::Model::CODE_VALUE_BITS;
const int goby::acomms::Model::FREQUENCY_BITS;
const goby::acomms::Model::freq_type goby::acomms::Model::MAX_FREQUENCY;

goby::acomms::Model::symbol_type goby::acomms::Model::value_to_symbol(value_type value) const
{
    
    symbol_type symbol =
        std::upper_bound(user_model_.value_bound().begin(),
                         user_model_.value_bound().end(),
                         value) - 1 - user_model_.value_bound().begin();

    return (symbol < 0 || symbol >= user_model_.frequency_size()) ?
        Model::OUT_OF_RANGE_SYMBOL :
        symbol;
                          
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
              

std::pair<goby::acomms::Model::freq_type, goby::acomms::Model::freq_type> goby::acomms::Model::symbol_to_cumulative_freq(symbol_type symbol) const
{
    boost::bimap<symbol_type, freq_type>::left_map::const_iterator c_freq_it =
        cumulative_freqs_.left.find(symbol);
    std::pair<freq_type, freq_type> c_freq_range;
    c_freq_range.second = c_freq_it->second;
    if(c_freq_it == cumulative_freqs_.left.begin())
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

std::pair<goby::acomms::Model::symbol_type, goby::acomms::Model::symbol_type> goby::acomms::Model::cumulative_freq_to_symbol(std::pair<freq_type, freq_type> c_freq_pair) const
{
    std::pair<symbol_type, symbol_type> symbol_pair;
    
    // find the symbol for which the cumulative frequency is greater than
    // e.g.
    // symbol: 0   freq: 10   c_freq: 10 [0 ... 9]
    // symbol: 1   freq: 15   c_freq: 25 [10 ... 24]
    // symbol: 2   freq: 10   c_freq: 35 [25 ... 34]
    // searching for c_freq of 30 should return symbol 2     
    // searching for c_freq of 10 should return symbol 1
    symbol_pair.first = cumulative_freqs_.right.upper_bound(c_freq_pair.first)->second;
    
    
    if(symbol_pair.first == cumulative_freqs_.left.rbegin()->first)
        symbol_pair.second = symbol_pair.first; // last symbol can't be ambiguous on the low end
    else if(cumulative_freqs_.left.find(symbol_pair.first)->second > c_freq_pair.second)
        symbol_pair.second = symbol_pair.first; // unambiguously this symbol
    else
        symbol_pair.second = symbol_pair.first + 1;


    
    return symbol_pair;
}
