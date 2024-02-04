/*****************************************************************
Copyright (c) 2015, Kevin Arlington Brooks
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*****************************************************************/

/****************************************************************
Window Manager Library
	Defines a window manager class and some basic functions
	    for controlling it.
****************************************************************/

#include <stdio.h>
#include <ncurses.h>
#include <vector>
#include <unordered_map>
#include <unistd.h>
#include "WinMan.hpp"

#ifndef UI_MANAGER_H_
#define UI_MANAGER_H_

/** @brief An interface for user input handling */
class Input {
public:
	enum class Key {
		Left,
		Right,
		Up,
		Down,
		Select
	};
protected:
	void ChangeSelection(Key K) {
		switch (K) {
		case Key::Left: {
			if (Cursor.Col == 0) { Cursor.Col = NCols - 1; }
			else { Cursor.Col -= 1; }
		}
		case Key::Right: {
			if (Cursor.Col == NCols - 1) { Cursor.Col = 0; }
			else { Cursor.Col += 1; }
		}
		case Key::Up: {
			if (Cursor.Row == 0) { Cursor.Row = NRows - 1; }
			else { Cursor.Row -= 1; }
		}
		case Key::Down: {
			if (Cursor.Row == NRows - 1) { Cursor.Row = 0; }
			else { Cursor.Row += 1; }
		}
		}
	}
	Selection Cursor;
	int NRows = 0;
	int NCols = 0;
	void set_nrows(int nrow) {NRows = nrow;}
	void set_ncols(int ncol) {NCols = ncol;}
public:
	virtual int GetKey() = 0;
	virtual int ProcessKey(int Keyval) = 0;
};

class NCurses_Input : public Input {
public:
	NCurses_Input(int tNRows, int tNCols) {
		set_nrows(tNRows);
		set_ncols(tNCols);
	}
	
	virtual int GetKey() override {
		return getch();
	}
	virtual int ProcessKey(int KeyVal) {
		switch (KeyVal) {
		case KEY_LEFT:  ChangeSelection(Key::Left); break;
		case KEY_RIGHT: ChangeSelection(Key::Right); break;
		case KEY_DOWN:  ChangeSelection(Key::Down); break;
		case KEY_UP:    ChangeSelection(Key::Up); break;
		//case '+':       IncrementValue(); break; //TODO
		//case '-':       DecrementValue(); break; //TODO
		case KEY_ENTER: [[fallthrough]]
		//case '\n':      GetInput(); break; //TODO
		default: return KeyVal;
		}
		return KeyVal;
	}
};

/****************************************************************
mLine structure
	Defines start and end points of a line.
****************************************************************/
struct mLine
{
    int x1;
    int y1;
    int x2;
    int y2;
};

/****************************************************************
mRect structure
	Defines the position and dimensions of a rectangle.
****************************************************************/
struct mRect
{
    int x;
    int y;
    int w;
    int h;
};

/****************************************************************
Manager class
	Manages the drawing, adding, and clearing of windows on
	    the screen.  Also initializes ncurses.

	Due to the limitations in drawing colours on screen, there
	    is a limited number of FGCOL/BGCOL combinations that
	    can be defined (256 to be specific).  

	The manager stores a "main" window, on which all other
	    windows are sub-windows.  
****************************************************************/
class Manager
{
    int x,y;
    public:
    std::vector<HybridWindow> Wins;
    WINDOW *MAIN;
    std::vector<unsigned int> FGCOL;
    std::vector<unsigned int> BGCOL;
    std::vector<unsigned int> MODE;
    unsigned int ColorPairs = 0;
    Manager();
    void DrawWindows();
    int AddWin(int,int,int,int,unsigned int,bool,chtype,chtype);
    void ClearBuffers();
    void ClearBuffer(unsigned int index);
    void RedrawWindow(int i);
};


/****************************************************************
Constructor
	Initializes ncurses and builds the manager screen.
****************************************************************/
Manager::Manager()
{
    initscr();
    //raw();
    //halfdelay(1);
    nodelay(stdscr,true);

    nonl();
    intrflush(stdscr,false);

    MAIN = newwin(getmaxy(stdscr),getmaxx(stdscr),0,0);
    x = getmaxx(stdscr);
    y = getmaxy(stdscr);
    keypad(stdscr,true);
};

/****************************************************************
DrawWindows
	Draws all sub-windows and the main window (if there are
	    any screen updates).  Additionally, if the screen size
	    has changed, updates the window sizes to coincide
	    with the new window size.
****************************************************************/
void Manager::DrawWindows()
{
    wnoutrefresh(MAIN);
    touchwin(MAIN);
    for (int i = 0; i < Wins.size(); i++)
    {
        Wins[i].draw(&ColorPairs);
    }
    if (x != getmaxx(stdscr) || y != getmaxy(stdscr))
    {
        for (int i = 0; i < Wins.size(); i++)
        {
            delwin(Wins[i].win);
        }
        delwin(MAIN);
        newwin(getmaxy(stdscr),getmaxx(stdscr),0,0);
        x = getmaxx(stdscr);
        y = getmaxy(stdscr);
        for (int i = 0; i < Wins.size(); i++)
        {
            Wins[i].RebuildWindow(MAIN);
        }
    }
    //flushinp(); //this was causing input issues
    clrtoeol();
    doupdate();
};

/****************************************************************
AddWin
	Takes:
	    -X position of new window
	    -Y position of new window
	    -Width of new window
	    -Height of new window
	    -Reset colour (optional)
	    -Window Border (either on or off)
	    -Horizontal and vertical border characters
	Returns: the integer location of a new window in the
	    window array.
****************************************************************/
int Manager::AddWin(int X, int Y, int W, int H, unsigned int ResetCol = 0, bool Border = 0, chtype hbord = 0, chtype vbord = 0)
{
    HybridWindow TmpWin(MAIN,X,Y,W,H,ResetCol,Border,hbord,vbord);
    Wins.push_back(TmpWin);
    return Wins.size()-1;
};

/****************************************************************
ClearBuffers
	Clears all window buffers and sets a re-draw flag to 
	    re-draw all windows.

ClearBuffer clears buffer only for window at index 'index'.
****************************************************************/
void Manager::ClearBuffers()
{
    for (int i = 0; i < Wins.size(); i++)
    {
        Wins[i].ClearBuffer();
    }
    ColorPairs = 0;
    touchwin(MAIN);
};

void Manager::ClearBuffer(unsigned int index)
{
    Wins[index].ClearBuffer();
    touchwin(MAIN);
};


/****************************************************************
RedrawWindow
	Force a re-draw of window i.
****************************************************************/
void Manager::RedrawWindow(int i)
{
    touchwin(MAIN);
    Wins[i].draw(&ColorPairs);
}
#endif //UI_MANAGER_H_
