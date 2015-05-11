

#ifndef IridiumDriverCommon20150508H
#define IridiumDriverCommon20150508H

namespace goby
{
    namespace acomms
    {
        enum { RATE_RUDICS = 1, RATE_SBD = 0 };    

        class OnCallBase
        {
          public:
          OnCallBase()
              : last_tx_time_(goby::common::goby_time<double>()),
                last_rx_time_(0),
                bye_received_(false),
                bye_sent_(false)
                { }
                
            double last_rx_tx_time() const { return std::max(last_tx_time_, last_rx_time_); }
            double last_rx_time() const { return last_rx_time_; }
            double last_tx_time() const { return last_tx_time_; }
                
                
            void set_bye_received(bool b) { bye_received_ = b; }
            void set_bye_sent(bool b) { bye_sent_ = b; }
                
            bool bye_received() const { return bye_received_; }
            bool bye_sent() const { return bye_sent_; }
                
            void set_last_tx_time(double d) { last_tx_time_ = d; }
            void set_last_rx_time(double d) { last_rx_time_ = d; }

          private:
            double last_tx_time_;            
            double last_rx_time_;            
            bool bye_received_;
            bool bye_sent_;     
        };
            
    }
}

#endif
