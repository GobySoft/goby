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
// but WITHOUT ANY WARRANTY; without even the implied warranty 
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this software.  If not, see <http://www.gnu.org/licenses/>.

#ifndef QueueKey20091211H
#define QueueKey20091211H
namespace goby
{
    namespace acomms
    {    
        /// forms a unique key for a given message %queue
        class QueueKey 
        {
          public:
            QueueKey(QueueType type = queue_notype, unsigned id = 0)
                : type_(type),
                id_(id)
                { }

            /// set the key type
            void set_type(QueueType type) { type_ = type; }
            /// set the key id (DCCL id for type = queue_dccl, CCL id for type = queue_ccl)
            void set_id(unsigned id)      { id_ = id; }        

            /// \return the key type
            QueueType type() const { return type_; }
            /// \return the key id
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
}
#endif
