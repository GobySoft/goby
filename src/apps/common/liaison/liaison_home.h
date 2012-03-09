
#ifndef LIAISONHOME20110609H
#define LIAISONHOME20110609H

#include <Wt/WText>
#include <Wt/WCssDecorationStyle>
#include <Wt/WBorder>
#include <Wt/WColor>
#include <Wt/WVBoxLayout>

#include "liaison.h"

namespace goby
{
    namespace common
    {
        class LiaisonHome : public LiaisonContainer
        {
          public:
            LiaisonHome(Wt::WContainerWidget* parent = 0);

          private:
            Wt::WVBoxLayout* main_layout_;

        };
    }
}

#endif
