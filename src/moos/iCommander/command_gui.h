#ifndef COMMANDGUI_H
#define COMMANDGUI_H

#include <cctype>
#include <cmath>
#include <fstream>

#include <boost/bind.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/thread/mutex.hpp>

#include "MOOSUtilityLib/MOOSGeodesy.h"
#include "MOOSLIB/MOOSLib.h"

#include "commander_cdk.h"
#include "goby/acomms/dccl.h"
#include "goby/moos/libmoos_util/modem_id_convert.h"

class CommandGui
{
  public:
    CommandGui(CommanderCdk & gui,
               boost::mutex & gui_mutex,
               CMOOSGeodesy & geodesy,
               tes::ModemIdConvert & modem_lookup,
               CMOOSCommClient & comms,
               std::string & schema,
               goby::acomms::DCCLCodec & dccl,
               std::vector<std::string> & loads,
               std::string & community,
               bool xy_only);
    
    ~CommandGui();

    bool run();
    void initialize()
    {
        gui_.initialize();

        for(std::vector<std::string>::size_type i = 0, n = loads_.size(); i < n; ++i)
            do_open(loads_[i]);

        initialized_ = true;
    }    

    std::string microsec_simple_time_of_day()
    { return boost::posix_time::to_simple_string(boost::posix_time::microsec_clock::universal_time().time_of_day()); }

    
  private:
    void do_run();
    
    void main_menu();
    void new_message();
    void open_message();
    void save_message();
    bool do_save(std::string);
    bool do_open(std::string);
    void import_xml();
    void edit();
    int edit_popup();
    void send();
    void insert_specials(std::string & s);
    bool field_check(std::string & s, boost::shared_ptr<goby::acomms::DCCLMessageVar> mp);
    void quick_switch();
    void clear_message() { curr_mess_vals_->clear();}
    void check_specials(std::string & val, chtype input, int row);    
    bool edit_precallback(int, int, std::string &, chtype &);
    bool edit_menu(int, std::string &, goby::acomms::DCCLType);
    bool edit_isprint(int, std::string &, chtype, goby::acomms::DCCLType, boost::shared_ptr<goby::acomms::DCCLMessageVar>);
    bool edit_backspace(int, std::string &, goby::acomms::DCCLType);
    bool edit_leftright(int, std::string &, chtype, goby::acomms::DCCLType, boost::shared_ptr<goby::acomms::DCCLMessageVar>);
    bool edit_postcallback(int, int, std::string &, chtype &);
    void prepare_message(std::map<std::string, std::string> & vals, std::vector<std::string> & out_of_range, bool do_substitutes = true, std::vector<std::string> * curr_vals = NULL, goby::acomms::DCCLMessage * curr_message = NULL);
    void preview();
    int insert_modem_id();
    void find_lat_lon_rows(int row, int & lat_row, int & lon_row);
    void assemble_message_display(std::string & mesg);    
    
  private:
    std::vector< boost::shared_ptr<goby::acomms::DCCLMessageVar> > fetch_message_vars(goby::acomms::DCCLMessage*);

    bool initialized_;

    CommanderCdk & gui_;
    boost::mutex & gui_mutex_;
    CMOOSGeodesy & geodesy_;
    tes::ModemIdConvert & modem_lookup_;
    CMOOSCommClient & m_Comms;
    std::string & schema_;
    goby::acomms::DCCLCodec & dccl_;
    std::vector<std::string> & loads_;
    std::string & community_;
    bool xy_only_;
    
    bool refresh_;
    
    std::vector<std::string> * curr_mess_vals_;    
    bool screen_initialized_;
    // used to specify if inserting the special:xy2latlong
    bool inserting_x_;
    bool inserting_y_;
    
    std::vector<std::string> rowtitle_;
    std::vector<std::string> coltitle_;
    
    std::map<int, std::vector<std::string> > open_mess_vals_;
    int curr_field_;
    goby::acomms::DCCLMessage * curr_message_;

    std::map<goby::acomms::DCCLMessage*, int> current_xy_num_;
    
    enum state {s_new, s_open, s_save, s_import, s_exit, s_return, s_menu, s_editing, s_preview, s_send, s_quick_switch, s_clear};
    state state_;    
};

#endif 
