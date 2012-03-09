
#include "message_var_float.h"

#include "goby/util/sci.h"

goby::transitional::DCCLMessageVarFloat::DCCLMessageVarFloat(double max /*= std::numeric_limits<double>::max()*/, double min /*= 0*/, double precision /*= 0*/)
    : DCCLMessageVar(),
      max_(max),
      min_(min),
      precision_(precision),
      max_delta_(std::numeric_limits<double>::quiet_NaN())
{ }

        
void goby::transitional::DCCLMessageVarFloat::initialize_specific()
{
    // flip max and min if needed
    if(max_ < min_)
    {
        double tmp = max_;
        max_ = min_;
        min_ = tmp;
    } 

    if(using_delta_differencing() && max_delta_ < 0)
        max_delta_ = -max_delta_;
}

void goby::transitional::DCCLMessageVarFloat::pre_encode(DCCLMessageVal& v)
{
    double r;
    if(v.get(r)) v = util::unbiased_round(r,precision_);
}

