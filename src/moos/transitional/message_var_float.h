
#ifndef MESSAGE_VAR_FLOAT20100317H
#define MESSAGE_VAR_FLOAT20100317H

#include "goby/util/as.h"

#include "message_var.h"

#include <boost/math/special_functions/fpclassify.hpp>

namespace goby
{
    namespace transitional
    {   
        class DCCLMessageVarFloat : public DCCLMessageVar
        {
          public:
            DCCLMessageVarFloat(double max = std::numeric_limits<double>::max(), double min = 0, double precision = 0);

        
            void set_max(double max) {max_ = max;}
            void set_max(const std::string& s) { set_max(util::as<double>(s)); }
        
            void set_min(double min) {min_ = min;}
            void set_min(const std::string& s) { set_min(util::as<double>(s)); }

            void set_precision(int precision) {precision_ = precision;}
            void set_precision(const std::string& s) { set_precision(util::as<int>(s)); }
        
            int precision() const { return precision_; }  

            double min() const { return min_; }
            double max() const { return max_; }
        
            void set_max_delta(double max_delta)
            { max_delta_ = max_delta; }
            void set_max_delta(const std::string& s)
            { set_max_delta(util::as<double>(s)); }

            virtual DCCLType type() const { return dccl_float; }


          protected:
            virtual DCCLMessageVal cast(double d, int precision) { return DCCLMessageVal(d, precision); }
            virtual void initialize_specific();
            
            virtual void pre_encode(DCCLMessageVal& v);
            
            
          private:
            bool is_delta() const
            { return using_delta_differencing() && !is_key_frame_; }
        
            bool using_delta_differencing() const
            { return !(boost::math::isnan)(max_delta_); }

        
          private:
            double max_;
            double min_;
            int precision_;

            double max_delta_;
        };

    }
}
#endif
