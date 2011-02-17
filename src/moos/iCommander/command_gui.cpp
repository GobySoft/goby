#include "command_gui.h"

using namespace std;
using boost::trim_copy;
using goby::util::stricmp;

using namespace goby::acomms;
using goby::util::as;

//using namespace boost::posix_time;

// Construction / Destruction
CommandGui::CommandGui(CommanderCdk & gui,
                       boost::mutex & gui_mutex,
                       CMOOSGeodesy & geodesy,
                       tes::ModemIdConvert & modem_lookup,
                       CMOOSCommClient & comms,
                       DCCLCodec & dccl,
                       std::vector<std::string> & loads,
                       std::string & community,
                       bool xy_only)
    : initialized_(false),
      gui_(gui),
      gui_mutex_(gui_mutex),
      geodesy_(geodesy),
      modem_lookup_(modem_lookup),
      m_Comms(comms),
      dccl_(dccl),
      loads_(loads),
      community_(community),
      xy_only_(xy_only),
      refresh_(false),
      curr_mess_vals_(NULL),
      screen_initialized_(false),
      inserting_x_(false),
      inserting_y_(false)
{ }

CommandGui::~CommandGui()
{ }

bool CommandGui::run()
{
    while(1)
    {
        if(initialized_)
            do_run(); // do the real loop
        else
            sleep(1); // wait for iCommander's startup and read to finish
    }
    return true;
}

void CommandGui::do_run()
{
    if(!screen_initialized_)
    {            
        gui_.refresh();
        state_ = s_menu;
        screen_initialized_ = true;
    }
        
    switch (state_)
    {
        default:
        case s_menu:
            main_menu();            
            break;
                
        case s_new:
            new_message();
            break;
                
        case s_open:
            open_message();
            break;
                
        case s_save:
            save_message();
            break;
                
        case s_import:
            import_xml();
            break;
                
        case s_exit:
            gui_.cleanup();
            exit(EXIT_SUCCESS);            
            break;
                
        case s_editing:
            edit();
            break;
                
        case s_preview:
            preview();
            break;
                
        case s_send:
            send();
            break;
                
        case s_quick_switch:
            quick_switch();
            state_ = s_editing;
            break;
                
        case s_clear:
            clear_message();
            state_ = s_editing;
            break;
    }
}

void CommandGui::main_menu()
{

    string title = string("<C></B>iCommander: Vehicle Command Message Sender\n") +
        string("<C> (C) 2009 t. schneider tes@mit.edu \n") +
        string("<C>") + boost::lexical_cast<std::string>(dccl_.message_count()) + string(" message(s) loaded.\n") +
        string("</B/40>Main Menu:<!40>");

//    for(int i = 0; i < 64; ++i)
//                title += "</" + as<string>(i) + ">" +  as<string>(i) + "<!"+ as<string>(i) + ">";
    

    vector<string> buttons;
    if (curr_mess_vals_ != NULL && !curr_mess_vals_->empty())
        buttons.push_back("Return to active message");
    buttons.push_back("Select Message");
    buttons.push_back("Load");
    buttons.push_back("Save");    
    buttons.push_back("Import Message File (XML)");
    buttons.push_back("Exit");

    int selection;
    gui_.disp_scroll(title, buttons, selection);
    
    
    if (curr_mess_vals_ == NULL || curr_mess_vals_->empty())
        state_ = (state)selection;
    else
        state_ = (selection == 0) ? s_editing : (state)(selection - 1);
}


void CommandGui::new_message()
{
    if(!dccl_.message_count())
    {
        gui_.disp_scroll(string("No messages defined. Import a message file or specify them in the moos file"), vector<string>(1, "OK"));
        state_ = s_menu;
        return;
    }
        
    string title = "</B/40>Available Messages<!40> (can use tab completion):";
    string label = "Message to create: ";
    vector<string> items;
    
    for (vector<DCCLMessage>::iterator it = dccl_.messages().begin(), n = dccl_.messages().end(); it != n; ++it)
        items.push_back(it->name());

    int selected;
    if(gui_.disp_alphalist(title, label, items, selected))
    {
        curr_mess_vals_ = &open_mess_vals_[selected];
        curr_message_ = &dccl_.messages()[selected];
        state_ = s_editing;
    }
    else
    {
        state_ = s_menu;
    }
}

void CommandGui::open_message()
{    
    string title = "Choose a file to open (ESC to cancel):";
    string label = "File name: ";
    string filename;

    if(gui_.disp_fselect(title, label, filename) && do_open(filename))
        gui_.disp_scroll(string("File read in:\n" + filename), vector<string>(1, "OK"));        
    
    state_ = s_menu;
}

void CommandGui::save_message()
{
    string title = "Choose a file to save to (ESC to cancel):";
    string label = "File name: ";

    string filename;

    if(gui_.disp_fselect(title, label, filename))
    {        
       
        vector<string> v;
        v.push_back("Save");
        v.push_back("Cancel");
        int selection;
        gui_.disp_scroll(string("Confirm save message to:\n" + filename), v, selection);    
        if(selection == 0)
        {
            if(do_save(filename))
            {
                gui_.disp_scroll(string("Message saved to:\n" + filename), vector<string>(1, "OK"));
            }
        }
    }

    state_ = s_menu;    
}

bool CommandGui::do_save(string filename)
{
    ofstream fout;
    fout.open(filename.c_str());

    vector<string> * save_curr_mess_vals;
    DCCLMessage * save_curr_message;

    if(fout.is_open())
    {
        string date = MOOSGetDate();
        fout << "%%% iCommander message save file: " << date.substr(0,date.length()-1) << " %%%" << endl;
        
        for(std::map<int, std::vector<std::string> >::iterator it = open_mess_vals_.begin(), n = open_mess_vals_.end(); it != n; ++it)
        {
            save_curr_mess_vals = &(it->second);
            save_curr_message = &dccl_.messages()[it->first];
            fout << "message::" << save_curr_message->name() << endl;
            std::map<std::string, std::string> vals;
            vector<string> unused;
            prepare_message(vals, unused, false, save_curr_mess_vals, save_curr_message);
            for(map<string, string>::const_iterator it = vals.begin(); it != vals.end(); ++it)
            {
                string variable = it->first;
                string contents = it->second;
              
                if(it != vals.begin())
                    fout << " ";
              
                fout << variable << ":=" << contents;
            }
            fout << endl;
          
        }
        fout.close();
        return true;
      
    }
  
    return false;
}

bool CommandGui::do_open(string filename)
{
    vector<string> * open_curr_mess_vals = NULL;
    DCCLMessage * open_curr_message = NULL;

    ifstream fin;
    fin.open(filename.c_str());
    
    if(fin.is_open())
    {
        string line;
        getline(fin, line);
        string::size_type pos = line.find("%%% iCommander message save file: ");
        if(pos == string::npos)
        {
            if(gui_.initialized())
                gui_.disp_scroll(string("Invalid file:\n" + filename), vector<string>(1, "OK"));
            state_ = s_menu;
            return false;
        }
        
        while(getline(fin,line))
        {
            pos = line.find("message::");
            if(pos != string::npos)
            {
                string new_message = line.substr(string("message::").length());
                
                for(vector<DCCLMessage>::size_type i = 0, n = dccl_.messages().size(); i < n; ++i)
                {
                    if(new_message == dccl_.messages()[i].name())
                    {
                        open_curr_mess_vals = &open_mess_vals_[i];
                        open_curr_message = &dccl_.messages()[i];
                        open_curr_mess_vals->clear();
                    }
                }
            }
            else if(open_curr_mess_vals != NULL && open_curr_message != NULL)
            {    
                //replace spaces with commas
                string::size_type pos2 = line.find(" ");
                while(pos2 != string::npos)
                {
                    line[pos2] = ',';
                    pos2 = line.find(" ");
                }
              
                vector< boost::shared_ptr<DCCLMessageVar> > message_vars =
                    fetch_message_vars(open_curr_message);

                for (vector< boost::shared_ptr<DCCLMessageVar> >::size_type i = 0, n = message_vars.size(); i < n; ++i)
                {
                    boost::shared_ptr<DCCLMessageVar> mp = message_vars.at(i);
                    string piece;
                    if(MOOSValFromString(piece, line, mp->name()))
                        open_curr_mess_vals->push_back(piece);
                }
            }
        }
      
        
        fin.close();
        return true;
    }
    return false;
}

void CommandGui::import_xml()
{
    string title = "Choose an xml message file to import (ESC to cancel):";
    string label = "File name: ";

    string filename;
    if(gui_.disp_fselect(title, label, filename))
    {
        
        
        try
        {
            protobuf::DCCLConfig cfg;
            cfg.add_message_file()->set_path(filename);
            dccl_.merge_cfg(cfg);
            gui_.disp_scroll(string("Imported:\n" + filename), vector<string>(1, "OK"));
        }
        catch(exception & e)
        {
            gui_.disp_scroll(string("Invalid xml file:\n" + filename), vector<string>(1, "OK"));
        }
    }

    state_ = s_menu;
}

void CommandGui::edit()
{
    // actual columns / rows
    vector< boost::shared_ptr<DCCLMessageVar> > message_vars =
        fetch_message_vars(curr_message_);
        
    int rows = message_vars.size();
    int cols = 1;

    curr_mess_vals_->resize(cols*rows);
    
    string title = "</B/40>Message<!40> (Type: " + curr_message_->name() + ")\n" +
        as<string>(message_vars.size()) + " entries total\n"+
        "\t{Enter} for options\n" +
        "\t{Up/Down} for more message variables";
    
    
    int maxwidth = 0;    
    rowtitle_.clear();
    coltitle_.clear();

    for (vector< boost::shared_ptr<DCCLMessageVar> >::size_type i = 0, n = message_vars.size(); i < n; ++i)
    {
        boost::shared_ptr<DCCLMessageVar> mp = message_vars.at(i);

        DCCLType type = mp->type();
        std::string stype;
        
        int width;
        switch(type)
        {
            case dccl_string:
                stype = "</24>string<!24>";
                width = mp->max_length();
                break;
            case dccl_bool:
                stype = "</32>bool<!32>";
                width = 4;
                break;
            case dccl_float:
                stype = "</48>float<!48>";
                // + 1 for decimal point, +1 for e
                width =
                    max((as<string>((int)mp->max())).size(),(as<string>((int)mp->min())).size()) + mp->precision() + 2;
                break;
            case dccl_int:
                stype = "</56>int<!56>";
                width = max((as<string>((int)mp->max())).size(),(as<string>((int)mp->min())).size());
                break;
                
            case dccl_static:
                stype = "static";
                width = mp->static_val().size();
                break;
                
            case dccl_enum:
                stype = "</40>enum<!40>";
                width = 0;
                vector<string>* enums = mp->enums();
                for (vector<string>::size_type j = 0, m = enums->size(); j < m; ++j)
                    width = max((int)enums->at(j).size(), width);
                break;
                    
        }
        
        maxwidth = max(maxwidth, width);

        string t = "</B/24>" + boost::lexical_cast<std::string>(i+1) + "<!B!24>" +
            "/" + boost::lexical_cast<std::string>(n) + ". " + mp->name() + " (" + stype + ")";

        rowtitle_.push_back(t);        
    }

    
    // account for specials
    maxwidth = max(maxwidth, (int)string("_time").length());
    maxwidth = max(maxwidth, (int)string("_x(lon)10:10000").length());
    
    // set static values
    for (vector< boost::shared_ptr<DCCLMessageVar> >::size_type i = 0, n = message_vars.size(); i < n; ++i)
    {
        boost::shared_ptr<DCCLMessageVar> mp = message_vars.at(i);
        DCCLType type = mp->type();
        
        if (type == dccl_static)
            curr_mess_vals_->at(i) = mp->static_val();            
    
    }

    // (boost::bind allows us to use a member function of CommandGui
    // as a callback to the C coded CDK)
    cdk_callback pre =
        boost::bind(&CommandGui::edit_precallback, this, _1, _2, _3, _4);

    cdk_callback post =
        boost::bind(&CommandGui::edit_postcallback, this, _1, _2, _3, _4);

    int startx = CENTER;
    int starty = 4;

    gui_.disp_matrix(title,
                     rows,
                     cols,
                     maxwidth,
                     rowtitle_,
                     coltitle_,
                     *curr_mess_vals_,
                     pre,
                     post,
                     startx,
                     starty);
    gui_.cursor_off();    
}

int CommandGui::edit_popup()
{   
    string title = (string)"<C> Choose an action";
    vector<string> buttons;

    buttons.push_back("Return to message");
    buttons.push_back("</B>Send");
    buttons.push_back("Preview");
    buttons.push_back("Quick switch to another open message");
    buttons.push_back("</B/40>Main Menu<!40>");
    buttons.push_back("Insert special: current time [shortcut: t]");
    buttons.push_back("Insert special: UTM x,y to lon,lat [shortcut: x]");
    buttons.push_back("Insert special: community");
    buttons.push_back("Insert special: modem id [shortcut: m]");
    buttons.push_back("Clear message");
    buttons.push_back("Fix resized screen [shortcut: ESC]");
    
    int selection;
    gui_.disp_scroll(title, buttons, selection);
    
    return selection;
}

bool CommandGui::edit_precallback(int row, int col, string & val, chtype & input)
{
    if(xy_only_)
    {
        for(vector<string>::size_type i = 0, n = rowtitle_.size(); i < n; ++i)
        {
            string aval;
            gui_.get_matrix_val(i+1, 1, aval);
            string::size_type aposy = aval.find("_y:");
            string::size_type aposx = aval.find("_x:");
            if(aposy == string::npos && aposx == string::npos)
            {
                int lat_row, lon_row;
                find_lat_lon_rows(i+1, lat_row, lon_row);
                if(lat_row && lon_row)
                {
                    gui_.set_matrix_val(lon_row, 1, "_x:");
                    gui_.set_matrix_val(lat_row, 1, "_y:");
                }
            }
        }
    }
    
    gui_.cursor_off();

    vector< boost::shared_ptr<DCCLMessageVar> > message_vars =
        fetch_message_vars(curr_message_);
    
    boost::shared_ptr<DCCLMessageVar> mp = message_vars.at(row-1);

    DCCLType type = mp->type();

    // check legality of input
    if(input == KEY_ENTER)
        return edit_menu(row, val, type);
    else if(isprint(input))
        return edit_isprint(row, val, input, type, mp);
//    else if(input == KEY_ESC)
//        return false;
    else if(input == KEY_DC || input == KEY_BACKSPACE)
        return edit_backspace(row, val, type);
    else if(input == KEY_LEFT || input == KEY_RIGHT)
        return edit_leftright(row, val, input, type, mp);    
        
    return true;
}


bool CommandGui::edit_menu(int row, std::string & val, DCCLType type)
{
    switch(edit_popup())
    {
        default:
        case 0: // return to message
            return false;
            break;

        case 1: // send
            state_ = s_send;
            break;
                
        case 2: //preview
            state_ = s_preview;
            break;

        case 3: // quick switch
            state_ = s_quick_switch;
            break;
                
                
        case 4: // main menu
            state_ = s_menu;
            break;

                    
        case 5: // insert special: time
            if (type == dccl_int || type == dccl_string || type == dccl_float)
                check_specials(val, 't', row);
            return false;
            break;
                
        case 6: // insert special: LocalUTM2LatLong
            check_specials(val, 'y', row);                
            return false;
            break;

                
        case 7: // insert special: community
            if (type == dccl_string)
                val = community_;
            return false;
            break;
                
        case 8: // insert special: modem id
            if (type == dccl_int || type == dccl_string || type == dccl_float)
                check_specials(val, 'm', row);
                
            return false;
            break;
                
        case 9: // clear
            state_ = s_clear;
            break;

        case 10: //resize
            gui_.resize();
            break;
                    
    }        
    return true;
}

bool CommandGui::edit_isprint(int row, std::string & val, chtype input, DCCLType type, boost::shared_ptr<DCCLMessageVar> mp)
{       
    string::size_type posy = val.find("_y:");
    string::size_type posx = val.find("_x:");

    if(type == dccl_string)
    {
        // can't enter string longer than max_length
        if(val.size() == (unsigned int)mp->max_length())
            return false;
    }
    else if (type == dccl_bool)
    {
        return false;
    }
    else if (type == dccl_float)
    {
        //must be a number or decimal place or exponential
        if(!isdigit(input) && input != '-' && input != '.' && input != 'e')
        {
            check_specials(val, input, row);
                
            return false;
        }
        else
        {
            // - sign at start only
            if(input == '-' && (((val.size() != 0) || mp->min() >= 0) && posy == string::npos && posx == string::npos))
                return false;
                
            if(input == '.')
            {
                for (unsigned int j = 0; j < val.size(); j++)
                {
                    if(val[j]=='.')
                        return false;
                }
            }
                
            // check precisions
            string snum = val + (char)input;
                
            double dnum = atof(snum.c_str());

                
            if(mp->precision() >= 0)
            {
                double a = goby::util::unbiased_round(fabs(dnum)*pow(10.0, (double)mp->precision()),0);
                double b = fabs(dnum)*pow(10.0, (double)mp->precision());
                if(as<string>(a) != as<string>(b))
                {
 
                    return false;
                }
                    
            }
                
            // check max, min
            if((dnum >0 && dnum > mp->max()) || (dnum < 0 && dnum < mp->min()))
                return false;
        }            
    }
    else if (type == dccl_int)
    {
        if(!isdigit(input) && input != '-')
        {
            check_specials(val, input, row);
            return false;
        }
            
        else
        {
            // - sign at start only
            if(input == '-' && (((val.size() != 0) || mp->min() >= 0) && posy == string::npos && posx == string::npos))
                return false;

            string snum = val + (char)input;
            double dnum = atof(snum.c_str());
            // check max, min
            if((dnum >0 && dnum > mp->max()) || (dnum < 0 && dnum < mp->min()))
                return false;

        }
    }
    else if (type == dccl_enum)
    {
        return false;
    }
    else if (type == dccl_static)
    {
        return false;    
    }

    return true;
}

bool CommandGui::edit_backspace(int row, std::string & val, DCCLType type)
{
    string::size_type posy = val.find("_y:");
    string::size_type posx = val.find("_x:");

    if(type == dccl_static)
        return false;
    else if(type == dccl_bool || type == dccl_enum)
    {
        val = "";
        return false;
    }
    else if(type == dccl_int || type == dccl_float)
    {
        if(val == "_time")
        {
            val = "";
            return false;
        }
            
        if(posy != string::npos || posx != string::npos)
        {
            if (val.at(val.length()-1) == ':')
            {
                int lat_row, lon_row;
                find_lat_lon_rows(row, lat_row, lon_row);
                if(!xy_only_)
                    val = "";
                if(lat_row && lon_row && !xy_only_)
                {
                    gui_.set_matrix_val(lon_row, 1, "");
                    gui_.set_matrix_val(lat_row, 1, "");
                }                        
                return false;
            }
                
        }
    }
    return true;
}

bool CommandGui::edit_leftright(int row, std::string & val, chtype input, DCCLType type, boost::shared_ptr<DCCLMessageVar> mp)
{
    string::size_type posy = val.find("_y:");
    string::size_type posx = val.find("_x:");

    if (type == dccl_bool || type == dccl_enum)
    {                
        string value;
            
        if(type == dccl_bool)
        {
            value = (val== "true") ? "false" : "true";
        }
        else
        {
            size_t curr_enum_pos = 0;
            vector<string>* enums = mp->enums();
            for (vector<string>::size_type j = 0, m = enums->size(); j < m; ++j)
                curr_enum_pos = (enums->at(j) == val) ? j : curr_enum_pos;

            if (input == KEY_LEFT)
            {
                curr_enum_pos = (curr_enum_pos == 0) ? enums->size()-1 : curr_enum_pos-1;
            }
            else
            {
                curr_enum_pos = (curr_enum_pos == enums->size()-1) ? 0 : curr_enum_pos+1;
            }
            value = enums->at(curr_enum_pos);
                
        }

        val = value;
    }

    if (type == dccl_float || type == dccl_int)
    {
        string snum;
            
        if(posy != string::npos || posx != string::npos)
        {
            string::size_type col_pos = val.find(':');
            snum = val.substr(col_pos+1);
        }
        else
        {
            snum = val;
        }

        double dnum = atof(snum.c_str());

        if (input == KEY_LEFT)
        {
            dnum = dnum-1;
        }
        else
        {
            dnum = dnum+1;
        }
            
        if(posy == string::npos && posx == string::npos)
        {    
            if(dnum > mp->max())
                dnum = mp->max();
            if(dnum < mp->min())
                dnum = mp->min();
        }
            
            
        if(posy != string::npos)
            val = "_y:" + as<string>(dnum);
        else if(posx != string::npos)
            val = "_x:" + as<string>(dnum);
        else
            val = as<string>(dnum);
    }

    return false;

}


bool CommandGui::edit_postcallback(int row, int col, string & val, chtype & input)
{
    if(input == KEY_RESIZE)
    {
        vector<string> a;
        a.push_back("resize");
        gui_.disp_lower_info(a);

        return false;
    }
    vector< boost::shared_ptr<DCCLMessageVar> > message_vars =
        fetch_message_vars(curr_message_);

    boost::shared_ptr<DCCLMessageVar> mp = message_vars.at(row-1);

    vector<string> mesg;
    
    string line = "Editing message variable " + as<string>(row) + " of " + as<string>(message_vars.size()) + ": " + mp->name();
    mesg.push_back(line);
    
    DCCLType type = mp->type();
    
    if(type == dccl_string)
        line = "(</24>string<!24>) max length: " + as<string>(mp->max_length()) + " chars.";
    else if (type == dccl_bool)
        line = "(</32>bool<!32>) specify true/false with right/left arrows";
    else if (type == dccl_float)
        line = "(</48>float<!48>) min: " + as<string>(mp->min()) + ", max: " + as<string>(mp->max()) + ", precision: " + as<string>(mp->precision());
    else if (type == dccl_int)
        line = "(</56>int<!56>) min: " + as<string>(mp->min()) + ", max: " + as<string>(mp->max());    
    else if (type == dccl_enum)
        line = "(</40>enum<!40>) specify values with right/left arrows";
    else if (type == dccl_static)
        line = "(static) you cannot change the value of this field";

    if(type == dccl_string || type == dccl_float || type == dccl_int)
        gui_.cursor_on();
    
    mesg.push_back(line);
    
    gui_.disp_info(mesg);

    return true;
}


void CommandGui::prepare_message(map<string, string> & out_vals, vector<string> & out_of_range, bool do_substitutes, std::vector<std::string> * curr_vals, DCCLMessage * curr_message)
{
    vector<string> copy_vals;
    
    if(curr_vals == NULL)
        copy_vals = (*curr_mess_vals_);
    else
        copy_vals = (*curr_vals);
    
    vector< boost::shared_ptr<DCCLMessageVar> > message_vars = (curr_message)
        ? fetch_message_vars(curr_message)
        : fetch_message_vars(curr_message_);
    
    if(do_substitutes)
    {
        string long_string;
        
        for(vector<string>::size_type i = 0, n = copy_vals.size(); i < n; ++i)
            long_string += copy_vals.at(i) + ",";
        
        insert_specials(long_string);
        
        boost::split(copy_vals, long_string, boost::is_any_of(","));
    }    
    
    for (vector< boost::shared_ptr<DCCLMessageVar> >::size_type i = 0, n = message_vars.size(); i < n; ++i)
    {
        boost::shared_ptr<DCCLMessageVar> mp = message_vars.at(i);
        map<string, string>::iterator it = out_vals.find(mp->source_var());
        if(it != out_vals.end())
            out_vals[mp->source_var()] += ",";
        
        string curr_val = copy_vals.at(i);
        
        if(do_substitutes)
        {
            if(!field_check(curr_val, mp))
                out_of_range.push_back(mp->name());    
        }
        
        
        out_vals[mp->source_var()] += mp->name() + "=" + curr_val;
    }    
    
}

void CommandGui::preview()
{
    string mesg = "</B/40>Previewing message:<!40>\n";

    assemble_message_display(mesg);
    
    vector<string> buttons;
    buttons.push_back("Return to message");
    buttons.push_back("Send");

    int selection;
    gui_.disp_scroll(mesg, buttons, selection, true);
    if (selection == 1)
        state_ = s_send;
    else
        state_ = s_editing;
}

void CommandGui::send()
{
    string mesg = "</B/40>Message(s) sent at time:<!40> " + microsec_simple_time_of_day() + "\n";
    if(do_save("iCommander_autosave.txt"))
        mesg += "</B>Autosaved to iCommander_autosave.txt\n";

    assemble_message_display(mesg);

    vector<string> buttons;
    buttons.push_back("Return to message");
    buttons.push_back("Main menu");

    int selection;
    gui_.disp_scroll(mesg, buttons, selection, true);
    if (selection == 0)
        state_ = s_editing;
    else
        state_ = s_menu;    
}


void CommandGui::assemble_message_display(std::string & mesg)
{
    map<string, string> vals;
    vector<string> out_of_range;
    prepare_message(vals, out_of_range);    

    for(map<string, string>::const_iterator it = vals.begin(), n = vals.end(); it != n; ++it)
    {
        const string & variable = it->first;
        const string & contents = it->second;
        
        m_Comms.Notify(variable, contents);
        mesg += "</56>" + variable + "<!56>" + ": " + contents + "\n";
    }

    if(!out_of_range.empty())
    {
        
        mesg += "\n</16>(Warning)<!16> These message vars are out of range (not specified or NaN):\n";

        for(vector<string>::const_iterator it = out_of_range.begin(), n = out_of_range.end(); it != n; ++it)
        {
            const string & variable = *it;
            if(it != out_of_range.begin())
                mesg += ", ";
            mesg += variable;
        }
        mesg += "\n";
    }
    
}

void CommandGui::insert_specials(string & s)
{
    string to_find;
    string::size_type pos;

    to_find = "_time";
    pos = s.find(to_find);
    while(pos != string::npos)
    {
        s = s.substr(0, pos) + as<string>(MOOSTime()) + s.substr(pos+to_find.length());
        pos = s.find(to_find);
    }

    string::size_type
        y_start_pos,
        x_start_pos,
        y_comma_pos,
        x_comma_pos;

    y_start_pos = s.find("_y:");
    x_start_pos = s.find("_x:");
    
    while(y_start_pos!=string::npos && x_start_pos!=string::npos)
    {
        y_comma_pos = s.find(",", y_start_pos);
        x_comma_pos = s.find(",", x_start_pos);
        
        string yval = s.substr(y_start_pos + 3, y_comma_pos-y_start_pos);
        string xval = s.substr(x_start_pos + 3, x_comma_pos-x_start_pos);

        double dyval = atof(yval.c_str());
        double dxval = atof(xval.c_str());

        double lat, lon;

        geodesy_.UTM2LatLong(dxval, dyval, lat, lon);

        
        // order matters
        if(y_start_pos < x_start_pos)
        {
            s.replace(x_start_pos, x_comma_pos-x_start_pos, as<string>(lon));
            s.replace(y_start_pos, y_comma_pos-y_start_pos, as<string>(lat));
        }
        else
        {
            s.replace(y_start_pos, y_comma_pos-y_start_pos, as<string>(lat));
            s.replace(x_start_pos, x_comma_pos-x_start_pos, as<string>(lon));
        }
        
        y_start_pos = s.find("_y:");
        x_start_pos = s.find("_x:");
    }    
}

int CommandGui::insert_modem_id()
{
    string title = "Choose vehicle to insert id for:";
    vector<string> buttons;

    for(int i = 0,  n = modem_lookup_.max_id(); i <= n; ++i)
    {
        buttons.push_back(
            string(
                as<string>(i) + ": " +
                modem_lookup_.get_name_from_id(i) +
                " (" + modem_lookup_.get_type_from_id(i) + ")"
                )
            );
    }
    
    int selection;
    gui_.disp_scroll(title, buttons, selection);
    return selection;
}

// here we decide how to handle any oversized fields
// we make the following choices:
// truncate strings
// fix float / integer to NaN if not specified or exceed
// static / bools / enums cannot be a problem
bool CommandGui::field_check(string & s, boost::shared_ptr<DCCLMessageVar> mp)
{
    bool ok = s.empty() ? false : true;
    
    DCCLType type = mp->type();
    
    if (type == dccl_float || type == dccl_int)
    {
        // check precisions
        string snum = s;
        double dnum = atof(snum.c_str());

        if (type == dccl_float)
            dnum = goby::util::unbiased_round(dnum, mp->precision());
        
        // check max, min
        if(dnum > mp->max() || dnum < mp->min() || s.empty())
        {
            dnum = goby::acomms::NaN;
            ok = false;
        }        
            
        s = as<string>(dnum);
    }
    else if (type == dccl_string && ((int)s.length() > mp->max_length()))
    {
        ok = false;
        s = s.substr(0,mp->max_length());
    }

    return ok;
}

void CommandGui::quick_switch()
{
    string title = "Choose an active message to switch to:";
    vector<string> buttons;

    vector<int> mapping;

    for(std::map<int, std::vector<std::string> >::iterator it = open_mess_vals_.begin(), n = open_mess_vals_.end(); it != n; ++it)
    {
        buttons.push_back(
            dccl_.messages().at(it->first).name()
            );

        mapping.push_back(it->first);
    }

    int selected;
    gui_.disp_scroll(title, buttons, selected);

    curr_mess_vals_ = &open_mess_vals_[mapping.at(selected)];
    curr_message_ = &dccl_.messages()[mapping.at(selected)];
}

void CommandGui::check_specials(string & val, chtype input, int row)
{
    if(input == 't')
    {
        val = "_time";
    }
    else if(input == 'y' || input == 'x')
    {
        int lat_row;
        int lon_row; 
        find_lat_lon_rows(row, lat_row, lon_row);
        
        if(lat_row && lon_row)
        {
            if(lat_row == row)
            {
                val = "_y:";
                gui_.set_matrix_val(lon_row, 1, "_x:");
            }
            else if(lon_row == row)
            {
                val = "_x:";
                gui_.set_matrix_val(lat_row, 1, "_y:");
            }    
        }
        
    }
    else if(input == 'm')
    {
        int id = insert_modem_id();
        val = as<string>(id);
    }
}

void CommandGui::find_lat_lon_rows(int row, int & lat_row, int & lon_row)
{
    lat_row = 0;
    lon_row = 0;

    int row_search = row;
    int row_other = 0;
    // rows start at 1
    int max_row = rowtitle_.size()+1;
    string current_row = boost::to_lower_copy(rowtitle_[row-1]);
        
    string::size_type lat_pos = current_row.find("lat");
    string::size_type lon_pos = current_row.find("lon");

    if(lon_pos != string::npos)
    {
        lon_row = row;
        int n = 1;
        bool end_hit = false;
        while(!end_hit)
        {
            string::size_type lat_pos_plus = string::npos, lat_pos_minus = string::npos;
            
            if((row_search + n) < max_row)
            {
                lat_pos_plus = boost::to_lower_copy(rowtitle_[row_search+1-1]).find("lat");
                end_hit = true;
            }
            
            if((row_search - n) > 0)
            {
                lat_pos_minus = boost::to_lower_copy(rowtitle_[row_search-1-1]).find("lat");
                end_hit = true;
            }

            if(lat_pos_plus == string::npos && lat_pos_minus == string::npos)
            {
                // cannot find the row
                return;
            }
            else if(lat_pos_plus != string::npos && lat_pos_minus == string::npos)
            {
                //plus found it
                row_other = row_search+1;
                break;
            }
            else if(lat_pos_plus == string::npos && lat_pos_minus != string::npos)
            {
                //minus found it
                row_other = row_search-1;
                break;
            }
            else if(lat_pos_plus != string::npos && lat_pos_minus != string::npos)
            {
                // both found it, just default to plus
                row_other = row_search+1;
                break;
            }

            
            ++n;
        }

        lat_row = row_other;        
    }
    else if(lat_pos != string::npos)
    {
        lat_row = row; 
        int n = 1;
        bool end_hit = false;
        while(!end_hit)
        {
            string::size_type lon_pos_plus = string::npos, lon_pos_minus = string::npos;
            if((row_search + n) < max_row)
            {
                lon_pos_plus = boost::to_lower_copy(rowtitle_[row_search+1-1]).find("lon");
                end_hit = true;
            }
                
            if((row_search - n) > 0)
            {
                lon_pos_minus = boost::to_lower_copy(rowtitle_[row_search-1-1]).find("lon");
                end_hit = true;
            }

            if(lon_pos_plus == string::npos && lon_pos_minus == string::npos)
            {
                // cannot find the row
                return;
            }
            else if(lon_pos_plus != string::npos && lon_pos_minus == string::npos)
            {
                //plus found it
                row_other = row_search+1;
                break;
            }
            else if(lon_pos_plus == string::npos && lon_pos_minus != string::npos)
            {
                //minus found it
                row_other = row_search-1;
                break;
            }
            else if(lon_pos_plus != string::npos && lon_pos_minus != string::npos)
            {
                //both found it, default to minus
                row_other = row_search-1;
                break;
            }
                
            ++n;
        }
        lon_row = row_other; 
    }
}


vector< boost::shared_ptr<DCCLMessageVar> > CommandGui::fetch_message_vars(DCCLMessage* msg)
{
    vector< boost::shared_ptr<DCCLMessageVar> > message_vars;

    // set src_id and dest_id, all the rest should be automatic
    if(msg->header()[goby::acomms::HEAD_SRC_ID]->name() != goby::acomms::DCCL_HEADER_NAMES[goby::acomms::HEAD_SRC_ID])
        message_vars.push_back(msg->header()[goby::acomms::HEAD_SRC_ID]);

    if(msg->header()[goby::acomms::HEAD_DEST_ID]->name() != goby::acomms::DCCL_HEADER_NAMES[goby::acomms::HEAD_DEST_ID])
        message_vars.push_back(msg->header()[goby::acomms::HEAD_DEST_ID]);
    
    message_vars.insert(message_vars.end(),
                        msg->layout().begin(),
                        msg->layout().end());

    return message_vars;
}
