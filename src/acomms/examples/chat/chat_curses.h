// copyright 2009 t. schneider tes@mit.edu
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

#include <ncurses.h>

const float UPPER_WIN_FRAC = 0.75;
const float LOWER_WIN_FRAC = 1-UPPER_WIN_FRAC;
const unsigned MAX_LINE = 26; // fixed in chat.xml

/// provides a terminal GUI for a chat window (lower box to type and upper box to receive messages). Part of the chat.cpp example.
class ChatCurses
{
  public:
        /// \name Constructors/Destructor
        //@{
    ChatCurses() {}
    ~ChatCurses()
    { cleanup(); }
    //@}

    /// give the modem_id so we know how to label our messages
    void set_modem_id(unsigned id) { id_ = id; }    

    /// start the display
    void startup();
    /// grab a character and if there's a line to return it will be returned in \c line
    void run_input(std::string& line);

    /// end the display
    void cleanup();

    /// add a message to the upper window (the chat log)
    void post_message(unsigned id, const std::string& line);

  private:
    void update_size();


    
  private:
    int xmax_;
    int ymax_;

    unsigned id_;
    
    WINDOW* upper_win_;
    WINDOW* lower_win_;
    WINDOW* divider_win_;

    std::string line_buffer_;
};


