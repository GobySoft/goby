// copyright 2009 t. schneider tes@mit.edu
// ocean engineering graudate student - mit / whoi joint program
// massachusetts institute of technology (mit)
// laboratory for autonomous marine sensing systems (lamss)
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

#ifndef QueueManager20091204_H
#define QueueManager20091204_H

#include <limits>

#include "queue_config.h"
#include "queue_key.h"
#include "queue.h"
#include "xml_parser.h"
#include "flex_cout.h"

/// all Queuing objects are in the `queue` namespace
namespace queue
{
    /// provides an API to the goby-acomms Queuing Library
    class QueueManager
    {
      public:
        /// \name Constructors/Destructor
        //@{         
        /// default
        QueueManager(FlexCout* tout = 0);

        /// instantiate with a single QueueConfig object
        QueueManager(const QueueConfig& cfg, FlexCout* tout = 0);

        /// instantiate with a set of QueueConfig objects
        QueueManager(const std::set<QueueConfig>& cfgs, FlexCout* tout = 0);

        /// instantiate with a single XML file
        QueueManager(const std::string& file, const std::string schema = "", FlexCout* tout = 0);
        
        /// instantiate with a set of XML files
        QueueManager(const std::set<std::string>& files, const std::string schema = "", FlexCout* tout = 0);

        /// destructor
        ~QueueManager() { if(own_tout_) delete tout_; }
        //@}
        
        /// \name Initialization Methods
        ///
        /// These methods are intended to be called before doing any work with the class. However,
        /// they may be called at any time as desired.
        //@{

        /// \brief add more queues
        ///
        /// \param QueueConfig& cfg: configuration object for the new queue
        void add_queue(const QueueConfig& cfg);
        
        /// \brief add more queues by configuration XML files (typically contained in DCCL message XML files
        ///
        /// \param xml_file path to the xml file to parse and add to this codec.
        /// \param xml_schema path to the message_schema.xsd file to validate XML with. if using a relative path this
        /// must be relative to the directory of the xml_file, not the present working directory. if not provided
        /// no validation is done.
        void add_xml_queue_file(const std::string& xml_file, const std::string xml_schema = "");

        /// \brief set the schema used for xml syntax checking
        /// 
        /// location is relative to the XML file location!
        /// if you have XML files in different places you must pass the
        /// proper relative path (or just use absolute paths)
        /// \param schema location of the message_schema.xsd file
        void set_schema(const std::string schema) { xml_schema_ = schema; }

        /// \brief set the modem id for this vehicle
        void set_modem_id(unsigned modem_id) { modem_id_ = modem_id; }

        /// \brief set a queue to call the data_on_demand callback every time data is request (basically forwards the modem data_request)
        void set_on_demand(QueueKey key);
        /// \brief set a queue to call the data_on_demand callback every time data is request (basically forwards the modem data_request)
        void set_on_demand(unsigned id, QueueType type = queue_dccl);
        
        //@}

        /// \name Push/Receive Methods
        ///
        /// These methods are used to do the real work of the QueueManager. From here you can push messages
        /// and set the callbacks to use on received messages
        //@{

        /// \brief push a message using the QueueKey as a key
        void push_message(QueueKey key, micromodem::Message& new_message);
        /// \brief push a message using the queue id and type together as a key
        void push_message(unsigned id, micromodem::Message& new_message, QueueType type = queue_dccl);

        
        //@}

        unsigned request_next_destination(unsigned size = std::numeric_limits<unsigned>::max());
        
        std::string summary() const;

        // finds data to send to the modem
        bool provide_outgoing_modem_data(micromodem::Message& message_in, micromodem::Message& message_out);
        // receive incoming data from the modem
        bool receive_incoming_modem_data(micromodem::Message& message);

        // pops messages acknowledged
        bool handle_modem_ack(micromodem::Message& message);

        typedef boost::function<void (QueueKey key, micromodem::Message & message)> MsgFunc1;
        typedef boost::function<void (QueueKey key, micromodem::Message & message1, micromodem::Message & message2)> MsgFunc2;
        typedef boost::function<void (QueueKey key, unsigned size)> QSizeFunc;

        void set_ack_cb(MsgFunc1 func)     { callback_ack = func; }
        
        void set_receive_cb(MsgFunc1 func)     { callback_receive = func; }
        void set_receive_ccl_cb(MsgFunc1 func) { callback_receive_ccl = func; }

        void set_data_on_demand_cb(MsgFunc2 func) { callback_ondemand = func; }
        
        void set_queue_size_change_cb(QSizeFunc func) { callback_qsize = func; }

      private:
        void qsize(Queue* q)
        {
            if(callback_qsize)
                callback_qsize(QueueKey(q->cfg().type(), q->cfg().id()), q->size());
        }
    
             
        FlexCout* init_tout(FlexCout* tout);


        // finds the queue with the highest priority
        Queue* find_next_sender(micromodem::Message& message);
        
        // combine multiple "user" frames into a single "modem" frame
        micromodem::Message stitch(const std::vector<micromodem::Message>& in_frames);
        
        // clears the destination and ack values for the packet to reset for next $CADRQ
        void clear_packet();

        // slave function to receive_incoming_modem_data that actually writes a piece of the message (called for each user-frame)
        bool publish_incoming_piece(micromodem::Message message, const unsigned incoming_var_id);        
        
      private:
        MsgFunc1 callback_ack;
                
        MsgFunc1 callback_receive;
        MsgFunc1 callback_receive_ccl;        
        MsgFunc2 callback_ondemand;
        
        QSizeFunc callback_qsize;
        
        unsigned modem_id_;
        
        std::map<QueueKey, Queue> queues_;

        std::string xml_schema_;
        time_t start_time_;

        FlexCout* tout_;
        bool own_tout_;

        // map frame number onto Queue pointer that contains
        // the data for this ack
        std::multimap<unsigned, Queue*> waiting_for_ack_;

        // the first *user* frame sets the tone (dest & ack) for the entire packet (all modem frames)
        unsigned packet_dest_;
        unsigned packet_ack_;
    
    };

    /// outputs information about all available messages (same as std::string summary())
    std::ostream& operator<< (std::ostream& out, const QueueManager& d);
}



#endif
