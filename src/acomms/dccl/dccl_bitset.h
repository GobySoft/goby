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


#ifndef DCCLBITSET20120424H
#define DCCLBITSET20120424H

#include <deque>
#include <algorithm>
#include <limits>

#include "goby/common/logger.h"
#include "goby/common/exception.h"


namespace goby
{
    namespace acomms
    {
        // front is LSB
        // back is MSB
        class Bitset : public std::deque<bool>
        {
          public:
          explicit Bitset(Bitset* parent = 0)
              : parent_(parent)
            { }

            
            explicit Bitset(size_type num_bits, unsigned long value = 0, Bitset* parent = 0)
                : std::deque<bool>(num_bits, false),
                parent_(parent)
                {
                    for(int i = 0, n = std::min<size_type>(std::numeric_limits<unsigned long>::digits, size());
                        i < n; ++i)
                    {
                        if(value & (1 << i))
                            (*this)[i] = true;
                    }
                }

            
            ~Bitset() { } 

            // get this number of bits from the little end of the parent bitset
            // and add them to the big end of our bitset,
            // the parent will request from their parent if required
            void get_more_bits(size_type num_bits);

            
            Bitset& operator&=(const Bitset& rhs)
            {
                if(rhs.size() != size())
                    throw(Exception("Bitset operator&= requires this->size() == rhs.size()"));
                
                for (size_type i = 0; i != this->size(); ++i)
                    (*this)[i] = (*this)[i] & rhs[i];
                return *this;
            }

            Bitset& operator|=(const Bitset& rhs)
            {
                if(rhs.size() != size())
                    throw(Exception("Bitset operator|= requires this->size() == rhs.size()"));

                for (size_type i = 0; i != this->size(); ++i)
                    (*this)[i] = (*this)[i] | rhs[i];
                return *this;
            }
            
            Bitset& operator^=(const Bitset& rhs)
            {
                if(rhs.size() != size())
                    throw(Exception("Bitset operator^= requires this->size() == rhs.size()"));

                for (size_type i = 0; i != this->size(); ++i)
                    (*this)[i] = (*this)[i] ^ rhs[i];
                return *this;
            }
            
//            Bitset& operator-=(const Bitset& rhs);
            
            
            Bitset& operator<<=(size_type n)
            {
                for(size_type i = 0; i < n; ++i)
                {
                    push_front(false);
                    pop_back();
                }
                return *this;
            }
               
            Bitset& operator>>=(size_type n)
            {
                for(size_type i = 0; i < n; ++i)
                {
                    push_back(false);
                    pop_front();
                }
                return *this;
            }
            
            Bitset operator<<(size_type n) const
            {    
                Bitset copy(*this);
                copy <<= n;
                return copy;
            }
            
            Bitset operator>>(size_type n) const
            {
                Bitset copy(*this);
                copy >>= n;
                return copy;
            }
            
            Bitset& set(size_type n, bool val = true)
            {
                (*this)[n] = val;
                return *this;
            }
            
            Bitset& set()
            {
                for(iterator it = begin(), n = end(); it != n; ++it)
                    *it = true;
                return *this;
            }
            
            Bitset& reset(size_type n)
            { return set(n, false);}
            Bitset& reset()
            {
                for(iterator it = begin(), n = end(); it != n; ++it)
                    *it = false;
                return *this;
            }

            Bitset& flip(size_type n)
            { return set(n, !(*this)[n]); }
            
            Bitset& flip()
            {
                for(size_type i = 0, n = size(); i < n; ++i)
                    flip(i);
                return *this;
            }
            

            bool test(size_type n) const
            { return (*this)[n]; }
            
            /* bool any() const; */
            /* bool none() const; */
            /* Bitset operator~() const; */
            /* size_type count() const; */

            unsigned long to_ulong() const
            {
                if(size() > static_cast<size_type>(std::numeric_limits<unsigned long>::digits))
                    throw(Exception("Type `unsigned long` cannot represent current bitset (this->size() > std::numeric_limits<unsigned long>::digits)"));                

                unsigned long out = 0;
                for(int i = 0, n = size(); i < n; ++i)
                {
                    if((*this)[i])
                        out |= (1ul << i);
                }
                
                return out;
            }

            std::string to_string() const
            {
                std::string s(size(), 0);
                int i = 0;
                for(Bitset::const_reverse_iterator it = rbegin(),
                        n = rend(); it != n; ++it)
                {
                    s[i] = (*it) ? '1' : '0';
                    ++i;
                }
                return s;
            }

            // little-endian
            // LSB = string[0]
            // MSB = string[N]
            std::string to_byte_string()
            {
                // number of bytes needed is ceil(size() / 8)
                std::string s(this->size()/8 + (this->size()%8 ? 1 : 0), 0);
                
                for(size_type i = 0, n = this->size(); i < n; ++i)
                    s[i/8] |= static_cast<char>((*this)[i] << (i%8));
                
                return s;
            }

            // little-endian
            void from_byte_string(const std::string& s)
            {
                this->resize(s.size() * 8);
                int i = 0;
                for(std::string::const_iterator it = s.begin(), n = s.end();
                    it != n; ++it)
                {
                    for(size_type j = 0; j < 8; ++j)
                        (*this)[i*8+j] = (*it) & (1 << j);
                    ++i;
                }
            }

            // adds the bitset to the little end
            Bitset& prepend(const Bitset& bits)
            {
                for(const_reverse_iterator it = bits.rbegin(),
                        n = bits.rend(); it != n; ++it)
                    push_front(*it);

                return *this;
            }

            // adds the bitset to the big end
            Bitset& append(const Bitset& bits)
            {
                for(const_iterator it = bits.begin(),
                        n = bits.end(); it != n; ++it)
                    push_back(*it);                

                return *this;
            }
            
            
          private:            
            Bitset relinquish_bits(size_type num_bits, bool final_child);
            
          private:            
            Bitset* parent_;

            
        };
        
        inline bool operator==(const Bitset& a, const Bitset& b)
        {
            return (a.size() == b.size()) && std::equal(a.begin(), a.end(), b.begin());
        }
        
        inline bool operator<(const Bitset& a, const Bitset& b)
        {
            for(Bitset::size_type i = (std::max(a.size(), b.size()) - 1); i >= 0; --i)
            {
                bool a_bit = (i < a.size()) ? a[i] : 0;
                bool b_bit = (i < b.size()) ? b[i] : 0;
                
                if(a_bit > b_bit) return false;
                else if(a_bit < b_bit) return true;
            }
        }                
        
        inline Bitset operator&(const Bitset& b1, const Bitset& b2)
        {
            Bitset out(b1);
            out &= b2;
            return out;
        }
        
        inline Bitset operator|(const Bitset& b1, const Bitset& b2)
        {
            Bitset out(b1);
            out |= b2;
            return out;
        }

        inline Bitset operator^(const Bitset& b1, const Bitset& b2)
        {
            Bitset out(b1);
            out ^= b2;
            return out;
        }

//           inline Bitset operator-(const Bitset& b1, const Bitset& b2);
        
        inline std::ostream& operator<<(std::ostream& os, const Bitset& b)
        {
            return (os << b.to_string());
        }

        
    }
}

inline void goby::acomms::Bitset::get_more_bits(size_type num_bits)
{
    relinquish_bits(num_bits, true);
}


#endif
