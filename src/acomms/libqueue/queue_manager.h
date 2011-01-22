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


#ifndef QueueManager20091204_H
#define QueueManager20091204_H

#include <limits>
#include <set>
#include <boost/bind.hpp>
#include <boost/signal.hpp>

#include "goby/acomms/xml/xml_parser.h"
#include "goby/acomms/dccl.h"
#include "goby/acomms/protobuf/queue.pb.h"
#include "goby/acomms/protobuf/acomms_proto_helpers.h"

#include <map>
#include <deque>

#include "queue_exception.h"
#include "queue.h"


namespace goby
{
    namespace util
    {
        class FlexOstream;
    }
        
    
    namespace acomms
    {
        /// provides an API to the goby-acomms Queuing Library.
        class QueueManager
        {
          public:
            /// \name Constructors/Destructor
            //@{         
            /// \brief Default constructor.
            ///
            /// \param os std::ostream object or FlexOstream to capture all humanly readable runtime and debug information (optional).
            QueueManager(std::ostream* os = 0);

            /// \brief Instantiate with a single XML file.
            ///
            /// \param file path to an XML queuing configuration (i.e. contains a \verbatim <queuing/> \endverbatim section) file to parse for use by the QueueManager.
            /// \param schema path (absolute or relative to the XML file path) for the validating schema (message_schema.xsd) (optional).
            /// \param os std::ostream object or FlexOstream to capture all humanly readable runtime and debug information (optional).
            QueueManager(const std::string& file, const std::string schema = "", std::ostream* os = 0);
        
            /// \brief Instantiate with a set of XML files
            /// 
            /// \param files set of paths to XML queuing configuration (i.e. contains a \verbatim <queuing/> \endverbatim section) files to parse for use by the QueueManager.
            /// \param os std::ostream object or FlexOstream to capture all humanly readable runtime and debug information (optional).
            QueueManager(const std::set<std::string>& files, const std::string schema = "", std::ostream* os = 0);


            /// \brief Instantiate with a single QueueConfig object.
            ///
            /// \param cfg QueueConfig object to initialize a new %queue with. Use the QueueConfig largely for non-DCCL messages. Use the XML file constructors for XML configured DCCL messages.
            /// \param os std::ostream object or FlexOstream to capture all humanly readable runtime and debug information (optional).
            QueueManager(const protobuf::QueueConfig& cfg, std::ostream* os = 0);
        
            /// \brief Instantiate with a set of QueueConfig objects.
            //
            /// \param cfgs Set of QueueConfig objects to initialize new %queues with. Use the QueueConfig largely for non-DCCL messages. Use the XML file constructors for XML configured DCCL messages.
            /// \param os std::ostream object or FlexOstream to capture all humanly readable runtime and debug information (optional).
            QueueManager(const std::set<protobuf::QueueConfig>& cfgs, std::ostream* os = 0);

            /// Destructor.
            ~QueueManager() { }
        
            //@}
        
            /// \name Initialization Methods
            ///
            /// These methods are intended to be called before doing any work with the class. However,
            /// they may be called at any time as desired.
            //@{

            /// \brief Add more %queues by configuration XML files (typically contained in DCCL message XML files).
            ///
            /// \param xml_file path to the XML file to parse and add to this codec.
            /// \param xml_schema path to the message_schema.xsd file to validate XML with. if using a relative path this
            /// must be relative to the directory of the xml_file, not the present working directory. if not provided
            /// no validation is done.
            void add_xml_queue_file(const std::string& xml_file, const std::string xml_schema = "");

            /// \brief Add more Queues.
            ///
            /// \param QueueConfig& cfg: configuration object for the new %queue.
            void add_queue(const protobuf::QueueConfig& cfg);
        

            /// \brief Set the schema used for XML syntax checking.
            /// 
            /// Schema location is relative to the XML file location!
            /// if you have XML files in different places you must pass the
            /// proper relative path (or just use absolute paths)
            /// \param schema location of the message_schema.xsd file.
            void set_schema(const std::string schema) { xml_schema_ = schema; }

            /// \brief Set the %modem id for this vehicle.
            ///
            /// \param modem_id unique (within a network) number representing the %modem on this vehicle.
            void set_modem_id(int modem_id) { modem_id_ = modem_id; }

            /// \brief Set a %queue to call the data_on_demand callback every time data is request (basically forwards the %modem data_request).
            ///
            /// \param key QueueKey that references the %queue for which to enable the <tt>on demand</tt>  callback.
            void set_on_demand(protobuf::QueueKey key);
            /// \brief Set a %queue to call the data_on_demand callback every time data is request (basically forwards the %modem data_request).
            /// 
            /// \param id DCCL message id (\verbatim <id/> \endverbatim) that references the %queue for which to enable the <tt>on demand</tt> callback.
            void set_on_demand(unsigned id, protobuf::QueueType type = protobuf::QUEUE_DCCL);

            void add_flex_groups(util::FlexOstream& tout);
            
            
            //@}

            /// \name Application level Push/Receive Methods
            ///
            /// These methods are the primary higher level interface to the QueueManager. From here you can push messages
            /// and set the callbacks to use on received messages.
            //@{

            /// \brief Push a message using a QueueKey as a key.
            ///
            /// \param key QueueKey that references the %queue to push the message to.
            /// \param new_message ModemMessage to push.
            void push_message(protobuf::QueueKey key, const protobuf::ModemDataTransmission& new_message);        
            //@}
        
            /// \name Modem Driver level Push/Receive Methods
            ///
            /// These methods are the interface to the QueueManager from the %modem driver.
            //@{

            /// \brief Finds data to send to the %modem.
            /// 
            /// Data from the highest priority %queue(s) will be combined to form a message equal or less than the size requested in ModemMessage message_in. If using one of the classes inheriting ModemDriverBase, this method should be bound and passed to ModemDriverBase::set_datarequest_cb.
            /// \param message_in The ModemMessage containing the details of the request (source, destination, size, etc.)
            /// \param message_out The packed ModemMessage ready for sending by the modem. This will be populated by this function.
            /// \return true if successful in finding data to send, false if no data is available
            void handle_modem_data_request(const protobuf::ModemDataRequest& msg_request, protobuf::ModemDataTransmission* msg_data);

            /// \brief Receive incoming data from the %modem.
            ///
            /// If using one of the classes inheriting ModemDriverBase, this method should be bound and passed to ModemDriverBase::set_receive_cb.
            /// \param message The received ModemMessage.
            void handle_modem_receive(const protobuf::ModemDataTransmission& message);
        
            /// \brief Receive acknowledgements from the %modem.
            ///
            /// If using one of the classes inheriting ModemDriverBase, this method should be bound and passed to ModemDriverBase::set_ack_cb.
            /// \param message The ModemMessage corresponding to the acknowledgement (dest, src, frame#)
            void handle_modem_ack(const protobuf::ModemDataAck& message);
        
            //@}


            /// \name Active methods
            ///
            /// Call these methods when you want the QueueManager to perform time sensitive tasks
            //@{
            void do_work();

            //@}
        
            /// \name Informational Methods
            ///
            //@{        

            /// \return human readable summary of all loaded %queues
            std::string summary() const;

            
            
            //@}
        
            /// \example libqueue/examples/queue_simple/queue_simple.cpp
            /// simple.xml
            /// \verbinclude queue_simple/simple.xml
            /// queue_simple.cpp


            /// \example acomms/examples/chat/chat.cpp

            static int modem_id_;

            boost::signal<void (protobuf::QueueKey key,
                                const protobuf::ModemDataAck& ack_msg)> signal_ack;
            boost::signal<void (protobuf::QueueKey key,
                                const protobuf::ModemDataTransmission& msg)> signal_receive;
            boost::signal<void (protobuf::QueueKey key,
                                const protobuf::ModemDataTransmission& msg)> signal_receive_ccl;
            boost::signal<void (protobuf::QueueKey key,
                                const protobuf::ModemDataExpire& expire_msg)> signal_expire;
            boost::signal<void (protobuf::QueueKey key,
                                const protobuf::ModemDataRequest& request_msg,
                                protobuf::ModemDataTransmission* data_msg)> signal_data_on_demand;
            boost::signal<void (protobuf::QueueKey key, unsigned size)> signal_queue_size_change;
            
          private:
            void qsize(Queue* q)
            {
                protobuf::QueueKey qkey;
                qkey.set_type(q->cfg().type());
                qkey.set_id(q->cfg().id());
                signal_queue_size_change(qkey, q->size());
            }
        
            // finds the %queue with the highest priority
            Queue* find_next_sender(const protobuf::ModemDataRequest& message, const protobuf::ModemDataTransmission& data_msg, bool first_user_frame);
        
            // combine multiple "user" frames into a single "modem" frame
            bool stitch_recursive(const protobuf::ModemDataRequest& request_msg, protobuf::ModemDataTransmission* data_msg, Queue* winning_var);
            bool unstitch_recursive(std::string* data, protobuf::ModemDataTransmission* message);

            void replace_header(bool is_last_user_frame, protobuf::ModemDataTransmission* data_msg, const protobuf::ModemDataTransmission& next_data_msg, const std::string& new_data);
       
            // clears the destination and ack values for the packet to reset for next $CADRQ
            void clear_packet();
            
            // slave function to receive_incoming_modem_data that actually writes a piece of the message (called for each user-frame)
            bool publish_incoming_piece(const protobuf::ModemDataTransmission& message, const unsigned incoming_var_id);
            
        

          private:
            std::map<goby::acomms::protobuf::QueueKey, Queue> queues_;
            //boost::unordered_map<goby::acomms::protobuf::QueueKey, Queue> queues_;
            
            std::string xml_schema_;

            std::ostream* log_;

            // map frame number onto %queue pointer that contains
            // the data for this ack
            std::multimap<unsigned, Queue*> waiting_for_ack_;

            // the first *user* frame sets the tone (dest & ack) for the entire packet (all %modem frames)
            unsigned packet_ack_;
        };

        /// outputs information about all available messages (same as std::string summary())
        std::ostream& operator<< (std::ostream& out, const QueueManager& d);

        
    }

}


#endif
