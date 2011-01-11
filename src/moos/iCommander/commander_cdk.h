// t. schneider tes@mit.edu 02.24.09
// ocean engineering graduate student - mit / whoi joint program
// massachusetts institute of technology (mit)
// laboratory for autonomous marine sensing systems (lamss)
// 
// this is commander_cdk.h
//
// see the readme file within this directory for information
// pertaining to usage and purpose of this script.
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

#include <vector>
#include <string>
#include <iostream>
#include <cmath>

#include <cdk/cdk.h>
#include <boost/function.hpp>

#include "goby/moos/lib_tes_util/tes_string.h"

typedef boost::function<bool(int, int, std::string &, chtype)> cdk_callback;

static cdk_callback post_cb_;
static cdk_callback pre_cb_;

const int GENERIC_WIDTH = 60;
const int GENERIC_HEIGHT = 25;

class CommanderCdk
{
  public:
  CommanderCdk()
      : matrix_(NULL),
        box_(true),
        shadow_(false),
        info_box_set_(false),
        lower_info_box_set_(false),
        initialized_(false),
        matrix_interrupt_(false),
        lower_box_size_(0)
        { }
    
    ~CommanderCdk() { cleanup(); }
    
    void initialize();
    void cleanup();
    
    bool disp_scroll(std::string title,
                    std::vector<std::string>  buttons,
                     int & selection,
                     bool do_split_title = false);

    bool disp_scroll(std::string title,
                     std::vector<std::string>  buttons)
    {
        int ignored;
        return disp_scroll(title, buttons, ignored);
    }
    
    bool disp_alphalist(const std::string title,
                       const std::string label,
                       const std::vector<std::string> & items,
                       int & selection);
    
    bool disp_matrix(const std::string title,
                     int rows,
                     int cols,
                     int fieldwidth,
                     const std::vector<std::string> & rowtitles,
                     const std::vector<std::string> & coltitles,
                     std::vector<std::string> & values,
                     cdk_callback pre_cb = NULL,
                     cdk_callback post_cb = NULL,
                     int startx = CENTER,
                     int starty = CENTER);

    bool disp_matrix_popup(std::string title,
                           std::vector<std::string> buttons);
    
    
    bool disp_fselect(const std::string title,
                      const std::string label,
                      std::string & name);    
    
    bool disp_info(const std::vector<std::string> & lines);
    bool disp_lower_info(const std::vector<std::string> & lines);

    void refresh();

    bool initialized () {return initialized_;}


    void cursor_on();
    void cursor_off();

    void set_matrix_val(int row, int col, std::string s);
    void get_matrix_val(int row, int col, std::string & s);
    void set_lower_box_size(int size) { lower_box_size_ = size; }

    void resize();

  private:
    void fail();
    void restore_widgets();
    
  private:
    WINDOW *cursesWin_;
    CDKSCREEN * cdkscreen_;
    
    CDKLABEL * info_box_;
    CDKLABEL * lower_info_box_;

    CDKMATRIX * matrix_;
    
    bool box_;
    bool shadow_;
    bool info_box_set_;
    bool lower_info_box_set_;
    bool initialized_;

    bool matrix_interrupt_;       

    int lower_box_size_;

    EObjectType top_widget_type_;
    void * top_widget_object_;

};

