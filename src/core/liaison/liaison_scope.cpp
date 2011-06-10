// copyright 2011 t. schneider tes@mit.edu
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

#include "liaison_scope.h"

#include <Wt/WStandardItemModel>
#include <Wt/WStandardItem>
#include <Wt/WTreeView>
#include <Wt/WPanel>
#include <Wt/WSortFilterProxyModel>


using namespace Wt;


goby::core::LiaisonScope::LiaisonScope(ZeroMQService* service)
    : MOOSNode(service)
{
    text_ = new Wt::WText("placeholder", this);
    
    int topLevelRows = 5;
    int secondLevelRows = 7;
    int thirdLevelRows = 3;

    model_ = new Wt::WStandardItemModel(0,1, this);
    Wt::WStandardItem *root = model_->invisibleRootItem();
    
    model_->setHeaderData(0, Horizontal, std::string("Type Name"));  

    
    for (int row = 0; row < topLevelRows; ++row) {
        Wt::WStandardItem *topLevel = new Wt::WStandardItem();
        topLevel->setText("Item " + boost::lexical_cast<std::string>(row));
        for (int row2 = 0; row2 < secondLevelRows; ++row2) {
            Wt::WStandardItem *item = new Wt::WStandardItem();
            item->setText("Item " + boost::lexical_cast<std::string>(row)
                          + ": " + boost::lexical_cast<std::string>(row2));
            for (int row3 = 0; row3 < thirdLevelRows; ++row3) {
                Wt::WStandardItem *item2 = new Wt::WStandardItem();
                item2->setText("Item " + boost::lexical_cast<std::string>(row)
                              + ": " + boost::lexical_cast<std::string>(row2)
                              + ": " + boost::lexical_cast<std::string>(row3));
                item->appendRow(item2);
            }
            topLevel->appendRow(item);
        }
        root->appendRow(topLevel);
    }    

    // Create the WTreeView
    Wt::WTreeView *gitView = new Wt::WTreeView(this);
//    gitView->resize(Wt::WLength::Auto, Wt::WLength::Auto);
    gitView->setModel(model_);
    gitView->setSelectionMode(Wt::SingleSelection);


    WSortFilterProxyModel* filteredSortedCocktails = new WSortFilterProxyModel(this);
    filteredSortedCocktails->setSourceModel(model_);
    filteredSortedCocktails->setDynamicSortFilter(true);
    filteredSortedCocktails->setFilterKeyColumn(0);
    filteredSortedCocktails->setFilterRole(Wt::DisplayRole);
    filteredSortedCocktails->setFilterRegExp(".*");
    filteredSortedCocktails->sort(0);
    
    // // Create the WTreeView
    // Wt::WTreeView *gitView2 = new Wt::WTreeView(this);
    // gitView2->resize(300, Wt::WLength::Auto);
    // gitView2->setModel(filteredSortedCocktails);
    // gitView2->setSelectionMode(Wt::SingleSelection);
    
//    addWidget(gitView);
}


void goby::core::LiaisonScope::moos_inbox(CMOOSMsg& msg)
{
    using namespace goby::util::logger_lock;

    using goby::moos::operator<<;
    
    glog.is(debug1, lock) && glog << "LiaisonScope: got message:  " << msg << std::endl << unlock;


    // exclusive access to app state
    Wt::WStandardItem* root = model_->invisibleRootItem();
    Wt::WStandardItem* topLevel = new Wt::WStandardItem();
    std::stringstream ss;
    ss << msg;
    topLevel->setText(ss.str());
    model_->setItem(0, 0, topLevel);
        
    text_->setText(ss.str());
}

