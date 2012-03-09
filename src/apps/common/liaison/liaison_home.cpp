
#include "liaison_home.h"

using namespace Wt;


goby::common::LiaisonHome::LiaisonHome(Wt::WContainerWidget* parent)
    : LiaisonContainer(parent),
      main_layout_(new Wt::WVBoxLayout(this))
{
    Wt::WContainerWidget* top_text = new Wt::WContainerWidget(this);
    main_layout_->addWidget(top_text);
    
    top_text->addWidget(new WText("Welcome to Goby Liaison: an extensible tool for commanding and comprehending this Goby platform."));
    top_text->addWidget(new WBreak());
    top_text->addWidget(new WText("<i>liaison (n): one that establishes and maintains communication for mutual understanding and cooperation</i>"));
}

