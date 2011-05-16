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

#include "goby/acomms/dccl.h"
#include "goby/protobuf/queue.pb.h"
#include "goby/protobuf/acomms_proto_helpers.h"

#include <map>
#include <deque>

#include "queue_exception.h"
#include "queue.h"


namespace goby
{
    namespace acomms
    {
        /// \class QueueManager queue.h goby/acomms/queue.h
        /// \brief provides an API to the goby-acomms Queuing Library.
        /// \ingroup acomms_api
        /// \sa queue.proto and modem_message.proto for definition of Google Protocol Buffers messages (namespace goby::acomms::protobuf).
        /// \todo turn into singleton a la DCCLCodec
        class QueueManager
        {
          public:
            static QueueManager* get()
            {
                // set these now so that the user has a chance of setting the logger
                if(!inst_)
                    inst_.reset(new QueueManager);

                return inst_.get();    
            }
        
            /// \name Initialization Methods
            ///
            /// These methods are intended to be called before doing any work with the class.
            //@{

            /// \brief Set (and overwrite completely if present) the current configuration. (protobuf::QueueManagerConfig defined in queue.proto)
            void set_cfg(const protobuf::QueueManagerConfig& cfg);

            /// \brief Set (and merge "repeat" fields) the current configuration. (protobuf::QueueManagerConfig defined in queue.proto)
            void merge_cfg(const protobuf::QueueManagerConfig& cfg);

            void add_queue(const google::protobuf::Message& msg);
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
            void push_message(const protobuf::ModemDataTransmission& new_message);
            void push_message(const google::protobuf::Message& new_message);
            //@}
            
            /// \name Modem Slots
            ///
            /// These methods are the interface to the QueueManager from the %modem driver.
            //@{

            /// \brief Finds data to send to the %modem.
            /// 
            /// Data from the highest priority %queue(s) will be combined to form a message equal or less than the size requested in ModemMessage message_in. If using one of the classes inheriting ModemDriverBase, this method should be bound and passed to ModemDriverBase::set_datarequest_cb.
            /// \param message_in The ModemMessage containing the details of the request (source, destination, size, etc.)
            /// \param message_out The packed ModemMessage ready for sending by the modem. This will be populated by this function.
            /// \return true if successful in finding data to send, false if no data is available
            void handle_modem_data_request(const protobuf::ModemDataRequest& request_msg,
                                           protobuf::ModemDataTransmission* data_msg);

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


            /// \name Control
            ///
            /// Call these methods when you want the QueueManager to perform time sensitive tasks (such as expiring old messages)
            //@{
            /// \brief Calculates which messages have expired and emits the goby::acomms::QueueManager::signal_expire as necessary.
            /// \todo (tes) turn into "poll" method
            void do_work();

            //@}
        
            /// \name Informational Methods
            ///
            //@{        

            /// \return human readable summary of all loaded %queues
            std::string summary() const;
            //const ManipulatorManager& manip_manager() const { return manip_manager_; }

            
            
            //@}
        
            /// \name Application Signals
            //@{
            /// \brief Signals when acknowledgment of proper message receipt has been received. This is only sent for queues with QueueConfig::ack() == true with an explicit destination (ModemMessageBase::dest() != 0)
            ///
            /// \param ack_msg a message containing details of the acknowledgment and the acknowledged transmission. (protobuf::ModemMsgAck is defined in modem_message.proto)        
            boost::signal<void (const protobuf::ModemDataAck& ack_msg,
                                const google::protobuf::Message& orig_msg)> signal_ack;

            /// \brief Signals when a DCCL message is received.
            ///
            /// \param msg the received transmission.
            boost::signal<void (const google::protobuf::Message& msg) > signal_receive;

            /// \brief Signals when a CCL message is received.
            ///
            /// \param msg the received transmission. (protobuf::ModemDataTransmission is defined in modem_message.proto)
            boost::signal<void (const protobuf::ModemDataTransmission& msg)> signal_receive_ccl;
            
            /// \brief Signals when a message is expires (exceeds its time-to-live or ttl) before being sent (if QueueConfig::ack() == false) or before being acknowledged (if QueueConfig::ack() == true).
            ///
            /// \param expire_msg the expired transmission. (protobuf::ModemDataExpire is defined in modem_message.proto)
            boost::signal<void (const google::protobuf::Message& orig_msg)> signal_expire;
            
            /// \brief Forwards the data request to the application layer. This advanced feature is used with the ON_DEMAND manipulator and allows for the application to provide data immediately before it is actually sent (for highly time sensitive data)
            ///
            /// \param request_msg the details of the requested data. (protobuf::ModemDataRequest is defined in modem_message.proto)
            /// \param data_msg pointer to store the supplied data. (protobuf::ModemDataTransmission is defined in modem_message.proto)
            boost::signal<void (const protobuf::ModemDataRequest& request_msg,
                                boost::shared_ptr<google::protobuf::Message> data_msg)> signal_data_on_demand;

            /// \brief Signals when any queue changes size (message is popped or pushed)
            ///
            /// \param size message containing the queue that changed size and its new size (protobuf::QueueSize is defined in queue.proto).
            boost::signal<void (protobuf::QueueSize size)> signal_queue_size_change;
            //@}

            /// \example acomms/chat/chat.cpp


            
          private:
            QueueManager();

            // so we can use shared_ptr to hold the singleton
            template<typename T>
                friend void boost::checked_delete(T*);
            /// destructor
            ~QueueManager()
            { }

            QueueManager(const QueueManager&);
            QueueManager& operator= (const QueueManager&);
            //@}

            

            /// \brief Add more Queues.
            ///
            /// \param QueueConfig& cfg: configuration object for the new %queue.
            void add_queue(const protobuf::QueueConfig& cfg);            
            void add_flex_groups();
            
            
            void qsize(Queue* q)
            {
                protobuf::QueueSize size;
                size.mutable_key()->CopyFrom(q->cfg().key());
                size.set_size(q->size());
                signal_queue_size_change(size);
            }
        
            // finds the %queue with the highest priority
            Queue* find_next_sender(const protobuf::ModemDataRequest& message, const protobuf::ModemDataTransmission& data_msg, bool first_user_frame);
        
            // clears the destination and ack values for the packet to reset for next $CADRQ
            void clear_packet(); 
            void process_cfg();


            void set_latest_dest(const boost::any& wire_value,
                                 const boost::any& extension_value)
            {
                goby::glog.is(debug2) &&
                    goby::glog << "setting dest to " << boost::any_cast<int32>(wire_value) << std::endl;
                
                latest_data_msg_.mutable_base()->set_dest(boost::any_cast<int32>(wire_value));
            }
            
            void set_latest_src(const boost::any& wire_value,
                                const boost::any& extension_value)
            {
                goby::glog.is(debug2) &&
                    goby::glog << "setting source to " << boost::any_cast<int32>(wire_value) << std::endl;
                
                latest_data_msg_.mutable_base()->set_src(boost::any_cast<int32>(wire_value));
            }
            
            void set_latest_time(const boost::any& wire_value,
                                 const boost::any& extension_value)
            {
                goby::glog.is(debug2) &&
                    goby::glog << "setting time to " << boost::any_cast<std::string>(wire_value) << std::endl;
                
                latest_data_msg_.mutable_base()->set_time(boost::any_cast<std::string>(wire_value));
            }

            
          private:
            static boost::shared_ptr<QueueManager> inst_;
            static protobuf::ModemDataTransmission latest_data_msg_;

            friend class Queue;
            static int modem_id_;
            std::map<goby::acomms::protobuf::QueueKey, Queue> queues_;
            
            // map frame number onto %queue pointer that contains
            // the data for this ack
            std::multimap<unsigned, Queue*> waiting_for_ack_;

            // the first *user* frame sets the tone (dest & ack) for the entire packet (all %modem frames)
            unsigned packet_ack_;
            int packet_dest_;
 
            protobuf::QueueManagerConfig cfg_;
            
//            ManipulatorManager manip_manager_;
        };

        /// outputs information about all available messages (same as std::string summary())
        std::ostream& operator<< (std::ostream& out, const QueueManager& d);

        
    }

}


#endif
