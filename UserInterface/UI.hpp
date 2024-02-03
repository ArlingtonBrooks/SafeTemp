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
User Interface Library
	This file contains all information relating to drawing
	the User Interface in the terminal.  
****************************************************************/
#include <memory>
#include <sys/time.h>

#include "Manager.hpp"
#include "Graph.hpp"

class CursesGraph : public baseGraph<float> {
private:
	ncursesWindow *m_Win; //non-owning pointer to a ncurses window;
public:
	CursesGraph(ncursesWindow *Win) {
		m_Win = Win;
	}
	void DrawAxis(baseGraph<float>::Axis A, std::string const &Label) override { throw 1; }
	void DrawPoints(std::string const &DataName, char Symbol, int Colour) override { throw 1; }
	void DrawLines(std::string const &DataName, bool Dashed, int Colour) override { throw 1; }
	void DrawLegend() override { throw 1; }
};

struct CursXY
{
    int x;
    int y;
};

/*
User Interface Format:
	Left of screen: text explaining each sensor (# and text)
		+/- keys (or left/right): adjust critical temperature
	Right of screen: string showing what command will be executed
		Also: maybe colour adjust?
	Bottom of screen: any additional output or sensor numerical output

Top of the screen will have graph output.
*/

class UserInterface
{
    HybridWindow *win;
    Graph *GG;

    std::vector<FILE*> PROGS;

    std::vector<std::vector<float>> SensorData; //1 vector per sensor
    std::vector<std::string> SensorName;
    std::vector<int> CritTemp; //TODO: shutoff temp
    std::vector<std::string> Command;
    std::vector<unsigned int> SensorColour;
    //TODO: time interval adjust?
    unsigned int FGCOL = 7;
    unsigned int BGCOL = 0;
    unsigned int MODE = A_NORMAL;
    unsigned int Scrollbar = 0;

    struct timeval TV_timer;
    CursXY CC;
    public:
    long int StartTime, LastTime;
    bool TriggerSensors = 1;
    
    int CCmax = 3;
    UserInterface(HybridWindow*, Graph*);
    ~UserInterface();
    bool SetupValues(unsigned int, std::vector<std::string>, std::vector<int>);
    bool SetupValues(std::vector<int>, std::vector<unsigned int>, std::vector<std::string>);
    void ChgFG(unsigned int);
    void ChgBG(unsigned int);
    void ChgMod(unsigned int);
    unsigned int AddSensor();
    void AppendSensorData(unsigned int, float, int);
    bool TriggerDataGrab(int);
    void UpdateTimer(int);
    void draw();
    void MoveCurs(short unsigned int);
    void TempAdjust(short unsigned int);
    void SetCommand(unsigned int, std::string);
    unsigned int GetFG(unsigned int, bool);
    unsigned int GetBG(unsigned int, bool);
    void GetCommand(unsigned int index);
    void GetCommand();

    std::vector<int> GetCritTemps();
    std::vector<std::string> GetCommands();
    std::vector<unsigned int> GetSensorColours();
};

/****************************************************************
User Interface Constructor
	Takes:
	    *WIN: mPointer to Hybrid Window
	    *GRP: mPointer to Graph
	Builds the User Interface object
****************************************************************/
UserInterface::UserInterface(HybridWindow *WIN, Graph *GRP)
{
    win = WIN;
    GG = GRP;
    CC = {0,0};
    gettimeofday(&TV_timer,NULL);
    StartTime = TV_timer.tv_sec;
    LastTime = StartTime;
};

/****************************************************************
User Interface Destructor
	Closes all running programs and exits
****************************************************************/
UserInterface::~UserInterface()
{
    for (int i = 0; i < PROGS.size(); i++)
    {
        if (PROGS[i] != NULL)
        {
            pclose(PROGS[i]);
            PROGS[i] = NULL;
        }
    }
}

/****************************************************************
SetupValues:
	Takes:
	    NumberOfSensors: Number of sensors being processed
	    NAMES: Text names of sensors
	    CRT: Critical Temperatures
	Returns:
	    0 if vector sizes are inconsistent
	    1 if vectors are correctly set up
****************************************************************/
bool UserInterface::SetupValues(unsigned int NumberOfSensors, std::vector<std::string> NAMES, std::vector<int> CRT)
{
    if (CRT.size() != NAMES.size() || NAMES.size() != NumberOfSensors || CRT.size() != NumberOfSensors)
    {
        return 0;
    }
    CritTemp = CRT;
    for (int i = 0; i < NAMES.size(); i++)
    {
        SensorName.resize(i+1);
        SensorName[i] = NAMES[i];
    }
    SensorData.resize(SensorName.size());
    Command.resize(SensorName.size());
    for (int i = 0; i < SensorName.size(); i++)
    {
        SensorColour.push_back(0);
    }
    PROGS.resize(NAMES.size());

    return 1;
};

/****************************************************************
SetupValues:
	Takes:
	    CritTemps: Critical Temperatures
	    Colours: Colours corresponding to what is drawn on
		graph
	    Commands: Names of programs to run when triggered
	Returns:
	    0 if vector sizes are inconsistent
	    1 if vectors are set up correctly
****************************************************************/
bool UserInterface::SetupValues(std::vector<int> CritTemps, std::vector<unsigned int> Colours, std::vector<std::string> Commands)
{
    if (CritTemps.size() != Commands.size() || Commands.size() != Colours.size() || Colours.size() != CritTemps.size())
    {
        return 0;
    }
    CritTemp = CritTemps;
    Command = Commands;
    SensorColour = Colours;
    for (int i = 0; i < CritTemp.size(); i++)
    {
        GG->ChangeDataStream(i,'*',GetFG(SensorColour[i],1),GetBG(SensorColour[i],1),A_NORMAL);
    }
    return 1;
};

/****************************************************************
ChangeFG/ChangeBG/ChgMod
	Takes:
	    FG/BG: New Foreground Colour to draw
	    MOD: Drawing mode of text on screen
****************************************************************/
void UserInterface::ChgFG(unsigned int FG)
{
    FGCOL = FG;
};

void UserInterface::ChgBG(unsigned int BG)
{
    BGCOL = BG;
};

void UserInterface::ChgMod(unsigned int MOD)
{
    MODE = MOD;
};

/****************************************************************
AddSensor
	Returns:
	    Index location of sensor in SensorData vector
****************************************************************/
unsigned int UserInterface::AddSensor()
{
    SensorData.resize(SensorData.size() + 1);
    return SensorData.size()-1;
};

/****************************************************************
AppendSensorData
	Takes:
	    Index: Index location of sensor in SensorData vector
	    DATA: Data to append to vector
	    interval: Time Interval to wait before appending data
****************************************************************/
void UserInterface::AppendSensorData(unsigned int Index, float DATA, int interval)
{
    if (TV_timer.tv_sec - LastTime >= interval || LastTime == StartTime) 
    {
        fmPoint DatamPoint;
        SensorData[Index].push_back(DATA);
        DatamPoint.x = TV_timer.tv_sec - StartTime;
        DatamPoint.y = DATA;
        GG->AppendData(Index,DatamPoint);
    }
};

/****************************************************************
TriggerDataGrab:
	Takes:
	    interval: Time (in seconds) to wait before triggering
		sensor data grab

	TriggerSensors is a boolean value introduced to prevent
		this program from calling a sensor grab every 
		100 ms; this was causing high CPU usage and 
		was completely unnecessary.  This fix should
		allow the program to idle with less CPU usage.
****************************************************************/
bool UserInterface::TriggerDataGrab(int interval)
{
    gettimeofday(&TV_timer,NULL);
    if (TV_timer.tv_sec - LastTime >= interval || LastTime == StartTime) 
    {
        TriggerSensors = 1;
        return 1;
    }
    else 
    {
        TriggerSensors = 0;
        return 0;
    }
};

/****************************************************************
UpdateTimer:
	Takes: 
	    interval: Time (in seconds) to wait before altering
		the LastTime value
****************************************************************/
void UserInterface::UpdateTimer(int interval)
{
    if (TV_timer.tv_sec - LastTime >= interval) 
    {
        LastTime = TV_timer.tv_sec;
        GG->AutoRecalcSize();
        TriggerSensors = 0;
    }
    if (LastTime == StartTime) 
    {
        LastTime++;
        TriggerSensors = 0;
    }
};

/****************************************************************
draw:
	Draws menu to screen, runs commands when triggered.
		DOES NOT CALL GRAPH DRAW FUNCTION!
****************************************************************/
void UserInterface::draw()
{
    win->DrawText(0,1,"NUM   NAME            Crit   Current   Colour  COMMAND",FGCOL,BGCOL,A_UNDERLINE);
    if (SensorName.size() == 0) win->DrawText(10,3,"NO SENSORS REPORTED",0,1);
    if (win->GetHeight() < 5) return;
    for (int i = Scrollbar; i < SensorName.size(); i++)
    {
        std::string Number = "";
        std::string Name = "";
        std::string CrTemp = "";
        std::string CurTemp = "";
        std::string ColStr = "";
        std::string CMD = "";
/*        win->DrawText(sensor NUMBER)
        win->DrawText(sensor NAME)
        win->DrawText(Critical Temp)
        win->DrawText(Colour Pattern Selection)
        win->DrawText(Command)
        TRUNCATE ALL OF THESE*/
//Build text and truncate
        for (int j = 0; j < 4; j++)
        {
            if (j < std::to_string(i+1).length()) Number += std::to_string(i+1)[j];
            else Number += ' ';
        }
        for (int j = 0; j < 15; j++)
        {
            if (j < SensorName[i].length()) Name += SensorName.at(i)[j];
            else Name += ' ';
        }
        for (int j = 0; j < 5; j++)
        {
            if (j < std::to_string(CritTemp[i]).length()) CrTemp += std::to_string(CritTemp[i])[j];
            else CrTemp += ' ';
        }
        for (int j = 0; j < 5; j++)
        {
            if (SensorData[i].size() > 0 && j < std::to_string(SensorData[i].at(SensorData[i].size()-1)).length()) CurTemp += std::to_string(SensorData[i].at(SensorData[i].size()-1))[j];
            else CurTemp += ' ';//std::to_string(i)[0];
        }
        for (int j = 0; j < win->GetWidth()-47; j++)
        {
            if (j < Command[i].length()) CMD += Command[i][j];
            else CMD += ' ';
        }
        for (int j = 0; j < 3; j++)
        {
            if (j < std::to_string(SensorColour[i]).length()) ColStr += std::to_string(SensorColour[i])[j];
            else ColStr += ' ';
        }
        
//Draw Number and Name
        if (SensorData[i].size() > 0 && SensorData[i].at(SensorData[i].size()-1) >= CritTemp[i]) 
        {
            if (CC.x == 0 && CC.y == i + 1 - Scrollbar) 
            {
                win->DrawText(0,i+2-Scrollbar,Number,0,1,A_REVERSE);
                win->DrawText(6, i+2-Scrollbar,Name,0,1,A_REVERSE);
            }
            else
            {
                win->DrawText(0,i+2-Scrollbar,Number,0,1,MODE);
                win->DrawText(6, i+2-Scrollbar,Name,0,1,MODE);
            }
            if (TriggerSensors)
            {
                if (PROGS[i] == NULL) PROGS[i] = popen(Command[i].c_str(),"r");
                else
                {
                    pclose(PROGS[i]);
                    PROGS[i] = NULL;
                    popen(Command[i].c_str(),"r");
                }
            }
            
        }
        else
        {
            if (CC.x == 0 && CC.y == i + 1 - Scrollbar) 
            {
                win->DrawText(0,i+2-Scrollbar,Number,FGCOL,BGCOL,A_REVERSE);
                win->DrawText(6, i+2-Scrollbar,Name,FGCOL,BGCOL,A_REVERSE);
            }
            else
            {
                win->DrawText(0,i+2-Scrollbar,Number,FGCOL,BGCOL,MODE);
                win->DrawText(6, i+2-Scrollbar,Name,FGCOL,BGCOL,MODE);
            }
            if (PROGS[i] != NULL)
            {
                pclose(PROGS[i]);
                PROGS[i] = NULL;
            }
        }
        
//CRITICAL TEMPS
        if (CC.x == 1)// && CC.y == i + 1 - Scrollbar)
        {
            if (CC.y != i + 1 - Scrollbar)
            {
                if (Scrollbar == 0 && i - Scrollbar < win->GetHeight()-5) win->DrawText(22,i+2-Scrollbar,CrTemp,FGCOL,BGCOL,MODE);
                if (Scrollbar > 0 && i - Scrollbar >= 1 && i - Scrollbar < win->GetHeight()-5) win->DrawText(22,i+2-Scrollbar,CrTemp,FGCOL,BGCOL,MODE);
            }
            else
            {
                if (Scrollbar == 0 && i - Scrollbar < win->GetHeight()-5) win->DrawText(22,i+2-Scrollbar,CrTemp,FGCOL,BGCOL,A_REVERSE);
                if (Scrollbar > 0 && i - Scrollbar >= 1 && i - Scrollbar < win->GetHeight()-5) win->DrawText(22,i+2-Scrollbar,CrTemp,FGCOL,BGCOL,A_REVERSE);
            }
        }
        else
        {
            if (Scrollbar == 0 && i - Scrollbar < win->GetHeight()-5) win->DrawText(22,i+2-Scrollbar,CrTemp,FGCOL,BGCOL,MODE);
            if (Scrollbar > 0 && i - Scrollbar >= 1 && i - Scrollbar < win->GetHeight()-5) win->DrawText(22,i+2-Scrollbar,CrTemp,FGCOL,BGCOL,MODE);
        }

//Draw current temps
       if (Scrollbar == 0 && i - Scrollbar < win->GetHeight()-5) win->DrawText(29,i+2-Scrollbar,CurTemp,FGCOL,BGCOL,MODE);
       if (Scrollbar > 0 && i - Scrollbar >= 1 && i - Scrollbar < win->GetHeight()-5) win->DrawText(29,i+2-Scrollbar,CurTemp,FGCOL,BGCOL,MODE);

//DRAW COLOURS
        if (CC.x == 2)// && CC.y == i + 1 - Scrollbar)
        {
            if (CC.y != i + 1 - Scrollbar)
            {
                if (Scrollbar == 0 && i - Scrollbar < win->GetHeight()-5) win->DrawText(40,i+2-Scrollbar,ColStr,GetFG(SensorColour[i],1),GetBG(SensorColour[i],1),MODE);
                if (Scrollbar > 0 && i - Scrollbar >= 1 && i - Scrollbar < win->GetHeight()-5) win->DrawText(40,i+2-Scrollbar,ColStr,GetFG(SensorColour[i],1),GetBG(SensorColour[i],1),MODE);
            }
            else
            {
                if (Scrollbar == 0 && i - Scrollbar < win->GetHeight()-5) win->DrawText(40,i+2-Scrollbar,ColStr,GetFG(SensorColour[i],1),GetBG(SensorColour[i],1),A_BOLD);
                if (Scrollbar > 0 && i - Scrollbar >= 1 && i - Scrollbar < win->GetHeight()-5) win->DrawText(40,i+2-Scrollbar,ColStr,GetFG(SensorColour[i],1),GetBG(SensorColour[i],1),A_BOLD);
            }
        }
        else
        {
            if (Scrollbar == 0 && i - Scrollbar < win->GetHeight()-5) win->DrawText(40,i+2-Scrollbar,ColStr,GetFG(SensorColour[i],1),GetBG(SensorColour[i],1),MODE);
            if (Scrollbar > 0 && i - Scrollbar >= 1 && i - Scrollbar < win->GetHeight()-5) win->DrawText(40,i+2-Scrollbar,ColStr,GetFG(SensorColour[i],1),GetBG(SensorColour[i],1),MODE);
        }



//Draw Commands
        if (CC.x == 3)// && CC.y == i + 1 - Scrollbar)
        {
            if (CC.y != i + 1 - Scrollbar)
            {
                if (Scrollbar == 0 && i - Scrollbar < win->GetHeight()-5) win->DrawText(47,i+2-Scrollbar,CMD,FGCOL,BGCOL,MODE);
                if (Scrollbar > 0 && i - Scrollbar >= 1 && i - Scrollbar < win->GetHeight()-5) win->DrawText(47,i+2-Scrollbar,CMD,FGCOL,BGCOL,MODE);
            }
            else
            {
                if (Scrollbar == 0 && i - Scrollbar < win->GetHeight()-5) win->DrawText(47,i+2-Scrollbar,CMD,FGCOL,BGCOL,A_REVERSE);
                if (Scrollbar > 0 && i - Scrollbar >= 1 && i - Scrollbar < win->GetHeight()-5) win->DrawText(47,i+2-Scrollbar,CMD,FGCOL,BGCOL,A_REVERSE);
            }
        }
        else
        {
            if (Scrollbar == 0 && i - Scrollbar < win->GetHeight()-5) win->DrawText(47,i+2-Scrollbar,CMD,FGCOL,BGCOL,MODE);
            if (Scrollbar > 0 && i - Scrollbar >= 1 && i - Scrollbar < win->GetHeight()-5) win->DrawText(47,i+2-Scrollbar,CMD,FGCOL,BGCOL,MODE);
        }
    }

//Draw scroll text (if needed)
    if (Scrollbar > 0)
    {
        win->DrawText(0,2,"---SCROLL UP---");
        if (CC.y < 2)
        {
            CC.y = 2;
            Scrollbar -= 1;
        }
    }
    if (SensorName.size() - Scrollbar >= win->GetHeight() - 4)
    {
        win->DrawText(0, win->GetHeight() - 3, "---SCROLL DOWN---");
        if (CC.y >= win->GetHeight()-4) 
        {
            CC.y = win->GetHeight()-5;
            Scrollbar += 1;
        }
    }
};

/****************************************************************
MoveCurs:
	Takes:
	    Direction: Direction Key from User Input
		1 = right
		2 = down
		3 = left
		4 = up

	This function alters the "cursor" position on screen and
		restricts movement if limited.
****************************************************************/
void UserInterface::MoveCurs(short unsigned int Direction)
{
    switch (Direction)
    {
        case 1: CC.x += 1; break;
        case 2: CC.y += 1; break;
        case 3: CC.x -= 1; break;
        case 4: CC.y -= 1; break;
        default: break;
    }
    if (CC.y <= 0) CC.y = 1;
    if (CC.y >= SensorName.size() - Scrollbar) CC.y = SensorName.size()-Scrollbar;
    if (CC.y >= win->GetHeight()-2) CC.y = win->GetHeight()-2;
    if (CC.x < 0) CC.x = 0;
    if (CC.x > CCmax) CC.x = CCmax;
};

/****************************************************************
TempAdjust
	Takes:
	    Direction: Direction Key from User Input
		0 = Increment
		1 = Decrement

	This function alters the draw colour of datasets on the
		graph.
****************************************************************/
void UserInterface::TempAdjust(short unsigned int Direction)
{
    if (Direction == 0 && CC.x == 1)
    {
        CritTemp[CC.y + Scrollbar - 1] += 1;
    }
    if (Direction == 1 && CC.x == 1)
    {
        CritTemp[CC.y + Scrollbar - 1] -= 1;
    }
    if (Direction == 0 && CC.x == 2 && SensorColour[CC.y + Scrollbar - 1] < 64)
    {
        SensorColour[CC.y + Scrollbar - 1] += 1;
        GG->ChangeDataStream(CC.y + Scrollbar - 1,'*',GetFG(SensorColour[CC.y + Scrollbar - 1],1),GetBG(SensorColour[CC.y + Scrollbar - 1],1),A_NORMAL);
    }
    if (Direction == 1 && CC.x == 2 && SensorColour[CC.y + Scrollbar - 1] > 0)
    {
        SensorColour[CC.y + Scrollbar - 1] -= 1;
        GG->ChangeDataStream(CC.y + Scrollbar - 1,'*',GetFG(SensorColour[CC.y + Scrollbar - 1],1),GetBG(SensorColour[CC.y + Scrollbar - 1],1),A_NORMAL);
    }
};

/****************************************************************
SetCommand:
	Takes:
	    SensorNum: Index location of sensor being modified
	    Cmd: Command to run on trigger for sensor 'sensorNum'
****************************************************************/
void UserInterface::SetCommand(unsigned int SensorNum, std::string Cmd)
{
    Command[SensorNum] = Cmd;
};

/****************************************************************
GetFG:
	Takes:
	    Code: Either colour code or sensor index location
	    Internal: 
		1 = return foreground colour based on 'Code'
		0 = return foreground for sensor index 'Code'
****************************************************************/
unsigned int UserInterface::GetFG(unsigned int Code, bool Internal)
{
    if (Internal) //internal: Code refers to a colour code;
    {
        for (int i = 0; i < 10; i++)
        {
            switch (Code)
            {
                case 0: return 0; break;
                case 1: return 1; break;
                case 2: return 2; break;
                case 3: return 3; break;
                case 4: return 4; break;
                case 5: return 5; break;
                case 6: return 6; break;
                case 7: return 7; break;
                default: Code -= 8;
            }
        }
    }
    else //Not Internal: Code refers to a sensor index number
    {
        unsigned int COL = SensorColour[Code];
        for (int i = 0; i < 10; i++)
        {
            switch (Code)
            {
                case 0: return 0; break;
                case 1: return 1; break;
                case 2: return 2; break;
                case 3: return 3; break;
                case 4: return 4; break;
                case 5: return 5; break;
                case 6: return 6; break;
                case 7: return 7; break;
                default: Code -= 8;
            }
        }
        
    }
};

/****************************************************************
GetBG:
	Takes:
	    Code: Either colour code or sensor index location
	    Internal: 
		1 = return background colour based on 'Code'
		0 = return background for sensor index 'Code'
****************************************************************/
unsigned int UserInterface::GetBG(unsigned int Code, bool Internal)
{
    if (Internal) //internal: Code refers to a colour code;
    {
        for (int i = 0; i < 10; i++)
        {
            switch (Code/8)
            {
                case 0: return 7; break;
                case 1: return 6; break;
                case 2: return 5; break;
                case 3: return 4; break;
                case 4: return 3; break;
                case 5: return 2; break;
                case 6: return 1; break;
                case 7: return 0; break;
                default: Code -= 8;
            }
        }
    }
    else //Not Internal: Code refers to a sensor index number
    {
        unsigned int COL = SensorColour[Code];
        for (int i = 0; i < 10; i++)
        {
            switch (Code/8)
            {
                case 0: return 7; break;
                case 1: return 6; break;
                case 2: return 5; break;
                case 3: return 4; break;
                case 4: return 3; break;
                case 5: return 2; break;
                case 6: return 1; break;
                case 7: return 0; break;
                default: Code -= 8;
            }
        }
        
    }
};

/****************************************************************
GetCommand:
	Takes:
	    index: Index location for sensor being modified

	This function modifies the command which is run when
		sensor is triggered

KNOWN BUG: backspace and special characters DO NOT WORK.  User 
	cannot correct mistake;
WORKAROUND: User must accept command and re-edit, entering the 
	correct command.
	    User can also edit config file to enter command.
****************************************************************/
void UserInterface::GetCommand(unsigned int index)
{
    std::string cmd = "";
    printw("Enter Command for Sensor %d: ",index);
    int CHCH = KEY_BACKSPACE;
    while (CHCH != KEY_EOL && CHCH != KEY_ENTER && CHCH != '\n' && CHCH != '\r')
    {
        if (CHCH != KEY_BACKSPACE && CHCH < KEY_MIN && CHCH != ERR)
        {
            cmd += CHCH;
        }
        else
        {
            if (CHCH == KEY_BACKSPACE && cmd.length() > 0) cmd.pop_back();
        }
        CHCH = getch();
    }
    Command[index] = cmd;
    return;
};

/****************************************************************
GetCommand:
	This function modifies the command which is run when the
		sensor highlighted by the 'scrollbar' or 'cursor'
		is triggered.
****************************************************************/
void UserInterface::GetCommand()
{
    unsigned int index = CC.y + Scrollbar - 1;
    if (CC.x == 3)
    {
        std::string cmd = "";
        printw("Enter Command for Sensor %d: ",index+1);
        int CHCH = KEY_BACKSPACE;
        while (CHCH != KEY_EOL && CHCH != KEY_ENTER && CHCH != '\n' && CHCH != '\r')
        {
            if (CHCH != KEY_BACKSPACE && CHCH < KEY_MIN && CHCH != ERR)
            {
                cmd += (char)CHCH;
            }
            else
            {
                if (CHCH == KEY_BACKSPACE && cmd.length() > 0) cmd.pop_back();
            }
            CHCH = getch();
        }
        Command[index] = cmd;
    }
    return;
};

/****************************************************************
GetCritTemps/GetCommands/GetSensorColours:
	These functions return the data stored in the internal
		vectors corresponding to that information.
		(CritTemps/Commands/Colours)
****************************************************************/
std::vector<int> UserInterface::GetCritTemps()
{
    return CritTemp;
};

std::vector<std::string> UserInterface::GetCommands()
{
    return Command;
};

std::vector<unsigned int> UserInterface::GetSensorColours()
{
    return SensorColour;
};
