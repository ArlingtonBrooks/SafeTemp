/****************************************************************
Window Manager Library
	Defines a window manager class and some basic functions
	    for controlling it.
****************************************************************/

#include <stdio.h>
#include <ncurses.h>
#include <vector>
#include <unistd.h>
#include "WinMan.h"

/****************************************************************
Line structure
	Defines start and end points of a line.
****************************************************************/
struct Line
{
    int x1;
    int y1;
    int x2;
    int y2;
};

/****************************************************************
Rect structure
	Defines the position and dimensions of a rectangle.
****************************************************************/
struct Rect
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
