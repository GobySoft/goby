// copyright 2009 t. schneider tes@mit.edu 
//
// this file is part of the Queue Library (libqueue),
// the goby-acomms message queue manager. goby-acomms is a collection of 
// libraries for acoustic underwater networking
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

#ifndef QueueKey20091211H
#define QueueKey20091211H

namespace queue
{    
    class QueueKey 
    {
      public:
      QueueKey(QueueType type = queue_notype, unsigned id = 0)
          : type_(type),
            id_(id)
            { }
        void set_type(QueueType type) { type_ = type; }
        void set_id(unsigned id)      { id_ = id; }        

        QueueType type() const { return type_; }
        unsigned id() const { return id_; }        
        
      private:
        QueueType type_;
        unsigned id_;
    };

    inline std::ostream& operator<< (std::ostream& out, const QueueKey& qk)
    { return out << "type: " << qk.type() << " | " << "id: " << qk.id(); }
    
    inline bool operator<(const QueueKey& a, const QueueKey& b)
    { return (a.id() == b.id()) ? a.type() < b.type() : a.id() < b.id(); }

    inline bool operator==(const QueueKey& a, const QueueKey& b)
    { return (a.id() == b.id()) && (a.type() == b.type()); }

}

#endif
