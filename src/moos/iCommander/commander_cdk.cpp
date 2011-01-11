// t. schneider tes@mit.edu 02.24.09
// ocean engineering graduate student - mit / whoi joint program
// massachusetts institute of technology (mit)
// laboratory for autonomous marine sensing systems (lamss)
// 
// this is commander_cdk.cpp
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

#include "commander_cdk.h"

using namespace std;

static int matrixPostProcessCB(EObjectType cdkType,
                               void *object,
                               void *clientData,
                               chtype input)
{
    CDKMATRIX *matrix = (CDKMATRIX *) object;
    int row = getCDKMatrixRow(matrix);
    int col = getCDKMatrixCol(matrix);
    char * crow_val  =  getCDKMatrixCell (matrix, row, 1);
    string row_val = crow_val;
    
    if(post_cb_)
    {        
        if(post_cb_(row, col, row_val, input))
        {
            drawCDKMatrix(matrix, true);
            return 1;
        }
        else
            matrixPostProcessCB(vMATRIX,matrix,NULL,KEY_ESC);
    }
 

    char buff [1][row_val.size()+1];    
    strcpy(buff[0], row_val.c_str()); crow_val = buff[0];

    setCDKMatrixCell(matrix, row, col, crow_val);   
    drawCDKMatrix(matrix, true);
    

    return 0;
}

static int matrixPreProcessCB(EObjectType cdkType,
                              void *object,
                              void *clientData,
                              chtype input)
{

    CDKMATRIX *matrix = (CDKMATRIX *) object;
    int row = getCDKMatrixRow(matrix);
    int col = getCDKMatrixCol(matrix);
    char * crow_val  =  getCDKMatrixCell (matrix, row, 1);
    string row_val = crow_val;
    
    if(pre_cb_)
    {        
        if(pre_cb_(row, col, row_val, input))
            return 1;

        else
            matrixPostProcessCB(vMATRIX,matrix,NULL,KEY_DC);
    }

    char buff [1][row_val.size()+1];    
    strcpy(buff[0], row_val.c_str()); crow_val = buff[0];

    setCDKMatrixCell(matrix, row, col, crow_val);   

    drawCDKMatrix(matrix, true);    
    return 0;
}

void CommanderCdk::cursor_on()
{
    curs_set(1);
}

void CommanderCdk::cursor_off()
{
    curs_set(0);
}


void CommanderCdk::initialize()
{   
    cursesWin_ = initscr ();

    keypad(cursesWin_, true);
    notimeout(cursesWin_, true);
    ESCDELAY = 0;
    
    cursor_off();
    
    cdkscreen_ = initCDKScreen (cursesWin_);


    initCDKColor ();
    
    initialized_ = true;
}

void CommanderCdk::cleanup()
{
    destroyCDKScreen (cdkscreen_);
    endCDK ();
}


bool CommanderCdk::disp_scroll(std::string title, std::vector<std::string> buttons, int & selection, bool do_split_title)
{
    if(!initialized_)
    {
        selection = 0;
        return false;
    }
    
    CDKSCROLL *scroll;
    char * ctitle;
    vector<char *> cbuttons;

    int max_x, max_y;
    getmaxyx(cursesWin_, max_y, max_x);

    int height = min(GENERIC_HEIGHT,max_y);
    int width = min(GENERIC_WIDTH,max_x);

    if (do_split_title)
        title = tes::word_wrap(title, width, ",");
        
    // figure out how big the buffer needs to be
    size_t max_length = title.length();

    for (vector<string>::size_type i = 0, n = buttons.size(); i < n; ++i)
    {
        buttons[i] = "> " + buttons[i];
        max_length = max(max_length, buttons[i].length());
    }
    
    // store room for title (1 space) and all buttons
    char buff [1+buttons.size()][max_length+1];

    // this whole mess is because CDK wants a freaking char ** (not const char **)
    strcpy(buff[0], title.c_str()); ctitle = buff[0];
    for (vector<string>::size_type i = 0, n = buttons.size(); i < n; ++i)
    {
        strcpy(buff[i+1], buttons[i].c_str()); cbuttons.push_back(buff[i+1]);
    }
  
    scroll = newCDKScroll (cdkscreen_,
                           CENTER,
                           CENTER,
                           NONE,
                           height,
                           width,
                           ctitle,
                           &cbuttons[0],
                           cbuttons.size(),
                           false,
                           A_REVERSE,
                           box_,
                           shadow_);

    if (scroll == 0)
        fail();

    top_widget_object_ = scroll;
    top_widget_type_ = vSCROLL;    

    selection = activateCDKScroll (scroll, (chtype *) 0);
    destroyCDKScroll (scroll);

    top_widget_object_ = NULL;
    
    if(scroll->exitType == vESCAPE_HIT)
        return false;
    else
        return true;
}


bool CommanderCdk::disp_alphalist(const string title,
                                 const string label,
                                 const vector<string> & items,
                                 int & selected)
{
    if(!initialized_)
    {
        selected = 0;
        return false;
    }

    cursor_on();
    
    
        
    CDKALPHALIST * alphalist;
    
    char * ctitle;
    char * clabel;
    vector<char *> citems;

    int max_x, max_y;
    getmaxyx(cursesWin_, max_y, max_x);
    
    // figure out how big the buffer needs to be
    size_t max_length = title.length();
    max_length = max(max_length, label.length());

    for (vector<string>::size_type i = 0, n = items.size(); i < n; ++i)
    {
        max_length = max(max_length, items[i].length());
    }
    
    // title, label, items
    char buff [1+1+items.size()][max_length+1];


    
    strcpy(buff[0], title.c_str()); ctitle = buff[0];
    strcpy(buff[1], label.c_str()); clabel = buff[1];
    for (vector<string>::size_type i = 0, n = items.size(); i < n; ++i)
    {
        strcpy(buff[i+2], items[i].c_str()); citems.push_back(buff[i+2]);
    }
    
    /* Create the alpha list widget. */
    alphalist = newCDKAlphalist (cdkscreen_,
                                 CENTER,
                                 CENTER,
                                 min(GENERIC_HEIGHT,max_y), 
                                 min(GENERIC_WIDTH,max_x), 
                                 ctitle,
                                 clabel,
                                 &citems[0],
                                 citems.size(),
                                 '_',
                                 A_REVERSE,
                                 box_,
                                 shadow_);
    
    if (alphalist == 0)
        fail();

    setCDKAlphalistCurrentItem (alphalist, 0);
    
    char * message = activateCDKAlphalist (alphalist, 0);
    string selected_message;
    if(message)
        selected_message = message;        

    selected = -1;
    // check if valid message
    for (vector<string>::size_type i = 0, n = items.size(); i < n; ++i)
    {
        if (tes::stricmp(items[i], selected_message))
            selected = i;
    }

    if(selected == -1 && alphalist->exitType != vESCAPE_HIT)
    {
        destroyCDKAlphalist (alphalist);
        disp_alphalist(title, label, items, selected);
    }

    destroyCDKAlphalist (alphalist);

    cursor_off();
    
    
    if(alphalist->exitType == vESCAPE_HIT)
        return false;
    else
        return true;

}


bool CommanderCdk::disp_matrix(const string title,
                               int rows,
                               int cols,
                               int fieldwidth,
                               const vector<string> & rowtitles,
                               const vector<string> & coltitles,
                               vector<string > & values,
                               cdk_callback pre_cb,
                               cdk_callback post_cb,
                               int startx,
                               int starty)
{
    if(!initialized_)
        return false;
    
    
    post_cb_ = post_cb;
    pre_cb_ = pre_cb;

    
//    CDKMATRIX * matrix;

    // figure out how big the buffer needs to be
    size_t max_length = title.length();

    size_t max_rowtitlelength = 0;
    for (vector<string>::size_type i = 0, n = rowtitles.size(); i < n; ++i)
    {
        max_rowtitlelength = max(max_rowtitlelength, rowtitles[i].length());
    }
    size_t max_coltitlelength = 0;
    for (vector<string>::size_type i = 0, n = coltitles.size(); i < n; ++i)
    {
        max_coltitlelength = max(max_coltitlelength, coltitles[i].length());
    }
    
    max_length = max(max_rowtitlelength, max_length);
    max_length = max(max_coltitlelength, max_length);
    

    int title_newlines = 0;
    for(string::size_type i = 0, n = title.length(); i < n; ++i)
    {
        if(title[i] == '\n')
            ++title_newlines;
    }

    int vcols, vrows = 0;
    while(vcols<=0 || vrows<=0)
    {
        int max_x, max_y;
        getmaxyx(cursesWin_, max_y, max_x);
// visible columns / rows
        // each row takes width + 2 + title length + 4  + 6 for safety
        vcols = (int)min((double)cols, floor((max_x - 4 - 6 - max_rowtitlelength) / (fieldwidth+2)));
        // each row takes 3 + title length + 3 constant + 4 for safety
        vrows = (int)min((double)rows, floor((max_y - lower_box_size_ - 3 - 4 - (title_newlines + 1)) / (3)));
        if(vcols<=0 || vrows<=0)
            disp_scroll("You must enlarge your window!", vector<string>(1, "OK"));
    }
    
                
    
    // cols + 1 because for some dumb reason the matrix widget starts
    // matrix numbering at 1 not 0
    int colwidth[cols+1];
    int colvalue[cols+1];
    char * ccoltitles[cols+1];
    char * crowtitles[rows+1];
    char * ctitle;
    
    char title_buff [1][max_length+1];
    char rowtitle_buff [rowtitles.size()][max_length+1];
    char coltitle_buff [coltitles.size()][max_length+1];

    strcpy(title_buff[0], title.c_str()); ctitle = title_buff[0];

    size_t num_row_titles = rowtitles.size();
    size_t num_col_titles = coltitles.size();
    for (vector<string>::size_type i = 0, n = rows; i < n; ++i)
    {
        if (i < num_row_titles)
        {
            strcpy(rowtitle_buff[i], rowtitles[i].c_str()); crowtitles[i+1] = rowtitle_buff[i];
        }
        else
            crowtitles[i+1] = 0;
    }
    for (vector<string>::size_type i = 0, n = cols; i < n; ++i)
    {
        if (i < num_col_titles)
        {
            strcpy(coltitle_buff[i], coltitles[i].c_str()); ccoltitles[i+1] = coltitle_buff[i];
        }
        else
            ccoltitles[i+1] = 0;

        colwidth[i+1] = fieldwidth;
        colvalue[i+1] = vMIXED;
    }    

    
    /* Create the matrix object. */
    matrix_ = newCDKMatrix (cdkscreen_,
                           startx,
                           starty,
                           rows,
                           cols,
                           vrows,
                           vcols,
                           ctitle,
                           crowtitles,
                           ccoltitles,
                           colwidth,
                           colvalue,
                           1,
                           1,
                           ' ',
                           COL,
                           true,
                           true,
                           false);

    int max_length2 = 0;
    for (vector<string>::size_type i = 0, n = values.size(); i < n; ++i)
    {
        max_length2 = max(max_length2, (int)values[i].length());
    }

    size_t num_values = values.size();
    for (size_t i = 0, n = cols; i < n; ++i)
    {
        for (size_t j = 0, m = rows; j < m; ++j)
        {

            if ((j+i*rows) < num_values)
            {
                char value_buff[1][max_length2];
                char * cvalue;
                strcpy(value_buff[1], values[j+i*rows].c_str()); cvalue = value_buff[1];

                setCDKMatrixCell (matrix_,
                                  j+1,
                                  i+1,
                                  cvalue);
                    
            }
        }
    }


        
        
    setCDKMatrixPostProcess (matrix_, matrixPostProcessCB, 0);
    setCDKMatrixPreProcess (matrix_, matrixPreProcessCB, 0);
    
    matrixPostProcessCB(vMATRIX,
                        matrix_,
                        NULL,
                        KEY_DC);

    refresh();
    
    /* Activate the matrix. */
    activateCDKMatrix (matrix_, 0);
    

    values.clear();
    for (size_t i = 0, n = cols; i < n; ++i)
    {
        for (size_t j = 0, m = rows; j < m; ++j)
        {
            char * cval = getCDKMatrixCell (matrix_,
                                            j+1,
                                            i+1);

            string val = cval;
            values.push_back(val);
                               
        }
    }

        
    destroyCDKMatrix (matrix_);
    
    destroyCDKLabel (info_box_);
    info_box_set_ = false;

    if(matrix_->exitType == vESCAPE_HIT)
    {
        matrix_ = NULL;
        return false;
    }            
    else
    {
        matrix_ = NULL;
        return true;
    }
}


void CommanderCdk::refresh()
{
    if(lower_info_box_set_)
        drawCDKLabel(lower_info_box_,
                     true);

        
    refreshCDKScreen(cdkscreen_);
}

bool CommanderCdk::disp_lower_info(const vector<string> & lines)
{
    if(!initialized_)
        return false;

    destroyCDKLabel (lower_info_box_);
     
    size_t max_length = 0;
    for (vector<string>::size_type i = 0, n = lines.size(); i < n; ++i)
    {
        max_length = max(max_length, lines[i].length());
    }

    char * clines[lines.size()];
    char buff [lines.size()][max_length+1];
    for (vector<string>::size_type i = 0, n = lines.size(); i < n; ++i)
    {
        strcpy(buff[i], lines[i].c_str()); clines[i] = buff[i];
    }    

    lower_info_box_ = newCDKLabel (cdkscreen_,
                                   CENTER,
                                   BOTTOM,
                                   clines,
                                   lines.size(),
                                   box_,
                                   shadow_);
    drawCDKLabel(lower_info_box_,
                 true);
    

    restore_widgets();
    
    return true;
}

void CommanderCdk::restore_widgets()
{
    if(matrix_ != NULL && top_widget_object_ == NULL)
    {
        drawCDKMatrix(matrix_, true);
    }
}

bool CommanderCdk::disp_info(const vector<string> & lines)
{
    if(!initialized_)
        return false;

    destroyCDKLabel (info_box_);
     
    size_t max_length = 0;
    for (vector<string>::size_type i = 0, n = lines.size(); i < n; ++i)
    {
        max_length = max(max_length, lines[i].length());
    }

    char * clines[lines.size()];
    char buff [lines.size()][max_length+1];
    for (vector<string>::size_type i = 0, n = lines.size(); i < n; ++i)
    {
        strcpy(buff[i], lines[i].c_str()); clines[i] = buff[i];
    }    
    
    info_box_ = newCDKLabel (cdkscreen_,
                             CENTER,
                             TOP,
                             clines,
                             lines.size(),
                             box_,
                             shadow_);
    drawCDKLabel(info_box_,
                 true);
    
    
    restore_widgets();

    return true;
}



bool CommanderCdk::disp_fselect(const string title, const string label, std::string & name)
{
    if(!initialized_)
        return false;

    cursor_on();    
    
    CDKFSELECT * fselect;
    
    char * filename;
    char * ctitle;
    char * clabel;

    int max_x, max_y;
    getmaxyx(cursesWin_, max_y, max_x);
    
    // figure out how big the buffer needs to be
    size_t max_length = title.length();
    max_length = max(max_length, label.length());
   
    // title, label, items
    char buff [2+4][max_length+1];    
    strcpy(buff[0], title.c_str()); ctitle = buff[0];
    strcpy(buff[1], label.c_str()); clabel = buff[1];

    strcpy(buff[2], "");
    strcpy(buff[3], "");
    strcpy(buff[4], "");
    strcpy(buff[5], "");
    
/* Create the alpha list widget. */
    fselect = newCDKFselect (cdkscreen_,
                             CENTER,
                             CENTER,
                             min(GENERIC_HEIGHT,max_y), 
                             min(GENERIC_WIDTH,max_x), 
                             ctitle,
                             clabel,
                             A_NORMAL,
                             ' ',
                             A_REVERSE,
                             buff[2],
                             buff[3],
                             buff[4],
                             buff[5],
                             box_,
                             shadow_);
    
    if (fselect == 0)
        fail();
    
    filename = activateCDKFselect (fselect, 0);
    filename = copyChar (filename);

    if(filename)
        name = filename;    

    destroyCDKFselect (fselect);

    cursor_off();
    
    if(fselect->exitType == vESCAPE_HIT)
        return false;
    else
        return true;
}

void CommanderCdk::set_matrix_val(int row, int col, std::string s)
{
    if(matrix_ != NULL)
    {
        char buff [s.length()+1];
        char * cs;
        
        strcpy(buff, s.c_str()); cs = buff;
        
        setCDKMatrixCell(matrix_, row, col, cs);
    }
}

void CommanderCdk::get_matrix_val(int row, int col, std::string & s)
{
    if(matrix_ != NULL)
        s = string(getCDKMatrixCell(matrix_, row, col));
}

void CommanderCdk::resize()
{
    destroyCDKLabel (lower_info_box_);
}


void CommanderCdk::fail()
{
    cleanup();
    
    cout << "Oops. Can't seem to create the dialog box. " << endl;
    cout << "Is the window too small?";
    exit(EXIT_FAILURE);
}

