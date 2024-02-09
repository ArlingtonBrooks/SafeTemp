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
Hybrid Window Library
	Contains the HybridWindow class which holds the
	    sub-windows managed by the Window Manager library.

	Hybrid Windows are defined as windows which have stored
	    text buffer, Foreground Colour buffer, Background
	    colour buffer, and Mode (display character type)
	    buffer.

	IMPORTANT: Since each location on the terminal is
	    processed as though it is a pixel, you have some 
	    cutoffs: 
	     -The BOTTOM has a layer of 'pixels' cut off due to
	      the fact that they lie right on the border
		(they exist as integers, but cannot be drawn)
	     -If a border exists, there is an additional layer of
	      pixels that get cut off along the border.  Overall,
	      you may lose up to 3 lines at the bottom due to this
	      fact (plan accordingly).
****************************************************************/
#include <stdio.h>
#include <ncurses.h>
#include <string>
#include <vector>
#include <unordered_map>
#ifndef WINMAN_H_
#define WINMAN_H_
#include <memory>
#include "../Types.hpp"

#ifndef UI_HYBRID_WIN_H_
#define UI_HYBRID_WIN_H_

/**
 * @brief A generic ncurses window
 */
class ncursesWindow {
protected:
	std::shared_ptr<WINDOW> m_Win;
	void set_handle(std::shared_ptr<WINDOW> Win) { m_Win = Win; }
public:
	std::shared_ptr<WINDOW> GetHandle() {return m_Win;}
	WinSize GetSize() {
		WinSize Size;
		getmaxyx(m_Win.get(),Size.y,Size.x);
		return Size;
	}
	virtual void Redraw() {
		touchwin(m_Win.get());
		Refresh();
	}
	virtual void Refresh() {
		wrefresh(m_Win.get());
	}
	virtual void Draw() = 0;
	void PrintString(int y, int x, std::string const &Message, bool box = false) {
		mvwprintw(m_Win.get(),y,x,Message.c_str());
	}
};

/**
 * @brief A subwindow to be contained within a MainWindow class
 * @note The user interface will be structured as:
 * <current temp> | <sensor name> | <critical temperature> | <Command>|
 */
class SubWindow : public ncursesWindow {
protected:
public:
	SubWindow(int h, int w, int y, int x) {
		//Set window handle; delete is handled by destructor;
		set_handle(std::shared_ptr<WINDOW>(newwin(h,w,y,x),[](WINDOW* win){wclear(win); delwin(win);}));
		box(m_Win.get(), 0, 0);
	}
	SubWindow(Rect<int> Size) {
		set_handle(std::shared_ptr<WINDOW>(newwin(Size.h,Size.w,Size.y,Size.x),[](WINDOW* win){wclear(win); delwin(win);}));
		box(m_Win.get(), 0, 0);
	}
	void Resize(Rect<int> NewSize) {
		wresize(m_Win.get(),NewSize.h, NewSize.w);
		mvwin(m_Win.get(),NewSize.y,NewSize.x);
		Refresh();
	}
	virtual void Draw() override {
		box(m_Win.get(), 0, 0);
	}
};

void PrintHeaders(SubWindow &Win) {
	const char* CurrentTemp = "Curr. T";
	const char* CriticalTemp = "Crit. T";
	const char* SName = "Sensor Name";
	const char* SymbolTxt = "Symbol";
	const char* ColTxt = "Colour";
	const char* Cmd = "Command";
	mvwprintw(Win.GetHandle().get(),1,1,"%-8s | ",CurrentTemp);
	wprintw(Win.GetHandle().get(),"%-16s  | ",SName);
	wprintw(Win.GetHandle().get(),"%-8s | ",CriticalTemp);
	wprintw(Win.GetHandle().get(),"%-6s | ",SymbolTxt);
	wprintw(Win.GetHandle().get(),"%-6s | ",ColTxt);
	wprintw(Win.GetHandle().get(),Cmd);
}

void CheckSetAttribute(SubWindow &Win, Selection const &Cursor, int i, int j) {
	if (Cursor.Row == i && Cursor.Col == j)
		wattron(Win.GetHandle().get(),A_STANDOUT);
	else
		wattroff(Win.GetHandle().get(),A_STANDOUT);
}

void NCursesPrintUiToWindow(SubWindow &Win, Selection Cursor, std::size_t ScrollPoint, std::vector<SensorDetailLine> const &Opts) {
	wmove(Win.GetHandle().get(),1,1);
	unsigned nSensors = Opts.size();
	WinSize WSize = Win.GetSize();
	ScrollPoint = std::min(ScrollPoint,Opts.size()-WSize.y - 5);
	PrintHeaders(Win);
	unsigned MinSensors = WSize.y - 5;
	MinSensors = (MinSensors > Opts.size()) ? Opts.size() : MinSensors;
	for (int i = 0; i != MinSensors; i++) {//auto const &i : Opts) 
		unsigned SensorNumber = i + ScrollPoint;
		CheckSetAttribute(Win,Cursor,i,0);
		mvwprintw(Win.GetHandle().get(), i+3, 1, "%8.2f",Opts[SensorNumber].TempData.Temp);
		wattroff(Win.GetHandle().get(),A_STANDOUT);
		wprintw(Win.GetHandle().get(),"  | ");
		CheckSetAttribute(Win,Cursor,i,1);
		wprintw(Win.GetHandle().get(),"%-16s",Opts[SensorNumber].FriendlyName.substr(0,16).c_str());
		wattroff(Win.GetHandle().get(),A_STANDOUT);
		wprintw(Win.GetHandle().get()," | ");
		CheckSetAttribute(Win,Cursor,i,2);
		wprintw(Win.GetHandle().get(),"%8.2f",Opts[SensorNumber].CritTemp);
		wattroff(Win.GetHandle().get(),A_STANDOUT);
		wprintw(Win.GetHandle().get()," | ");
		CheckSetAttribute(Win,Cursor,i,3);
		wprintw(Win.GetHandle().get(),"%6c",Opts[SensorNumber].Symbol);
		wattroff(Win.GetHandle().get(),A_STANDOUT);
		wprintw(Win.GetHandle().get()," | ");
		CheckSetAttribute(Win,Cursor,i,4);
		wprintw(Win.GetHandle().get(),"%6d",(int)Opts[SensorNumber].Colour);
		wattroff(Win.GetHandle().get(),A_STANDOUT);
		wprintw(Win.GetHandle().get()," | ");
		CheckSetAttribute(Win,Cursor,i,5);
		wprintw(Win.GetHandle().get()," %s ",Opts[SensorNumber].Command.substr(0,WSize.x - 60).c_str());
		if (60 + Opts[SensorNumber].Command.length() > WSize.x - 8)
			wprintw(Win.GetHandle().get(),"...");
		wattroff(Win.GetHandle().get(),A_STANDOUT);
	}
	if (Opts.size() > MinSensors) {
		mvwprintw(Win.GetHandle().get(), WSize.y-2, 1, "%-8s | ","(...)");
		wprintw(Win.GetHandle().get(), "%-16s  | ","  (...)  ");
		wprintw(Win.GetHandle().get(), "%-8s | "," (...) ");
		wprintw(Win.GetHandle().get(), "%-6s | ","(...)");
		wprintw(Win.GetHandle().get(), "%-6s | ","(...)");
		wprintw(Win.GetHandle().get(), "%s "," (...) ");
	}
	if (ScrollPoint > 0) {
		mvwprintw(Win.GetHandle().get(), 2, 1, "%-8s | ","(...)");
		wprintw(Win.GetHandle().get(), "%-16s  | ","  (...)  ");
		wprintw(Win.GetHandle().get(), "%-8s | "," (...) ");
		wprintw(Win.GetHandle().get(), "%-6s | ","(...)");
		wprintw(Win.GetHandle().get(), "%-6s | ","(...)");
		wprintw(Win.GetHandle().get(), "%s "," (...) ");
	}
}

void NCursesPrintGraphAxes(SubWindow &Win/*, float MaxTemp, float MinTemp, std::time_t MaxTime, std::time_t MinTime*/) {
	WinSize WSize = Win.GetSize();
	const char TempText[] = "Temperature";
	const char TimeText[] = "Time";
	int loc_y = WSize.y / 2 - sizeof(TempText) / 2;
	int loc_x = WSize.x / 2 - sizeof(TimeText) / 2;
	wattron(Win.GetHandle().get(),A_BOLD);
	for (auto const &c : TempText) {
		mvwprintw(Win.GetHandle().get(),loc_y++,2,"%c",c);
	}
	mvwprintw(Win.GetHandle().get(),WSize.y-2,loc_x,"%s",TimeText);
	wmove(Win.GetHandle().get(),WSize.y-4,6);
	whline(Win.GetHandle().get(),0,WSize.x - 8);
	wmove(Win.GetHandle().get(),2,12);
	wvline(Win.GetHandle().get(),0,WSize.y-4);
	wattroff(Win.GetHandle().get(),A_BOLD);
}

void NCursesPrintGraphToWindow(SubWindow &Win, std::vector<SensorDetailLine> const &Opts, unsigned dtime = 0) {
	//todo
}

/**
 * @brief The global main window
 */
class MainWindow : public ncursesWindow {
protected:
public:
	std::unordered_map<std::string,SubWindow> Windows;
	MainWindow() {
		//Set window handle; delete is handled by destructor;
		set_handle(std::shared_ptr<WINDOW>(initscr(),[](WINDOW* win){endwin();}));
		cbreak();
		noecho();
		keypad(GetHandle().get(),true);
		curs_set(0);
		timeout(128);
	}
	~MainWindow() {
		Windows.clear();
	}
	void CreateSubWindow(std::string const &WName, int h, int w, int y, int x) {
		SubWindow WSub(h,w,y,x);
		Windows.emplace(WName,WSub);
	}
	void CreateSubWindow(std::string const &WName, Rect<int> Size) {
		SubWindow WSub(Size);
		Windows.emplace(WName,WSub);
	}
	virtual void Draw() override {
		for (auto &i : Windows) i.second.Draw();
	}
	virtual void Refresh() override {
		wrefresh(m_Win.get());
		refresh();
	}
	SubWindow &GetSubWindow(std::string const &WName) {
		return Windows.at(WName);
	}
	void RefreshAll() {
		Refresh();
		for (auto &i : Windows) i.second.Refresh();
	}
	void RedrawAll() {
		wclear(m_Win.get());
		for (auto &i : Windows) wclear(i.second.GetHandle().get());
		Redraw();
		for (auto &i : Windows) i.second.Redraw();
	}
};
#endif //WINMAN_H_

/****************************************************************
Alignment Structure
	Defines types of alignment for text
****************************************************************/
enum Alignment
{
    ALIGN_TOP_CENTER = 1,
    ALIGN_TOP_LEFT,
    ALIGN_TOP_RIGHT,
    ALIGN_BOTTOM_CENTER,
    ALIGN_BOTTOM_LEFT,
    ALIGN_BOTTOM_RIGHT,

    ALIGN__RIGHT,
    ALIGN__LEFT
};

class HybridWindow
{
    int x,y,w,h;
    bool border;
    chtype vBound, hBound;
    std::string TextBuffer;
    std::vector<unsigned int> FGCOL {1,1,1,1,1,1,1};
    std::vector<unsigned int> BGCOL {1,1,1,1,1,1,1};
    std::vector<unsigned int> Mode {1,1,1,1,1,1};
    unsigned int resetCol = 0;
    bool Colours;
    WINDOW* MainWin;
    public:
    WINDOW *win;

    HybridWindow(WINDOW*&, int, int, int, int, unsigned int, bool, chtype, chtype);
    HybridWindow();
    void MoveWin(int, int, int, int);
    void draw(unsigned int*);
    void DrawPixel(int,int,char,unsigned int,unsigned int,unsigned int);
    void ClearBuffer();
    void RebuildWindow(WINDOW*&);
    void UpdateWindow(WINDOW*&);
    int GetX();
    int GetY();
    int GetWidth();
    int GetHeight();
    void DrawText(unsigned int, unsigned int, std::string, int, int, int);
    void DrawAlignedText(Alignment, std::string, int, int, int);
};

/****************************************************************
Constructor:
	Takes:
	    -Main Window (the window which manages this sub-win
	    -X-location on the main window
	    -Y-location on the main window
	    -Width of sub-window
	    -Height of sub-window
	    -Optional reset colour for window
	    -Window border (off or on)
	    -Characters to use as window borders
	Returns:
	    -New sub-window (win) is formed, buffers are
	     initialized, and (if applicable) colours are 
	     enabled.
****************************************************************/
HybridWindow::HybridWindow(WINDOW*& Main, int X, int Y, int W, int H, unsigned int ResetColour = 0, bool BORDER = 0, chtype vBorder = 0, chtype hBorder = 0)
{
    resetCol = ResetColour;
    if (resetCol > 7 || resetCol < 0) resetCol = 0;
    x = X;
    y = Y;
    w = W;
    h = H;
    MainWin = Main;

    if (x < 0) x = 0;
    if (x+w > MainWin->_maxx) x = MainWin->_maxx - w;
    if (y < 0) y = 0;
    if (y+h > MainWin->_maxy) y = MainWin->_maxy - h;
    if (x+w > MainWin->_maxx)
    {
	x = 0;
	w = MainWin->_maxx;
    }
    if (y+h > MainWin->_maxy)
    {
	y = 0;
	h = MainWin->_maxy;
    }

    border = BORDER;
    vBound = vBorder;
    hBound = hBorder;
    win = subwin(Main,h,w,y,x);
    if (border) box(win,vBorder,hBorder);
    ClearBuffer();
    Colours = has_colors();
    if (Colours) start_color();
};

/****************************************************************
MoveWin
	Takes:
	    -New X-position for sub-window
	    -New Y-position for sub-window
	    -New width of sub-window
	    -New height of sub-window
	Returns:
	    -Sub-window is moved to new X and Y coordinates, the
	     old window is deleted from the screen.
****************************************************************/
void HybridWindow::MoveWin(int NewX, int NewY, int NewWidth = -1, int NewHeight = -1)
{
    wborder(win,' ',' ',' ',' ',' ',' ',' ',' ');
    wmove(win,0,0);
    std::string tmp = "";
    for (int i = 0; i < w*h; i++)
    {
	tmp += " ";
    }
    wprintw(win,tmp.c_str());
    wnoutrefresh(win);
    delwin(win);
    x = NewX;
    y = NewY;
    if (NewWidth > 0) w = NewWidth;
    if (NewHeight > 0) h = NewHeight;

    if (x < 0) x = 0;
    if (x+w > MainWin->_maxx) x = MainWin->_maxx - w;
    if (y < 0) y = 0;
    if (y+h > MainWin->_maxy) y = MainWin->_maxy - h;
    if (x+w > MainWin->_maxx)
    {
	x = 0;
	w = MainWin->_maxx;
    }
    if (y+h > MainWin->_maxy)
    {
	y = 0;
	h = MainWin->_maxy;
    }

    win = subwin(MainWin,h,w,y,x);
    if (border) box(win,vBound,hBound);
};

/****************************************************************
draw
	Takes:
	    -mPointer to a Colour Pairs variable (in Manager)
	Returns:
	    -Draws window to screen as well as all information in
	     the text, FGCOL, BGCOL, and MODE buffers.
	Screen will only update where pixels have changed.
****************************************************************/
void HybridWindow::draw(unsigned int *ColorPairs)
{
    if (border) wprintw(win," ");
    if (Colours)
    {
	short int tmp1,tmp2,ColPair;
	bool TestPair = 0;
	init_pair(1,7,resetCol);
	for (int i = 0; i < TextBuffer.length(); i++)
	{
	    TestPair = 0;
	    for (int j = 0; j <= *ColorPairs; j++)
	    {
		pair_content(j,&tmp1,&tmp2);
		if (tmp1 == FGCOL[i] && tmp2 == BGCOL[i])
		{
		    TestPair = 1;
		    ColPair = j;
		    break;
		}
	    }
	    if (!TestPair)
	    {
		init_pair(*ColorPairs+1,FGCOL[i],BGCOL[i]);
		*ColorPairs += 1;
		ColPair = *ColorPairs;
	    }
	    if (border && i != 0 && i != TextBuffer.length()-1 && (i)%(w-2*border) == 0) wprintw(win,"  ");
//            if (border && (i-1)%(w) == 0) wprintw(win," ");
	    wattron(win,Mode[i]);
	    wattron(win,COLOR_PAIR(ColPair));
	    wprintw(win,"%c",TextBuffer[i]);
	    wattroff(win,COLOR_PAIR(ColPair));
	    wattroff(win,Mode[i]);
	}
	if (border) box(win,vBound,hBound);
//        wrefresh(win);
	wnoutrefresh(win);
    }
    else
    {
	for (int i = 0; i < TextBuffer.length(); i++)
	{
	    wattron(win,Mode[i]);
	    wprintw(win,"%c",TextBuffer[i]);
	    wattroff(win,Mode[i]);
	}
	if (border) box(win,vBound,hBound);
//        wrefresh(win);
	wnoutrefresh(win);
    }
};

/****************************************************************
DrawPixel
	Takes:
	    -X-position of pixel (on sub-window)
	    -Y-position of pixel (on sub-window)
	    -Character to display at pixel
	    -Optional FGCOL
	    -Optional BGCOL
	    -Optional MODE
	Returns:
	    -Display character is placed on the text buffer at 
	     approprate position, FGCOL, BGCOL, and MODE buffers
	     are updated
****************************************************************/
void HybridWindow::DrawPixel(int XP, int YP, char disp, unsigned int FG = -1, unsigned int BG = -1, unsigned int mode = A_NORMAL)
{
//    XP = XP + border;
    YP = YP + border;
    if (XP < w && YP < h && XP >= 0 && YP >= 0)
    {
	TextBuffer[YP*(w-2*border) + XP] = disp;
	//if (FG >= 0) FGCOL[YP*(w-2*border) + XP] = FG;
	//if (BG >= 0) BGCOL[YP*(w-2*border) + XP] = BG;
	//else BGCOL[YP*(w-2*border) + XP] = resetCol;
	Mode[YP*(w-2*border) + XP] = mode;
    }
};

/****************************************************************
ClearBuffer
	Clears the text, FGCOL, BGCOL, and MODE buffers, 
	    restoring them to their default values.
****************************************************************/
void HybridWindow::ClearBuffer()
{
    TextBuffer = "";
    FGCOL.clear();
    BGCOL.clear();
    Mode.clear();
    for (int i = 0; i <= (w-2*border)*(h-border); i++)
    {
	TextBuffer += ' ';
	if (Colours) FGCOL.push_back(7);
	if (Colours) BGCOL.push_back(resetCol);
	Mode.push_back(A_NORMAL);
    }
    wmove(win,0,0);
};

/****************************************************************
RebuildWindow
	Takes:
	    -Reference to the main window (Manager)
	Returns:
	    -Window is deleted and re-built without changes to 
	     the buffers.  This function is mostly useful for 
	     updating the window in case the window size changes.
	CAUTION:
	    Since the buffers are not updated, it is possible
		for EXCESS data to be left in the buffer.  On
		most systems, this should simply truncate the
		output.
****************************************************************/
void HybridWindow::RebuildWindow(WINDOW*& Main)
{
    MainWin = Main;
    delwin(win);

    if (x < 0) x = 0;
    if (x+w > MainWin->_maxx) x = MainWin->_maxx - w;
    if (y < 0) y = 0;
    if (y+h > MainWin->_maxy) y = MainWin->_maxy - h;
    if (x+w > MainWin->_maxx)
    {
	x = 0;
	w = MainWin->_maxx;
    }
    if (y+h > MainWin->_maxy)
    {
	y = 0;
	h = MainWin->_maxy;
    }

    win = subwin(MainWin,h,w,y,x);
};

/****************************************************************
UpdateWindow
	Same as RebuildWindow, however only rebuilds the window
	    if required to fit the window on the Main window.
****************************************************************/
void HybridWindow::UpdateWindow(WINDOW*& Main)
{
    bool rebuild = 0;
    MainWin = Main;

    if (x < 0 || y < 0 || x+w > MainWin->_maxx || y+h > MainWin->_maxy) rebuild = 1;

    if (x < 0) x = 0;
    if (x+w > MainWin->_maxx) x = MainWin->_maxx - w;
    if (y < 0) y = 0;
    if (y+h > MainWin->_maxy) y = MainWin->_maxy - h;
    if (x+w > MainWin->_maxx)
    {
	x = 0;
	w = MainWin->_maxx;
    }
    if (y+h > MainWin->_maxy)
    {
	y = 0;
	h = MainWin->_maxy;
    }
    if (rebuild)
    {
	delwin(win);
	win = subwin(MainWin,h,w,y,x);
    }
};

/****************************************************************
Dimensions Functions
	Returns window dimensions.
****************************************************************/
int HybridWindow::GetX()
{
    return x;
};
int HybridWindow::GetY()
{
    return y;
};
int HybridWindow::GetWidth()
{
    return w;
};
int HybridWindow::GetHeight()
{
    return h;
};

/****************************************************************
DrawText
	Draws text to the current sub-window at positions X and Y
	    relative to the sub-window.  Text truncates if it goes
	    off the edge of the window.
	If the window has a border, there is a 1-pixel space
	    around the window that is covered up by the border.  
****************************************************************/
void HybridWindow::DrawText(unsigned int XP, unsigned int YP, std::string Data, int FG = -1, int BG = -1, int MODE = A_NORMAL)
{
    YP = YP + border;
    for (int i = 0; i < Data.length(); i++)
    {
	if (YP*(w-2*border) + XP + i < TextBuffer.length())
	{
	    TextBuffer[YP*(w-2*border) + XP + i] = (char)Data[i];
	    //if (Colours && FG >= 0) FGCOL[YP*(w-2*border) + XP + i] = FG;
	    //if (Colours && BG >= 0) BGCOL[YP*(w-2*border) + XP + i] = BG;
	    Mode[YP*(w-2*border) + XP + i] = MODE;
	}
    }
};

/****************************************************************
DrawAlignedText
	Draws text which attempts to align itself with the window
	    at a given position
****************************************************************/
void HybridWindow::DrawAlignedText(Alignment TYPE, std::string TEXT, int FG = -1, int BG = -1, int MODE = A_NORMAL)
{
    int Xst, Yst;
    switch (TYPE)
    {
	case (ALIGN_TOP_CENTER):
	{
	    Xst = (w - border)/2 - TEXT.length()/2;
	    Yst = 0;
            DrawText(Xst,Yst,TEXT,FG,BG,MODE);
            break;
        };
        case (ALIGN_TOP_LEFT):
        {
            Xst = 0;
            Yst = 0;
            DrawText(Xst,Yst,TEXT,FG,BG,MODE);
            break;
        };
        case (ALIGN_TOP_RIGHT):
        {
            Xst = w - 2*border - TEXT.length();
            Yst = 0;
            DrawText(Xst,Yst,TEXT,FG,BG,MODE);
            break;
        };
        case (ALIGN_BOTTOM_CENTER):
        {
            Xst = (w - 2*border)/2 - TEXT.length()/2;
            Yst = h - 1 - 2*border;
            DrawText(Xst,Yst,TEXT,FG,BG,MODE);
            break;
        };
        case (ALIGN_BOTTOM_LEFT):
        {
            Xst = 0;
            Yst = h-1-2*border;
            DrawText(Xst,Yst,TEXT,FG,BG,MODE);
            break;
        };
        case (ALIGN_BOTTOM_RIGHT):
        {
            Xst = w - 2*border - TEXT.length();
            Yst = h-1-2*border;
            DrawText(Xst,Yst,TEXT,FG,BG,MODE);
            break;
        };
        case (ALIGN__LEFT):
        {
            Yst = (h-1-2*border)/2 - TEXT.length()/2;
            for (int i = 0; i < TEXT.length(); i++)
            {
                DrawPixel(0,Yst+i,TEXT[i],FG,BG,MODE);
            }
            break;
        };
        case (ALIGN__RIGHT):
        {
            Yst = (h-1-2*border)/2 - TEXT.length()/2;
            for (int i = 0; i < TEXT.length(); i++)
            {
                DrawPixel(w-1-2*border,Yst+i,TEXT[i],FG,BG,MODE);
            }
            break;
        };
        default: break;
    }
};
#endif //UI_HYBRID_WIN_H_
