// Copyright 2009-2018 Toby Schneider (http://gobysoft.org/index.wt/people/toby)
//                     GobySoft, LLC (2013-)
//                     Massachusetts Institute of Technology (2007-2014)
//                     Community contributors (see AUTHORS file)
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

#ifndef MAC20091019H
#define MAC20091019H

#include <boost/asio.hpp>
#include <boost/asio/time_traits.hpp>
#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <boost/function.hpp>
#include <boost/lexical_cast.hpp>

#include "goby/acomms/modem_driver.h"
#include "goby/acomms/protobuf/amac.pb.h"
#include "goby/acomms/protobuf/amac_config.pb.h"
#include "goby/acomms/protobuf/modem_message.pb.h"
#include "goby/common/io_service.h"
#include "goby/common/time.h"
#include "goby/util/as.h"

namespace goby
{
namespace acomms
{
/// \name Acoustic MAC Library callback function type definitions
//@{

/// \class MACManager mac_manager.h goby/acomms/amac.h
/// \ingroup acomms_api
/// \brief provides an API to the goby-acomms MAC library. MACManager is essentially a std::list<protobuf::ModemTransmission> plus a timer.
/// \sa acomms_amac.proto and acomms_modem_message.proto for definition of Google Protocol Buffers messages (namespace goby::acomms::protobuf).
class MACManager : public std::list<protobuf::ModemTransmission>
{
  public:
    /// \name Constructors/Destructor
    //@{
    /// \brief Default constructor.
    MACManager();
    ~MACManager();
    //@}

    /// \name Control
    //@{

    /// \brief Starts the MAC with given configuration
    ///
    /// \param cfg Initial configuration values (protobuf::MACConfig defined in acomms_amac.proto)
    void startup(const protobuf::MACConfig& cfg);

    /// \brief Restarts the MAC with original configuration
    void restart();

    /// \brief Shutdown the MAC
    void shutdown();

    /// \brief Allows the MAC timer to do its work. Does not block. If you prefer more control you can directly control the underlying boost::asio::io_service (get_io_service()) instead of using this function. This function is equivalent to get_io_service().poll();
    void do_work() { io_.poll(); }

    /// \brief You must call this after any change to the underlying list that would invalidate iterators or change the size (insert, push_back, erase, etc.).
    void update();

    bool running() { return started_up_; }

    //@}

    /// \name Modem Signals
    //@{
    /// \brief Signals when it is time for this platform to begin transmission of an acoustic message at the start of its TDMA slot. Typically connected to ModemDriverBase::handle_initiate_transmission() using bind().
    ///
    /// \param m a message containing details of the transmission to be initated.  (protobuf::ModemMsgBase defined in acomms_modem_message.proto)
    boost::signals2::signal<void(const protobuf::ModemTransmission& m)>
        signal_initiate_transmission;

    /// \brief Signals the start of all transmissions (even when we don't transmit)
    ///
    /// \param m a message containing details of the transmission to be initated.  (protobuf::ModemMsgBase defined in acomms_modem_message.proto)

    boost::signals2::signal<void(const protobuf::ModemTransmission& m)> signal_slot_start;

    /// \example acomms/amac/amac_simple/amac_simple.cpp
    /// \example acomms/chat/chat.cpp

    unsigned cycle_count() { return std::list<protobuf::ModemTransmission>::size(); }
    double cycle_duration();

    boost::asio::io_service& get_io_service() { return io_; }

    const std::string& glog_mac_group() const { return glog_mac_group_; }

  private:
    void begin_slot(const boost::system::error_code&);
    boost::posix_time::ptime next_cycle_time();

    void increment_slot();

    void restart_timer();
    void stop_timer();

    unsigned cycle_sum();
    void position_blank();

    // allowed offset from actual end of slot
    enum
    {
        ALLOWED_SKEW_SECONDS = 2
    };

  private:
    MACManager(const MACManager&);
    MACManager& operator=(const MACManager&);

    protobuf::MACConfig cfg_;

    // asynchronous timer
    boost::asio::io_service io_;

    boost::asio::basic_deadline_timer<goby::common::GobyTime> timer_;
    // give the io_service some work to do forever
    boost::asio::io_service::work work_;

    boost::posix_time::ptime next_cycle_t_;
    boost::posix_time::ptime next_slot_t_;

    std::list<protobuf::ModemTransmission>::iterator current_slot_;

    unsigned cycles_since_reference_;

    bool started_up_;

    std::string glog_mac_group_;
    static int count_;
};

namespace protobuf
{
inline bool operator==(const ModemTransmission& a, const ModemTransmission& b)
{
    return a.SerializeAsString() == b.SerializeAsString();
}
} // namespace protobuf

inline std::ostream& operator<<(std::ostream& os, const MACManager& mac)
{
    for (std::list<protobuf::ModemTransmission>::const_iterator it = mac.begin(), n = mac.end();
         it != n; ++it)
    { os << *it; } return os;
}

} // namespace acomms
} // namespace goby

#endif
