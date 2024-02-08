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
TempSafe Program
	Note that the names TempSafe and SafeTemp are used 
		interchangeably in the documentation.

KNOWN BUGS:
	-In the User Interface, backspace does not work when 
		editing "commands"
	-In the User Interface, the cursor position is not steady
	-The Estimated Maximum Temperatures make no sense
	-This program WILL RUN OUT OF MEMORY EVENTUALLY WHEN USING
				THE GUI.  The graphical interfaces store the
				temperatures from the start of the program.  

RECOMMENDATION:
		-Since there is a known (eventual) memory issue, it is
				recommended that for a system with 4 temperature
				sensors, a time interval of 1 minute be selected.
				This will result in up to 10 MB of sensor data
				being stored after a run time of about 800 days.

PLANNED UPDATES:
		GUI:
			-Datapoint Count selector (select the number of time
				intervals to display)
			-Save sensor data as CSV for analysis
			-Save graph PNG to filename
			-Pop-up for handling all sensor configurations
			-Sensor construction interface to read data from
				a file stream instead of lm_sensors
****************************************************************/

#include <fstream>
#include <iostream>
#include <thread>
#include <string>
#include <vector>
#include <cmath>
#include <ctime>
#include <memory>
#include <stdexcept>
#include <unordered_map>
//#include <config.h>
#include <sensors/sensors.h> //lm_sensors-devel
#include <sensors/error.h>   //lm_sensors-devel
#if HAVE_LIBNVIDIA_ML
#include <NVCtrl/NVCtrl.h>
#include <nvml.h>
#endif

#include "UserInterface/Manager.hpp"
#include "UserInterface/Graph.hpp"
#include "UserInterface/UI.hpp"
#include "UserInterface/GTKInterface.hpp"
#include "Sensors/SensorClass.hpp"
using namespace std;

/****************************************************************
Global Variables:
	-TimeStep: amount of time (in microseconds) between 
		temperature readings
	-StartTime: the system time that the program started at
	-MaxTemps: Critical Temperatures that trigger a warning
	-MinTemp: the minimum of MaxTemps
	-File: config file for lm_sensors
	-Temp: file of critical temperatures (non-UI mode)
	-Prog: file for running a program (i.e. a command)
	-run: whether the program runs in a loop
	-PrtTmp: whether to print temperatures on-screen
	-Stats: whether to calculate statistics (not reliable)
	-Command: the actual command text to be run when 
		temperature threshold is exceeded
	-UseUI: whether to use the EXPERIMENTAL user interface
	-helptext: the text to print with the -h option
****************************************************************/
/*int TimeStep = 5000000;
time_t StartTime = time(NULL);
vector<int> MaxTemps;
int MinTemp;
FILE *File = NULL;
FILE *Temp = NULL;
FILE *Prog = NULL;
bool run = 1;
bool PrtTmp = 0;
bool Stats = 0;
string Command = "";
string HomeDir = "";

bool UseUI = 0;
bool UseGUI = 0;*/

struct InputArguments {
	vector<int> MaxTemps;
	string Command = "";
	string HomeDir = "";
	time_t StartTime = time(NULL);
	int TimeStep = 5000000;
	int MinTemp;
	FILE* File = NULL;
	FILE* Temp = NULL;
	FILE* Prog = NULL;
	bool run = 1;
	bool PrtTmp = 0;
	bool Stats = 0;
	bool UseUI = 0;
	bool UseGUI = 0;
	bool Success = 0;
};

const char* helptext = "tempsafe -p FILE -w TIME -i -v -f FILE -C SCRIPT \nsensors-checking program\nKevin Brooks, 2015\nUsage: \n-p\t\tPath to lm-sensors config file\n-w\t\ttime interval to wait between checks (seconds); default is 5 seconds\n-f\t\tLoad temperatures from a file\n-i\t\tDon't run, just print temperatures and exit (implies -v)\n-v\t\tVerbose output (print temperatures at each TIME interval)\n-C\t\texecute a shell script;\n\t\tSCRIPT path should be given in double-quotes.\n-UI\t\tEXPERIMENTAL: Start with User Interface (overrides -v, -c, -f, and -s)\n\t\tUser Interface reads a config file from ~/.config/TempSafe.cfg \n--use-gtk\tEXPERIMENTAL: Use GTK graphical interface\n\t\tReads config file from ~/.config/TempSafe_GUI.cfg\n-h\t\tPrint this help file\n\n";

InputArguments ProcessArgs(int, char**);
bool ParseTemp(InputArguments &InArgs);
bool ProcessTemp(int,double, InputArguments &InArgs);
//double deriv(double,double,int);
double avg(double, double);
//double EstMaxTemp(vector<double>, vector<double>, vector<double>, vector<time_t>, time_t StartTime);
//double EstMaxTime(vector<double>, vector<double>, vector<double>, vector<time_t>, time_t StartTime);
bool ReadConfig(UserInterface*, unsigned int, InputArguments &InArgs);
bool WriteConfig(UserInterface*, InputArguments &InArgs);
void SetHomeDirectory(InputArguments &InArgs);

/** @brief Set size of graph window */
Rect<int> GetGraphSize(WinSize const &MainWindowSize)
{
	Rect<int> GraphSize;
	GraphSize.x = 0;
	GraphSize.y = 0;
	GraphSize.w = MainWindowSize.x;
	GraphSize.h = MainWindowSize.y-10;
	return GraphSize;
}

/** @brief Set size of UI window */
Rect<int> GetUiSize(WinSize const &MainWindowSize)
{
	Rect<int> UiSize;
	UiSize.x = 0;
	UiSize.y = MainWindowSize.y- 10;
	UiSize.w = MainWindowSize.x;
	UiSize.h = 10;
	return UiSize;
}

void NCurses_Draw(MainWindow &Main, std::vector<SensorDetailLine> const &SensorDetails, bool Resize) {
	WinSize MainWindowSize = Main.GetSize();
	if (Resize) {
		Main.GetSubWindow("Graph").Resize(GetGraphSize(MainWindowSize));
		Main.GetSubWindow("UI").Resize(GetUiSize(MainWindowSize));
		if (MainWindowSize.y >= 24) Main.RedrawAll();
	}
#if UI_TEST
	NCursesPrintGraphAxes(Main.GetSubWindow("Graph"));
	NCursesPrintUiToWindow(Main.GetSubWindow("UI"),{0,0},0,SensorDetails);
#endif
	Main.Draw();
	if (MainWindowSize.y < 24 || MainWindowSize.x < 50) {
		Main.PrintString(MainWindowSize.y/2,MainWindowSize.x/2-10,"Window size too small");
		Main.Refresh();
	} else {
		mvwprintw(Main.GetSubWindow("Graph").GetHandle().get(),0,0,"  Plot of Temperature VS Time  ");
		Main.RefreshAll();
	}
}

static unsigned GetTotalNumberOfSensors(std::vector<std::shared_ptr<temperature_sensor_set>> const Sensors) {
	unsigned ret = 0;
	for (auto const &i : Sensors) {
		ret += i.get()->GetNumberOfSensors();
	}
	return ret;
}

void RunNCurses(InputArguments &InArgs, std::vector<std::shared_ptr<temperature_sensor_set>> &Sensors, std::unordered_map<std::string,SensorDetailLine> const &NameMap) {
	MainWindow Main;
	unsigned TotalNSensors = GetTotalNumberOfSensors(Sensors);
	NCurses_Input InputHandler(TotalNSensors,3);
	//Create UI and graph windows;
	Main.CreateSubWindow("Graph",GetGraphSize(Main.GetSize()));
	Main.CreateSubWindow("UI",GetUiSize(Main.GetSize()));
	Main.RefreshAll();
	int i = 0;
	while (i != 'q') { //step
		i = InputHandler.GetKey();
		std::vector<SensorDetailLine> StepDetails = GetAllSensorDetails(Sensors,NameMap);

		NCurses_Draw(Main,StepDetails,i == KEY_RESIZE);
	}
}

int main(int argc,char** argv)
{
	/* Check for the presence of libsensors package */
	if (HAVE_LIBSENSORS != 1) 
	{
		std::cout << "WARNING: lm_sensors must be installed for this program to work.\n\tPlease install lm_sensors and re-configure this package.\n\n";
		return -1;
	}
	
	/* Process command line arguments */
	InputArguments InArgs = ProcessArgs(argc,argv);
	if (!InArgs.Success)
	{
		std::cerr << "Failed to parse input.\n";
		std::cout << helptext;
		return -3;
	}
	if (InArgs.UseUI && InArgs.UseGUI)
	{
		std::cerr << "ERROR: -UI and --use-gtk cannot be used simultaneously\n";
		return -4;
	}

	std::vector<std::shared_ptr<temperature_sensor_set>> AllSensors;
#if UI_TEST
	AllSensors.emplace_back(std::make_shared<test_sensor>(15));
#else
	AllSensors.emplace_back(std::make_shared<lm_sensor>(nullptr));
#endif
	std::unordered_map<std::string,SensorDetailLine> BasicSensorMap;
	if (InArgs.UseUI) {
		RunNCurses(InArgs,AllSensors,BasicSensorMap);
		return 0;
	}

	return -5; //Temporary; don't go beyond this.
	/* Statistics variables */
	vector<vector<time_t>> X_pts;
	vector<vector<double>> Y_pts;
	vector<vector<double>> Yp_pts;
	vector<vector<double>> Ypp_pts;

	/* Initialize NVidia devices if installed */
#if HAVE_LIBNVIDIA_ML
	static_assert(false,"Nvidia ML has been temporarily disabled");
	/*bool nv = true;
	unsigned int nvDevCount;
	vector<nvmlDevice_t> nvDev;
	unsigned int nvTempTmp = 0;
	if (nvmlInit() != NVML_SUCCESS)
	{
		fprintf(stderr,"Could not initialize nvml library... No GPU temperatures will be reported.\n");
		nv = false;
	}
	else
	{
		nvmlDeviceGetCount(&nvDevCount);
		for (unsigned int i = 0; i < nvDevCount; i++)
		{
			nvDev.resize(i+1);
			if (nvmlDeviceGetHandleByIndex(i,&nvDev[i]) != NVML_SUCCESS) 
			{
				fprintf(stderr,"Error: NV Device at index %d could not be used.\n",i);
				nvDev.erase(nvDev.begin()+i);
			}
		}
		if (nvDev.size() == 0) nv = false;
		//Check that we can read temperatures from this device (remove if we can't)
		for (unsigned int i = 0; i < nvDev.size(); i++)
		{
			if (nvmlDeviceGetTemperature(nvDev[i],NVML_TEMPERATURE_GPU,&nvTempTmp) != NVML_SUCCESS) nvDev.erase(nvDev.begin()+i);
		}
	}*/
#endif

	lm_sensor Sensors(nullptr);
	std::vector<std::string> SensorNames;
	for (unsigned i = 0; i != Sensors.GetNumberOfSensors(); i++) {
		SensorNames.push_back(Sensors.GetSensorName(i));
	}
	std::vector<std::string> ChipNames = SensorNames;

	if (Sensors.GetNumberOfSensors() == 0)
		throw std::runtime_error("No sensors were found.");

	/* Initialize User Interface */
	SetHomeDirectory(InArgs);
#if HAVE_LIBNVIDIA_ML
	static_assert(false,"Nvidia ML has been temporarily disabled");
	/*if (nv)
	{
		for (int i = 0; i < nvDev.size(); i++) SensorNames.push_back("NVidia Device");
	}*/
#endif
	Manager WM;
	int WM_Graph = WM.AddWin(0,0,WM.MAIN->_maxx,(int)((float)WM.MAIN->_maxy/2.0)-2,1,0);
	int WM_Data = WM.AddWin(0,(int)((float)WM.MAIN->_maxy/2.0)+2,WM.MAIN->_maxx,(int)((float)WM.MAIN->_maxy/2.0)-1,0,1);
	Graph GP(&WM.Wins[WM_Graph],0,0,10,40,{9,2,WM.MAIN->_maxx-12,(int)((float)WM.MAIN->_maxy/2.0)-7});
#if HAVE_LIBNVIDIA_ML
	static_assert(false,"Nvidia ML has been temporarily disabled");
	/*for (int i = 0; i < ChipNames.size()+nvDev.size(); i++)
	{
		GP.InitDataset();
	}*/
#endif
	UserInterface UI(&WM.Wins[WM_Data],&GP);
	if (Sensors.GetNumberOfSensors() != InArgs.MaxTemps.size())
	{
		for (unsigned i = 0; i < Sensors.GetNumberOfSensors(); i++) {
			InArgs.MaxTemps.push_back(0);
		}
	}
	if (!UI.SetupValues(Sensors.GetNumberOfSensors(), SensorNames, InArgs.MaxTemps) && InArgs.UseUI)
	{
		endwin();
		std::cerr << "ERROR: User Interface could not be configured.  Exiting...\n";
		std::cerr << "\tNumberOfSensors: " << SensorNames.size() << "\n\tNumber of Critical Temperatures: " << InArgs.MaxTemps.size() << "\n";
		return -1;
	}
	if (InArgs.UseUI)
	{
		if (!ReadConfig(&UI,SensorNames.size(),InArgs))
		{
			endwin();
			std::cerr << "Error loading config file ~/.config/TempSafe.cfg.  Exiting...\n";
		}
	}
	int CharBuffer = 0;
	if (!InArgs.UseUI) endwin();
	if (InArgs.UseUI) nodelay(stdscr,1);

#if HAVE_GTK == 1 && HAVE_GNUPLOT == 1
	std::thread GTKMain;
	if (InArgs.UseGUI)
	{
		GUI::BuildInterface(argc,argv,SensorNames,&InArgs.run);
		GTKMain = std::thread(gtk_main);
	}
#endif
	
	/* Main Events Loop */
	if (true)
	{
		double val;
#if HAVE_NVIDIA_ML
	static_assert(false,"Nvidia ML has been temporarily disabled");
#else
#endif
		if (InArgs.Stats)
		{
			X_pts.resize(Sensors.GetNumberOfSensors());
			Y_pts.resize(Sensors.GetNumberOfSensors());
			Yp_pts.resize(Sensors.GetNumberOfSensors());
			Ypp_pts.resize(Sensors.GetNumberOfSensors());
		}

		while (true)
		{
			/* Loop through all available sensors and perform relevant actions */
			if (!InArgs.UseUI && !InArgs.UseGUI)
			{
				for (unsigned i = 0; i < ChipNames.size(); i++)
				{
					val = Sensors.GetTemperature(ChipNames[i]);
					if (InArgs.Stats) X_pts[i].push_back(time(NULL));
					if (InArgs.Stats) Y_pts[i].push_back(val);
					if (InArgs.PrtTmp) std::cout << "Sensor " << i << ": " << val << "\n";
					ProcessTemp(i,val,InArgs);
				}
#if HAVE_LIBNVIDIA_ML
	static_assert(false,"Nvidia ML has been temporarily disabled");
				/*if (nv) //NVIDIA GPU data
				{
					//Loop through all nvidia chips and perform relevant actions
					for (int i = 0; i < nvDev.size(); i++)
					{
						if (nvmlDeviceGetTemperature(nvDev[i],NVML_TEMPERATURE_GPU,&nvTempTmp) != NVML_SUCCESS) fprintf(stderr,"Failed to read temperature from NVIDIA chip (%d)\n",ChipNames.size());
						val = (double)nvTempTmp;
						//NV_CTRL_THERMAL_SENSOR_READING;
						if (InArgs.Stats) X_pts[ChipNames.size() + i].push_back(time(NULL));
						if (InArgs.Stats) Y_pts[ChipNames.size() + i].push_back(val);
						if (InArgs.Stats && Y_pts[ChipNames.size() + i].size() >= 2) Yp_pts[ChipNames.size() + i].push_back(deriv(Y_pts[ChipNames.size() + i][Y_pts.size()-2],Y_pts[ChipNames.size() + i][Y_pts[ChipNames.size() + i].size()-1],InArgs.TimeStep));
						if (InArgs.Stats && Yp_pts[ChipNames.size() + i].size() >= 2) Ypp_pts[ChipNames.size() + i].push_back(deriv(Yp_pts[ChipNames.size() + i][Yp_pts.size()-2],Yp_pts[ChipNames.size() + i][Yp_pts[ChipNames.size() + i].size()-1],InArgs.TimeStep));
						if (InArgs.Stats) MaxTemp[ChipNames.size() + i] = EstMaxTemp(Y_pts[ChipNames.size() + i],Yp_pts[ChipNames.size() + i],Ypp_pts[ChipNames.size() + i],X_pts[ChipNames.size() + i],InArgs.StartTime);
						if (InArgs.Stats) MaxTime[ChipNames.size() + i] = EstMaxTime(Y_pts[ChipNames.size() + i],Yp_pts[ChipNames.size() + i],Ypp_pts[ChipNames.size() + i],X_pts[ChipNames.size() + i],InArgs.StartTime);
						if (InArgs.PrtTmp) printf("Sensor %d: %f\n",ChipNames.size() + i,val);
						if (InArgs.PrtTmp && Stats && Ypp_pts[ChipNames.size() + i].size() >= 1) printf("Estimated max temperature is %f in %f seconds for sensor %d\n",MaxTemp[ChipNames.size() + i],MaxTime[ChipNames.size() + i],ChipNames.size() + i);
						ProcessTemp(ChipNames.size() + i,val,InArgs);
					}
				}*/
#endif
			}
			else if (InArgs.UseUI)//UseUI OR UseGUI
			{
				if (UI.TriggerSensors)
				{
					for (unsigned i = 0; i < ChipNames.size(); i++)
					{
						GUI::Handle.AddData(Sensors.GetTemperature(ChipNames[i]),i);
						UI.AppendSensorData(i,val,InArgs.TimeStep/1000000);
					}
#if HAVE_LIBNVIDIA_ML
	static_assert(false,"Nvidia ML has been temporarily disabled");
					/*if (nv) //NVIDIA GPU data
					{
						//Loop through all nvidia chips and perform relevant actions
						for (int i = 0; i < nvDev.size(); i++)
						{
							if (nvmlDeviceGetTemperature(nvDev[i],NVML_TEMPERATURE_GPU,&nvTempTmp) != NVML_SUCCESS) fprintf(stderr,"Failed to read temperature from NVIDIA chip (%d)\n",ChipNames.size() + i);
							val = (double)nvTempTmp;
							//NV_CTRL_THERMAL_SENSOR_READING;
							UI.AppendSensorData(ChipNames.size()+i,val,InArgs.TimeStep/1000000);
						}
					}*/
#endif

				}
			}
#if HAVE_GTK == 1 && HAVE_GNUPLOT == 1
			else if (InArgs.UseGUI)
			{
				if (GUI::Handle.GetTimeTrigger(InArgs.TimeStep/1000000))
				{
					for (unsigned i = 0; i < ChipNames.size(); i++)
					{
						//sensors_get_value(ChipNames[i],SubFeats[i]->number,&val);
						GUI::Handle.AddData(Sensors.GetTemperature(ChipNames[i]),i);
					}
	#if HAVE_LIBNVIDIA_ML
	static_assert(false,"Nvidia ML has been temporarily disabled");
					/*if (nv) //NVIDIA GPU data
					{
						// Loop through all nvidia chips and perform relevant actions
						for (int i = 0; i < nvDev.size(); i++)
						{
							if (nvmlDeviceGetTemperature(nvDev[i],NVML_TEMPERATURE_GPU,&nvTempTmp) != NVML_SUCCESS) fprintf(stderr,"Failed to read temperature from NVIDIA chip (%d)\n",ChipNames.size() + i);
							val = (double)nvTempTmp;
							//NV_CTRL_THERMAL_SENSOR_READING;
							GUI::Handle.AddData((float)val,ChipNames.size()+i);
//							UI.AppendSensorData(ChipNames.size()+i,val,TimeStep/1000000);
						}
					}*/
	#endif
					g_idle_add((GSourceFunc)GUI::replot,GUI::Objects);
				}
			}
#endif


			if (InArgs.PrtTmp && !InArgs.UseUI) std::cout << "Finished Line\n";
			if (!InArgs.run) break;

			/* If User Interface is enabled, perform the required actions to process data */
			if (InArgs.UseUI)
			{ //FIXME: UI is broken somewhere in here;
				UI.UpdateTimer(InArgs.TimeStep/1000000);
				UI.TriggerDataGrab(InArgs.TimeStep/1000000);
				CharBuffer = 0;
				CharBuffer = getch();

				if (CharBuffer == KEY_UP) UI.MoveCurs(4);
				if (CharBuffer == KEY_DOWN) UI.MoveCurs(2);
				if (CharBuffer == KEY_RIGHT) UI.MoveCurs(1);
				if (CharBuffer == KEY_LEFT) UI.MoveCurs(3);
				if (CharBuffer == '+') UI.TempAdjust(0);
				if (CharBuffer == '-') UI.TempAdjust(1);
				if (CharBuffer == KEY_ENTER || CharBuffer == '\n' || CharBuffer == '\r') UI.GetCommand();
				if (CharBuffer == 'q') InArgs.run = 0;

				WM.ClearBuffer(WM_Data);
				if (UI.TriggerSensors)
				{
					WM.ClearBuffer(WM_Graph);
					WM.Wins[WM_Graph].DrawAlignedText(ALIGN_BOTTOM_CENTER,"Time Since Program Start (seconds)",7,0);
					WM.Wins[WM_Graph].DrawAlignedText(ALIGN__LEFT,"Temperature C",7,0);
					WM.Wins[WM_Graph].DrawAlignedText(ALIGN_TOP_CENTER,"Temperature vs. Time",7,0);
				}
				WM.Wins[WM_Data].DrawAlignedText(ALIGN_TOP_CENTER,"-Main Menu- (tap q to exit)",0,7,A_BOLD & A_STANDOUT);
				GP.DrawToWindow();
				UI.draw();
				move(WM.MAIN->_maxy,0);
				WM.DrawWindows();

			}

			if (!InArgs.UseUI && !InArgs.UseGUI) usleep((int)(InArgs.TimeStep));
			if (InArgs.UseUI && !InArgs.UseGUI) timeout(500);
#if HAVE_GTK == 1 && HAVE_GNUPLOT == 1
			if (InArgs.UseGUI) 
			{
				usleep(10000);//((int)TimeStep/4);
				g_idle_add((GSourceFunc)GUI::CheckResize,GUI::Objects);
			}
#endif
		}
	}
	else 
	{
		std::cerr << "Vector sizes uneven\n";
		return -1;
	}
	/* Clean up on exit */
#if HAVE_GTK == 1 && HAVE_GNUPLOT == 1
	if (InArgs.UseGUI) GTKMain.join();
#endif
	if (InArgs.Prog != NULL)
	{
		pclose(InArgs.Prog);
		InArgs.Prog = NULL;
	}
#if HAVE_LIBNVIDIA_ML
	static_assert(false,"Nvidia ML has been temporarily disabled");
	//nvmlShutdown();
#endif
	if (InArgs.File != NULL) fclose(InArgs.File);
	if (InArgs.Temp != NULL) fclose(InArgs.Temp);
	sensors_cleanup();
	endwin();

	if (InArgs.UseUI)
	{
		if (!WriteConfig(&UI,InArgs)) return -1;
	}

	return 0;
}

/****************************************************************
ProcessArgs:
	Takes:
		argc: number of command line arguments passed
		argv: text passed from command line
	Returns:
		1 if arguments were successfully processed
		0 if an error occurred while processing arguments

	This function sets global variables based on input from
		the command line.  
****************************************************************/
InputArguments ProcessArgs(int argc, char** argv)
{
	InputArguments InArgs;
	InArgs.Success = true;
	for (int i = 1; i < argc; i++)
	{
		if (strcmp(argv[i],"-p") == 0) {InArgs.File = fopen(argv[i+1],"r"); i++;}
		else if (strcmp(argv[i],"-h") == 0) {std::cout << helptext; InArgs.run = 0;}
		else if (strcmp(argv[i],"-w") == 0) {InArgs.TimeStep = (stoi(argv[i+1])*1000000); i++;}
		else if (strcmp(argv[i],"-i") == 0) {InArgs.run = 0; InArgs.PrtTmp = 1;}
		else if (strcmp(argv[i],"-f") == 0) 
		{
			InArgs.Temp = fopen(argv[i+1],"r"); 
			i++;
			if (InArgs.Temp == NULL || !ParseTemp(InArgs)) 
			{
				std::cerr << "Could not load temperatures: " << strerror(errno) << "\n";
				InArgs.Success = false;
			}
		}
		else if (strcmp(argv[i],"-v") == 0) InArgs.PrtTmp = 1;
		else if (strcmp(argv[i],"-C") == 0) {InArgs.Command = (string)argv[i+1]; i++;}
		else if (strcmp(argv[i],"-s") == 0) InArgs.Stats = 1;
		else if (strcmp(argv[i],"-UI") == 0) InArgs.UseUI = 1;
		else if (strcmp(argv[i],"--use-gtk") == 0) InArgs.UseGUI = 1;
		else if (argv[i][0] == '-')
		{
			for (unsigned j = 1; j != string(argv[i]).length(); j++) 
			{
				if (argv[i][j] == 'p') {InArgs.File = fopen(argv[i+1],"r"); i++;}
				else if (argv[i][j] == 'h') {std::cout << helptext; InArgs.run = 0;}
				else if (argv[i][j] == 'w') {InArgs.TimeStep = (stoi(argv[i+1])*1000000); i++;}
				else if (argv[i][j] == 'i') {InArgs.run = 0; InArgs.PrtTmp = 1;}
				else if (strcmp(argv[i],"f") == 0) 
				{
					InArgs.Temp = fopen(argv[i+1],"r"); 
					i++;
					if (InArgs.Temp == NULL || !ParseTemp(InArgs)) 
					{
						std::cerr << "Could not load temperatures: " << strerror(errno) << "\n";
						InArgs.Success = false;
					}
				}
				else if (argv[i][j] == 'v') InArgs.PrtTmp = 1;
				else if (argv[i][j] == 'C') {InArgs.Command = (string)argv[i+1]; i++;}
				else if (argv[i][j] == 's') InArgs.Stats = 1;
				else InArgs.Success = false;
			}
		}
		else { InArgs.Success = false; }
	}
	return InArgs;
};

/****************************************************************
ParseTemp:
	Returns:
		1 upon successful reading of Critical Temperatures file

	This function reads Critical Temperatures from a user-
		specified file and loads them into the MaxTemps
		global variable.
****************************************************************/
bool ParseTemp(InputArguments &InArgs)
{
	rewind(InArgs.Temp);
	int IntBuffer;
	string StrBuffer;
	while (!feof(InArgs.Temp))
	{
		InArgs.MaxTemps.resize(InArgs.MaxTemps.size()+1);
		StrBuffer = "";
		IntBuffer = fgetc(InArgs.Temp);
		while (IntBuffer != ',' && IntBuffer != EOF)
		{
			StrBuffer += (char)IntBuffer;
			IntBuffer = fgetc(InArgs.Temp);
		}
		InArgs.MaxTemps.at(InArgs.MaxTemps.size()-1) = stof(StrBuffer.c_str());
	}
	InArgs.MinTemp = InArgs.MaxTemps[0];
	for (unsigned i = 1; i != InArgs.MaxTemps.size(); i++)
	{
		if (InArgs.MaxTemps[i] < InArgs.MinTemp) InArgs.MinTemp = InArgs.MaxTemps[i];
	}
	return 1;
};

/****************************************************************
ProcessTemp:
	Takes:
		index: the sensor number corresponding to 'value'
		value: the temperature reading from the sensor
	Returns:
		0 if the sensor temperature threshold is exceeded
		1 otherwise

	This program takes the sensor temperature reading and
		runs the user-specified program if the critical 
		temperature is exceeded.  If insufficient temperature
		data is supplied at program start, this will compare 
		the temperature 'value' to the minimum of MaxTemps.
****************************************************************/
bool ProcessTemp(int index,double value, InputArguments &InArgs)
{
	if (InArgs.Prog != NULL)
	{
		pclose(InArgs.Prog);
		InArgs.Prog = NULL;
	}
	if (InArgs.MaxTemps.size() == 0) return 1;
	if ((unsigned)index < InArgs.MaxTemps.size())
	{
		if (value >= InArgs.MaxTemps[index])
		{
			if (InArgs.PrtTmp) std::cout << "Maximum temperature exceeded by sensor " << index << "\n";
			if (InArgs.Command.length() > 0) InArgs.Prog = popen(InArgs.Command.c_str(),"r");
			return 0;
		}
		else return 1;
	}
	else
	{
		if (value >= InArgs.MinTemp)
		{
			if (InArgs.PrtTmp) std::cout << "Maximum temperature exceeded by sensor " << index << "\n";
			if (InArgs.Command.length() > 0) InArgs.Prog = popen(InArgs.Command.c_str(),"r");
			return 0;
		}
		else return 1;
	}
};

/****************************************************************
avg:
	Takes:
		x1: some value
		x2: some other value
	Returns:
		The mean value of x1 and x2
****************************************************************/
double avg(double x1, double x2)
{
	return (x1+x2)/2;
};

#include <sys/types.h>
#include <pwd.h>
/****************************************************************
SetHomeDirectory:
	Reads user information and attempts to determine where
		the home directory is.
	
	Used for locating config file.
****************************************************************/
void SetHomeDirectory(InputArguments &InArgs)
{
	//attempt to figure out home directory from environment variable
	InArgs.HomeDir = string(getenv("HOME"));
	//otherwise, get current directory
	if (InArgs.HomeDir == "") 
	{
		InArgs.HomeDir = string(getpwuid(getuid())->pw_dir);
	}
};

/****************************************************************
ReadConfig:
	Takes:
		*UI: Pointer to a User Interface object
		NumSensors: Number of Sensors
	Returns:
		0 if the config file at ~/.config/TempSafe.cfg could
		not be read
		1 if the config file is successfully read and
		loaded into the appropriate variables

	Since this is run only after curses mode is initialized,
		the errors printed to stderr are not likely to
		be visible to the user.
****************************************************************/
bool ReadConfig(UserInterface* UI, unsigned int NumSensors, InputArguments &InArgs)
{
	FILE *f;
	string ConfFile = InArgs.HomeDir + "/.config/TempSafe.cfg";
	f = fopen(ConfFile.c_str(),"r");
	if (f == NULL) return 0;
	vector<int> CritTemps;
	vector<unsigned int> Colours;
	vector<string> Commands;

	int BUFFER;
	string StrBuffer;
	BUFFER = fgetc(f);

	while (BUFFER != EOF)
	{
		for (int i = 0; i < 3; i++)
		{
			StrBuffer = "";
			while (BUFFER != ',' && BUFFER != '\n' && BUFFER != EOF)
			{
				StrBuffer += (char) BUFFER;
				BUFFER = fgetc(f);
				if (BUFFER == '.' && i < 2) 
				{
					std::cerr << "ERROR: inputs in TempSafe.cfg cannot be decimal numbers.\n";
					fclose(f);
					return 0;
				}
			}
			switch (i)
			{
				case (0): CritTemps.push_back(stoi(StrBuffer)); break;
				case (1): Colours.push_back(stoi(StrBuffer)); break;
				case (2): Commands.push_back(StrBuffer); break;
				default: break;
			}
			if (BUFFER != EOF) BUFFER = fgetc(f);
		}
	}

	if (CritTemps.size() != Colours.size() || Colours.size() != Commands.size() || Commands.size() != CritTemps.size())
	{
		std::cerr << "ERROR: TempSafe.cfg has incomplete lines.\n";
		fclose(f);
		return 0;
	}
	if (CritTemps.size() > NumSensors)
	{
		std::cerr << "ERROR: TempSafe.cfg has too many config lines.\n";
		fclose(f);
		return 0;
	}
	else if (CritTemps.size() < NumSensors)
	{
		std::cerr << "ERROR: TempSafe.cfg has too few config lines.\n";
		fclose(f);
		return 0;
	}
	
	if (!UI->SetupValues(CritTemps,Colours,Commands))
	{
		std::cerr << "ERROR: Failed to load TempSafe.cfg into vectors.\n";
		fclose(f);
		return 0;
	}
	fclose(f);
	return 1;
};

/****************************************************************
WriteConfig
	Takes:
		UI: Pointer to User Interface
	Returns:
		1 upon successful updating of config files

	This function checks the text in the TempSafe.cfg file
		and if the data has changed, writes the updated
		information.
****************************************************************/
bool WriteConfig(UserInterface* UI, InputArguments &InArgs)
{
	FILE *conf = NULL;
	string ConfLoc = InArgs.HomeDir + "/.config/TempSafe.cfg";
	int BUFFER;
	string ConfData = "";

	string TextToPrint = "";

	vector<int> CritTemps = UI->GetCritTemps();
	vector<string> Commands = UI->GetCommands();
	vector<unsigned int> SensColours = UI->GetSensorColours();

	for (unsigned i = 0; i != CritTemps.size(); i++)
	{
		char BufferString[512]; //anything this long should be a shell script instead
		sprintf(BufferString,"%d,%d,%s\n",CritTemps[i],SensColours[i],Commands[i].c_str());
		TextToPrint += string(BufferString);
	}

	conf = fopen(ConfLoc.c_str(),"r");
	if (conf != NULL)
	{
		BUFFER = fgetc(conf);
		while (BUFFER != EOF)
		{
			ConfData += (char)BUFFER;
			BUFFER = fgetc(conf);
		}
		fclose(conf);
		if (strcmp(ConfData.c_str(),TextToPrint.c_str()) == 0) 
		{
			return 1;
		}
	}

	std::cerr << "Configuration changed.  Writing changes...\n";
	remove(ConfLoc.c_str());
	conf = fopen(ConfLoc.c_str(),"w+");
	if (conf == NULL)
	{
		std::cerr << "ERROR: Could not open " << ConfLoc.c_str() << "for writing.\n";
	}
	else
	{
		fprintf(conf,"%s",TextToPrint.c_str());
		fclose(conf);
		return 1;
	}
	return 0;
}

/*
GUI Config binary file format:
	GUI Config file starts with a 13-character signature
	detailing the file version number.
	File Format Heirarchy:
		(int)# of sensors
		<for all sensors>:
		|(char*) null-terminated sensor name
		|(unsigned int) colour
		|(float) critical temperature
		|(char) sensor active (converts to bool)
		|(char*) null-terminated command
*/

/****************************************************************
SaveGUIConfig
	Takes:
		GUI Data Handler for the current context
	Returns:
		1 upon successful updating of config files

	This function checks the text in the TempSafe_GUI.cfg file
		and if the data has changed, writes the updated
		information.
****************************************************************/
bool SaveGUIConfig(GUI::GUIDataHandler *Handle)
{
	FILE *f;
	string ConfFile = "TempSafe_GUI.cfg";
	f = fopen(ConfFile.c_str(),"r");
	if (f != NULL)
	{
		fclose(f);
		remove(ConfFile.c_str());
	}

	std::ofstream f_out(ConfFile.c_str(),std::ifstream::binary);

	/*Read signature: {SafeTemp_0.3}*/
	char Signature[13] = "SafeTemp_0.3";
	f_out.write(Signature,sizeof(Signature));

	int NumSensors = Handle->SensorNames.size();
	f_out.write((char*)&NumSensors,sizeof(int));
	for (unsigned i = 0; i != Handle->SensorNames.size(); i++)
	{
		for (unsigned j = 0; j != Handle->SensorNames[i].length(); j++)
		{
			f_out.write((char*)&Handle->SensorNames[i][j],sizeof(char));
		}
		char NullChar = '\0';
		f_out.write(&NullChar,sizeof(char));
		unsigned int TMP_Colour = Handle->SensorColours[i];
		float TMP_Critical = Handle->SensorCriticals[i];
		char TMP_Active = (char)Handle->SensorActive[i];
		f_out.write((char*)&TMP_Colour,sizeof(unsigned int));
		f_out.write((char*)&TMP_Critical,sizeof(float));
		f_out.write((char*)&TMP_Active,sizeof(char));
		for (unsigned k = 0; k < Handle->SensorCommands[i].length(); k++)
		{
			f_out.write((char*)&Handle->SensorCommands[i][k],sizeof(char));
		}
		f_out.write(&NullChar,sizeof(char));
	}
	f_out.close();
	return 1;
};

/****************************************************************
ReadGUIConfig
	Takes:
		GUI Data Handler for the current context
	Returns:
		1 upon successful reading of config file

	This function reads information from the binary-formatted
		TempSafe_GUI.cfg and loads the configuration information
		for use.  
****************************************************************/
bool ReadGUIConfig(GUI::GUIDataHandler *Handle)
{
	FILE *f;
	string ConfFile = "TempSafe_GUI.cfg";
	f = fopen(ConfFile.c_str(),"r");
	if (f == NULL) return 0;
	else fclose(f);

	std::ifstream f_in(ConfFile.c_str(),std::ifstream::binary);

	/*Read signature: {SafeTemp_0.3}*/
	char Signature[13];
	f_in.read(Signature,sizeof(Signature));

	int NumSensors = 0;
	Handle->Harmonize();

	if (strcmp(Signature,"SafeTemp_0.3") == 0)
	{
		f_in.read((char*)&NumSensors,sizeof(int));
		if (Handle->SensorNames.size() != NumSensors)
			return 0;
		for (int i = 0; i < NumSensors; i++)
		{
			char BUFFER;
			f_in.read(&BUFFER,sizeof(char));
			while (BUFFER != '\0' && !f_in.eof())
			{
				/*Basically dump this buffer (the sensor name is just a placeholder in the file*/
				f_in.read(&BUFFER,sizeof(char));
			}
			unsigned int TMP_Colour;
			float TMP_Critical;
			char TMP_Active;
			f_in.read((char*)&TMP_Colour,sizeof(unsigned int));
			f_in.read((char*)&TMP_Critical,sizeof(float));
			f_in.read((char*)&TMP_Active,sizeof(char));
			Handle->SensorColours[i] = TMP_Colour;
			Handle->SensorCriticals[i] = TMP_Critical;
			Handle->SensorActive[i] = (bool)TMP_Active;

			f_in.read(&BUFFER,sizeof(char));
			while (BUFFER != '\0' && !f_in.eof())
			{
				Handle->SensorCommands[i] += BUFFER;
				f_in.read(&BUFFER,sizeof(char));
			}
		}
	}
	else
	{
		std::cerr << "[Load config]: " << ConfFile.c_str() << ": unrecognized file format " << Signature << "\n";
		return 0;
	}
	f_in.close();
	return 1;
};
