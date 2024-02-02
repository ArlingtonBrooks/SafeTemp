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
Graph Class
	Handles data pertaining to the drawing of a graph to the
	    screen.  This is mostly a strictly numerical class
	    and depends only on ncurses for the final screen draw.
	    
	User supplies xmin, ymin, xmax, ymax, graph dimensions, 
	    and draw "window".  This class creates an internal 
	    graph buffer which can be later drawn to "window".
****************************************************************/
//#include "WinMan.h"
#include <vector>
#include <math.h>
#include <ncurses.h>

struct mPoint
{
    int x;
    int y;
};

struct fmPoint
{
    float x;
    float y;
};

class Graph
{
    int xmin, ymin, xmax, ymax;
    int xsp, ysp;
    mPoint Origin;
    std::vector<float> xlabels;
    std::vector<float> ylabels;
    public:
    HybridWindow *win;
    mRect Dimensions;

    std::vector<std::vector<struct fmPoint>> Data;
    std::vector<char> DataDraw {'*','*','*','*','*','*','*'};

    char xln;
    char yln;
    char xtck;
    char ytck;
    std::vector<int> FGCOL {1,1,1,1,1,1,1};
    std::vector<int> BGCOL {1,1,1,1,1,1,1};
//    std::vector<int> LNCOL = {7};
    std::vector<int> LNTYP {0,0,0,0,0,0,0};

    short unsigned int AxisFG = 7;
    short unsigned int AxisBG = 0;
    short unsigned int AxisMod = A_NORMAL;

    Graph(HybridWindow*, int, int, int, int, mRect, char, char, char, char);
    void SetSpacing();
    void DrawToWindow();
    int AppendData(std::vector<fmPoint>, char, unsigned int, unsigned int, unsigned int);
    int AppendData(unsigned int, fmPoint);
    int InitDataset(char, unsigned int, unsigned int, unsigned int);
    void ChangeDataStream(unsigned int, char, unsigned int, unsigned int, unsigned int);
    void Resize(mRect, int, int, int, int);
    void Resize(int, int, int, int);
    void Resize(mRect);
    void AutoRecalcSize();
};

/****************************************************************
Graph Constructor
	Takes: 
	    -Window to draw to
	    -Minimum X and Y values
	    -Maximum X and Y values
	    -Dimensions of graph to draw to (mRect)
	    -X-axis character
	    -Y-axis character
	    -X-axis tick mark character
	    -Y-axis tick mark character
	Returns:
	    -Graph object with spacing and dimensions set to user
	     restrictions
****************************************************************/
Graph::Graph(HybridWindow *WIN, int xmn, int ymn, int xmx, int ymx, mRect Dim, char XLN = '-', char YLN = '|', char XTCK = '+', char YTCK = '+')
{
    if (xmn < xmx) 
    {
        xmin = xmn;
        xmax = xmx;
    }
    else
    {
        xmin = xmx;
        xmax = xmn;
    }
    if (ymn < ymx) 
    {
        ymin = ymn;
        ymax = ymx;
    }
    else
    {
        ymin = ymx;
        ymax = ymn;
    }
    
    Dimensions = Dim;
    Data.resize(7);

    xln = XLN;
    yln = YLN;
    xtck = XTCK;
    ytck = YTCK;
    win = WIN;
    SetSpacing();
};

/****************************************************************
SetSpacing:
	Sets spacing for graph based on user input.  This function
	    is used in initialization of the graph to set the
	    appropriate X-axis number labels and Y-axis number 
	    labels.  
****************************************************************/
void Graph::SetSpacing()
{
    xlabels.clear();
    ylabels.clear();
    Origin.x = Dimensions.x;
    Origin.y = Dimensions.y + Dimensions.h - 1;
    xsp = 8;
    ysp = 2;
    if (xmax > 0 && xmin < 0)
    {
        for (float i = xmin; i <= 0; i += (float)(xmax-xmin)*xsp/(Dimensions.w))
        {
            xlabels.push_back(i);//((xmax - xmin)*5.0/Dimensions.w)*i);
        }
        Origin.x = (xlabels.size()-1)*xsp + Dimensions.x + xsp*(0-xlabels.at(xlabels.size()-1)) / (float)((xmax-xmin)*xsp/(Dimensions.w)); //(0.0 - (float)Dimensions.w/(5.0*(float)xlabels.at(xlabels.size()-1)));//    Dimensions.x + (xlabels.size()) * 5;//TODO: origin needs to be shifted based on positions!
        for (float i = xlabels.at(xlabels.size()-1) + (xmax - xmin)*(float)xsp/(Dimensions.w); i <= xmax; i+= (xmax-xmin)*(float)xsp/(Dimensions.w))
        {
            xlabels.push_back(i);//((xmax - xmin)*5.0/Dimensions.w)*i);
        }
    }
    else
    {
        for (float i = xmin; i <= xmax; i += (xmax-xmin)*(float)xsp/(Dimensions.w))
        {
            xlabels.push_back(i);//((xmax - xmin)*5.0/Dimensions.w)*i);
        }
    }
    if (ymax > 0 && ymin < 0)
    {
        for (float i = ymin; i <= 0; i += (ymax-ymin)*(float)ysp/(Dimensions.h-1))
        {
            ylabels.push_back(i);//((ymax - ymin)*2.0/Dimensions.h)*i);
        }
        Origin.y = -(ylabels.size()-1)*ysp + Dimensions.y + Dimensions.h - ysp*(0-ylabels.at(ylabels.size()-1)) / (float)((ymax-ymin)*ysp/(Dimensions.h-1)); //(0.0 - (float)Dimensions.w/(5.0*(float)xlabels.at(xlabels.size()-1)));//    Dimensions.x + (xlabels.size()) * 5;//TODO: origin needs to be shifted based on positions!
        //Origin.y = Dimensions.y + Dimensions.h - 1 - (ylabels.size()-1)*ysp + ysp*(0 - ylabels.at(ylabels.size()-1))/(float)((ymax-ymin)*ysp/(Dimensions.h));// - (ylabels.size()) * 2 + ylabels.at(ylabels.size()-1)*(Dimensions.h-1);
        for (float i = ylabels.at(ylabels.size()-1) + (ymax-ymin)*ysp/(Dimensions.h-1); i <= ymax; i+= (ymax-ymin)*(float)ysp/(Dimensions.h-1))
        {
            ylabels.push_back(i);//((ymax - ymin)*2.0/Dimensions.h)*i);
        }
    }
    else
    {
        for (float i = ymin; i <= ymax; i+= (ymax-ymin)*(float)ysp/(Dimensions.h))
        {
            ylabels.push_back(i);//((ymax - ymin)*2.0/(Dimensions.h))*i);
        }
    }
};

/****************************************************************
DrawToWindow
	Draws the graph to the window, as well as relevant axis
	    text.  Additionally draws any data stored in the data
	    storage vectors.
	WARNING: If your X- or Y- axis labels are too long, they
	    will not draw properly (data may overlap or be lost)
****************************************************************/
void Graph::DrawToWindow()
{
    char TEXT[4] = {' ',' ',' ',' '};
    //Draw Axes
    for (int i = Dimensions.x; i <= Dimensions.x + Dimensions.w; i += 1)
    {
        win->DrawPixel(i,Origin.y,xln,AxisFG,AxisBG,AxisMod);
    }
    for (int i = Dimensions.y + Dimensions.h - 1; i >= Dimensions.y-1; i-= 1)
    {
        win->DrawPixel(Origin.x,i,yln,AxisFG,AxisBG,AxisMod);
    }
    win->DrawPixel(Origin.x,Origin.y,'+',AxisFG,AxisBG); //Draw origin (a '+' symbol)
    //Draw Ticks
    for (int i = Dimensions.x; i <= Dimensions.x + Dimensions.w; i += xsp)
    {
        win->DrawPixel(i,Origin.y,xtck,AxisFG,AxisBG,AxisMod);
    }
    for (int i = Dimensions.y + Dimensions.h - 1; i >= Dimensions.y - 1; i -= ysp)
    {
        win->DrawPixel(Origin.x,i,ytck,AxisFG,AxisBG,AxisMod);
    }
    //Draw Labels; TODO: restrict number of decimals
    for (int i = 0; i < xlabels.size(); i++)
    {
        char TEXT[4] = {' ',' ',' ',' '};
        sprintf(&TEXT[0],"%4g",xlabels[i]);
        win->DrawText(i*xsp - 2 + Dimensions.x, Origin.y + 1,TEXT,AxisFG,AxisBG,AxisMod);
    }
    for (int i = 0; i < ylabels.size(); i++)
    {
        char TEXT[4] = {' ',' ',' ',' '};
        sprintf(&TEXT[0],"%4g",ylabels[i]);
        win->DrawText(Origin.x - 5, Dimensions.y + Dimensions.h - 1 - i*ysp,TEXT,AxisFG,AxisBG,AxisMod);
    }
    //Draw Data: To be implemented...
    float y_float;
    for (int i = 0; i < Data.size(); i++)
    {
        for (int j = 0; j < Data[i].size(); j++)
        {
            if (Data[i].at(j).x >= xmin && Data[i].at(j).x <= xmax && Data[i].at(j).y >= ymin && Data[i].at(j).y <= ymax)
            {
                //y_float = (Dimensions.y + Dimensions.h - 1 - Data[i].at(j).y*(Dimensions.h-1)/((ymax-ymin)*ysp)) - (int)(Dimensions.y + Dimensions.h - 1 - Data[i].at(j).y*(Dimensions.h-1)/((ymax-ymin)*ysp));
                win->DrawPixel(Dimensions.x + Dimensions.w*(Data[i].at(j).x-xmin)/(xmax-xmin),Dimensions.y + Dimensions.h - 1 -(Dimensions.h - 1)*(Data[i].at(j).y-ymin)/(ymax-ymin),DataDraw[i],FGCOL[i],BGCOL[i],A_NORMAL);
            //    win->DrawText(Data[i].at(j).x*xsp/Dimensions.w + Dimensions.x, -Data[i].at[j].y*ysp/(Dimensions.h-1) + Dimensions.y + Dimensions.h-1,'*',FGCOL[i],BGCOL[i],LNTYP[i]);
            }
        }
    }
};

/****************************************************************
AppendData
	Appends data to the graph data vectors.  Allows user to 
	    graph additional data.
****************************************************************/
int Graph::AppendData(std::vector<fmPoint> DAT, char DrawCH = '*', unsigned int FG = 7, unsigned int BG = 0, unsigned int MOD = A_NORMAL)
{
    Data.resize(Data.size()+1);
    Data[Data.size()-1] = DAT;
    FGCOL.push_back(FG);
    BGCOL.push_back(BG);
    LNTYP.push_back(MOD);
    DataDraw.push_back(DrawCH);
    return Data.size()-1;
};

int Graph::AppendData(unsigned int Index, fmPoint DAT)
{
    Data[Index].push_back(DAT);
    return Data[Index].size()-1;
};

/****************************************************************
ChangeDataStream:
	Takes: 
	    Index: Index of data being changed
	    DrawCH: Character to draw for data at index location
	    FG: Foreground to draw for data at index location
	    BG: Background to draw for data at index location
	    MOD: Draw mode of data at index location
****************************************************************/
void Graph::ChangeDataStream(unsigned int Index, char DrawCH, unsigned int FG, unsigned int BG, unsigned int MOD)
{
    FGCOL[Index] = FG;
    BGCOL[Index] = BG;
    LNTYP[Index] = MOD;
    DataDraw[Index] = DrawCH;
};

/****************************************************************
InitDataset:
	Takes:
	    DrawCH: Character to draw for new dataset
	    FG: Foreground colour of data in new dataset
	    BG: Background colour of data in new dataset
	    MOD: Draw mode of data in new dataset
	Returns:
	    Index location of new dataset

	This function creates a new set of data values which can
		be drawn
****************************************************************/
int Graph::InitDataset(char DrawCH = '*', unsigned int FG = 7, unsigned int BG = 0, unsigned int MOD = A_NORMAL)
{
    Data.resize(Data.size()+1);
    FGCOL.push_back(FG);
    BGCOL.push_back(BG);
    LNTYP.push_back(MOD);
    DataDraw.push_back(DrawCH);
    return Data.size()-1;
};

/****************************************************************
Resize
	Takes:
	     -New Dimension mRectangle
	     -New maxima and minima
	Returns:
	     -Graph has new dimensions
****************************************************************/
void Graph::Resize(mRect NewDim, int xmn, int ymn, int xmx, int ymx)
{
    Dimensions = NewDim;
    if (xmn < xmx) 
    {
        xmin = xmn;
        xmax = xmx;
    }
    else
    {
        xmin = xmx;
        xmax = xmn;
    }
    if (ymn < ymx) 
    {
        ymin = ymn;
        ymax = ymx;
    }
    else
    {
        ymin = ymx;
        ymax = ymn;
    }
    SetSpacing();
};

void Graph::Resize(int xmn, int ymn, int xmx, int ymx)
{
    if (xmn < xmx) 
    {
        xmin = xmn;
        xmax = xmx;
    }
    else
    {
        xmin = xmx;
        xmax = xmn;
    }
    if (ymn < ymx) 
    {
        ymin = ymn;
        ymax = ymx;
    }
    else
    {
        ymin = ymx;
        ymax = ymn;
    }
    SetSpacing();
};


/****************************************************************
Resize
	Takes:
	    -New Dimension mRectangle
	Returns:
	    -Graph has new dimensions with same maxima and minima
****************************************************************/
void Graph::Resize(mRect NewDim)
{
    Dimensions = NewDim;
    SetSpacing();
};

/****************************************************************
AutoRecalcSize:
	This function automatically rescales the axes (x and y)
		with the goal of having the y-axis encompass all
		data points being drawn as well as ensuring
		that the x-axis is not over-crowded.

	The x-axis may still get over-crowded if the datasets
		have vastly different x-axis scales. 
	(it just looks wierd if we have multiple y-values per 
	x-value) 
****************************************************************/
void Graph::AutoRecalcSize()
{
    if (Data[0].size() > 1)
    {
        unsigned int NumPts = INT_MAX;
        bool Limit = 0;
        xmin = INT_MAX;
        ymin = INT_MAX;
        xmax = INT_MIN;
        ymax = INT_MIN;
        for (int i = 0; i < Data.size(); i++)
        {
            if (NumPts > Data[i].size()) NumPts = Data[i].size();
        }
        if (NumPts > Dimensions.w) Limit = 1;
    
        for (int i = 0; i < Data.size(); i++)
        {
            if (Limit)
            {
                for (int j = NumPts - Dimensions.w; j < Data[i].size(); j++)
                {
                    if (Data[i][j].x < xmin) xmin = Data[i][j].x;
                    if (Data[i][j].x > xmax) xmax = Data[i][j].x;
                    if (Data[i][j].y < ymin) ymin = Data[i][j].y;
                    if (Data[i][j].y > ymax) ymax = Data[i][j].y;
                }
            }
            else
            {
                for (int j = 0; j < Data[i].size(); j++)
                {
                    if (Data[i][j].x < xmin) xmin = Data[i][j].x;
                    if (Data[i][j].x > xmax) xmax = Data[i][j].x;
                    if (Data[i][j].y < ymin) ymin = Data[i][j].y;
                    if (Data[i][j].y > ymax) ymax = Data[i][j].y;
                }
            }
        }
    }
    SetSpacing();
};
