/****************************************************************
TempSafe Program
	Note that the names TempSafe and SafeTemp are used 
		interchangeably in the documentation.

KNOWN BUGS:
	-In the User Interface, backspace does not work when 
		editing "commands"
	-In the User Interface, the cursor position is not steady
	-The Estimated Maximum Temperatures make no sense
****************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string>
#include <cstring>
#include <vector>
#include <cmath>
#include <config.h>
#if HAVE_LIBSENSORS
#include <sensors/sensors.h>
#include <sensors/error.h>
#endif
#if HAVE_LIBNVIDIA_ML
#include <NVCtrl/NVCtrl.h>
#include <nvml.h>
#endif

#include "UserInterface/Manager.h"
#include "UserInterface/Graph.h"
#include "UserInterface/UI.h"
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
int TimeStep = 5000000;
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

string helptext = "tempsafe -p FILE -w TIME -i -v -f FILE -C SCRIPT \nsensors-checking program\nKevin Brooks, 2015\nUsage: \n-p\t Path to lm-sensors config file\n-w\t time interval to wait between checks (seconds); default is 5 seconds\n-s\t Calculate statistics (when possible) and give information as to when the max temperature will be reached, and what that maximum temperature might be\n\t\t NOTE: this is VERY rough and shouldn't be trusted for anything critical.\n-f\t Load temperatures from a file\n-i\t Don't run, just print temperatures and exit (implies -v)\n-v\t Verbose output (print temperatures at each TIME interval)\n-C\t execute a shell script;\n-UI\t EXPERIMENTAL: Start with User Interface (overrides -v, -c, -f, and -s)\n\t\tUser Interface reads a config file from /etc/TempSafe.cfg \n\t\t SCRIPT path should be given in double-quotes.\n-h\t Print this help file\n\n";

bool ProcessArgs(int, char**);
bool ParseTemp();
bool ProcessTemp(int,double);
double deriv(double,double);
double avg(double, double);
double EstMaxTemp(vector<double>, vector<double>, vector<double>, vector<time_t>);
double EstMaxTime(vector<double>, vector<double>, vector<double>, vector<time_t>);
bool ReadConfig(UserInterface*, unsigned int);
bool WriteConfig(UserInterface*);
void SetHomeDirectory();

int main(int argc,char** argv)
{
    /* Check for the presence of libsensors package */
    if (HAVE_LIBSENSORS != 1) 
    {
        printf("WARNING: lm_sensors must be installed for this program to work.\n\tPlease install lm_sensors and re-configure this package.\n\n");
        return -1;
    }
    
    /* Process command line arguments */
    if (!ProcessArgs(argc,argv))
    {
        fprintf(stderr,"Failed to parse input.\n");
        printf(helptext.c_str());
        return -3;
    }

    /* Statistics variables */
    vector<vector<time_t>> X_pts;
    vector<vector<double>> Y_pts;
    vector<vector<double>> Yp_pts;
    vector<vector<double>> Ypp_pts;

    /* Initialize NVidia devices if installed */
#if HAVE_LIBNVIDIA_ML
    bool nv = true;
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
    }
#endif

    int ErrorNum;
    int ChipNo;
    int FeatNo;
    ChipNo = 0;
    bool running = 1;

    sensors_chip_name const * Chip;
    sensors_feature const * feat;
    sensors_subfeature const * subfeat;

    vector<sensors_chip_name const*> ChipNames;
    vector<sensors_feature const*> ChipFeats;
    vector<sensors_subfeature const*> SubFeats;

    /* Initialize libsensors and build an index of available temperature sensors */
    ErrorNum = sensors_init(File);
    if (ErrorNum != 0)
    {
        fprintf(stderr,"An error has occurred on initialization: %s\n",sensors_strerror(ErrorNum));
        return -1;
    }
    while ((Chip = sensors_get_detected_chips(NULL,&ChipNo)) != 0)
    {
        FeatNo = 0;
        while ((feat = sensors_get_features(Chip,&FeatNo)) != 0)
        {
            if (feat->type == SENSORS_FEATURE_TEMP)
            {
                subfeat = sensors_get_subfeature(Chip,feat,SENSORS_SUBFEATURE_TEMP_INPUT);
                ChipNames.push_back(Chip);
                ChipFeats.push_back(feat);
                SubFeats.push_back(subfeat);
            }
        }
    }
    if (ChipNames.size() == 0 || ChipFeats.size() == 0 || SubFeats.size() == 0)
    {
        fprintf(stderr,"Could not find sensors\n");
        return -2;
    }

    /* Initialize User Interface */
    SetHomeDirectory();
    vector<string> SensorNames;
    for (int i = 0; i < ChipNames.size(); i++)
    {
        char* SensorLabel = sensors_get_label(ChipNames[i],ChipFeats[i]);
        SensorNames.resize(i+1);
        SensorNames[i] = string(SensorLabel);
        free(SensorLabel);
    }
    if (nv)
    {
        for (int i = 0; i < nvDev.size(); i++) SensorNames.push_back("NVidia Device");
    }
    Manager WM;
    int WM_Graph = WM.AddWin(0,0,WM.MAIN->_maxx,(int)((float)WM.MAIN->_maxy/2.0)-2,1,0);
    int WM_Data = WM.AddWin(0,(int)((float)WM.MAIN->_maxy/2.0)+2,WM.MAIN->_maxx,(int)((float)WM.MAIN->_maxy/2.0)-1,0,1);
    Graph GP(&WM.Wins[WM_Graph],0,0,10,40,{9,2,WM.MAIN->_maxx-12,(int)((float)WM.MAIN->_maxy/2.0)-7});
    for (int i = 0; i < ChipNames.size()+nvDev.size(); i++)
    {
        GP.InitDataset();
    }
    UserInterface UI(&WM.Wins[WM_Data],&GP);
    if (SensorNames.size() != MaxTemps.size())
    {
        for (int i = 0; i < SensorNames.size(); i++)
        {
            MaxTemps.push_back(0);
        }
    }
    if (!UI.SetupValues(SensorNames.size(), SensorNames, MaxTemps) && UseUI)
    {
        endwin();
        fprintf(stderr,"ERROR: User Interface could not be configured.  Exiting...\n");
        fprintf(stderr,"\tNumberOfSensors: %d\n\tNumber of Sensor Names: %d\n\tNumber of Critical Temperatures: %d\n",SensorNames.size(),SensorNames.size(),MaxTemps.size());
        return -1;
    }
    if (UseUI)
    {
        if (!ReadConfig(&UI,SensorNames.size()))
        {
            endwin();
            fprintf(stderr,"Error loading config file ~/.config/TempSafe.cfg.  Exiting...\n");
        }
    }
    int CharBuffer = 0;
    if (!UseUI) endwin();
    if (UseUI) nodelay(stdscr,1);
    
    /* Main Events Loop */
    if (ChipNames.size() == ChipFeats.size() && ChipFeats.size() == SubFeats.size())
    {
        double val;
#if HAVE_NVIDIA_ML
            double MaxTemp[ChipNames.size()+nvDev.size()];
            double MaxTime[ChipNames.size()+nvDev.size()];
#else
            double MaxTemp[ChipNames.size()];
            double MaxTime[ChipNames.size()];
#endif
        if (Stats)
        {
            X_pts.resize(ChipNames.size());
            Y_pts.resize(ChipNames.size());
            Yp_pts.resize(ChipNames.size());
            Ypp_pts.resize(ChipNames.size());
        }

        while (true)
        {
            /* Loop through all available sensors and perform relevant actions */
            for (int i = 0; i < ChipNames.size(); i++)
            {
                sensors_get_value(ChipNames[i],SubFeats[i]->number,&val);
                if (Stats && !UseUI) X_pts[i].push_back(time(NULL));
                if (Stats && !UseUI) Y_pts[i].push_back(val);
                if (Stats && Y_pts[i].size() >= 2 && !UseUI) Yp_pts[i].push_back(deriv(Y_pts[i][Y_pts.size()-2],Y_pts[i][Y_pts[i].size()-1]));
                if (Stats && Yp_pts[i].size() >= 2 && !UseUI) Ypp_pts[i].push_back(deriv(Yp_pts[i][Yp_pts.size()-2],Yp_pts[i][Yp_pts[i].size()-1]));
                if (Stats && !UseUI) MaxTemp[i] = EstMaxTemp(Y_pts[i],Yp_pts[i],Ypp_pts[i],X_pts[i]);
                if (Stats && !UseUI) MaxTime[i] = EstMaxTime(Y_pts[i],Yp_pts[i],Ypp_pts[i],X_pts[i]);
                if (PrtTmp && !UseUI) printf("Sensor %d: %f\n",i,val);
                if (PrtTmp && Stats && Ypp_pts[i].size() >= 1 && !UseUI) printf("Estimated max temperature is %f in %f seconds for sensor %d\n",MaxTemp[i],MaxTime[i],i);
                if (!UseUI) ProcessTemp(i,val);
                if (UseUI) UI.AppendSensorData(i,val,TimeStep/1000000);
            }
#if HAVE_LIBNVIDIA_ML
            if (nv) //NVIDIA GPU data
            {
                /*Loop through all nvidia chips and perform relevant actions */
                for (int i = 0; i < nvDev.size(); i++)
                {
                    if (nvmlDeviceGetTemperature(nvDev[i],NVML_TEMPERATURE_GPU,&nvTempTmp) != NVML_SUCCESS) fprintf(stderr,"Failed to read temperature from NVIDIA chip (%d)\n",ChipNames.size());
                    val = (double)nvTempTmp;
                    //NV_CTRL_THERMAL_SENSOR_READING;
                    if (Stats && !UseUI) X_pts[ChipNames.size() + i].push_back(time(NULL));
                    if (Stats && !UseUI) Y_pts[ChipNames.size() + i].push_back(val);
                    if (Stats && Y_pts[ChipNames.size() + i].size() >= 2 && !UseUI) Yp_pts[ChipNames.size() + i].push_back(deriv(Y_pts[ChipNames.size() + i][Y_pts.size()-2],Y_pts[ChipNames.size() + i][Y_pts[ChipNames.size() + i].size()-1]));
                    if (Stats && Yp_pts[ChipNames.size() + i].size() >= 2 && !UseUI) Ypp_pts[ChipNames.size() + i].push_back(deriv(Yp_pts[ChipNames.size() + i][Yp_pts.size()-2],Yp_pts[ChipNames.size() + i][Yp_pts[ChipNames.size() + i].size()-1]));
                    if (Stats && !UseUI) MaxTemp[ChipNames.size() + i] = EstMaxTemp(Y_pts[ChipNames.size() + i],Yp_pts[ChipNames.size() + i],Ypp_pts[ChipNames.size() + i],X_pts[ChipNames.size() + i]);
                    if (Stats && !UseUI) MaxTime[ChipNames.size() + i] = EstMaxTime(Y_pts[ChipNames.size() + i],Yp_pts[ChipNames.size() + i],Ypp_pts[ChipNames.size() + i],X_pts[ChipNames.size() + i]);
                    if (PrtTmp && !UseUI) printf("Sensor %d: %f\n",ChipNames.size() + i,val);
                    if (PrtTmp && Stats && Ypp_pts[ChipNames.size() + i].size() >= 1 && !UseUI) printf("Estimated max temperature is %f in %f seconds for sensor %d\n",MaxTemp[ChipNames.size() + i],MaxTime[ChipNames.size() + i],ChipNames.size() + i);
                    if (!UseUI) ProcessTemp(ChipNames.size() + i,val);
                    if (UseUI) UI.AppendSensorData(ChipNames.size()+i,val,TimeStep/1000000);
                }
            }
#endif
            if (PrtTmp && !UseUI) printf("Finished Line\n");
            if (!run) break;
            
            /* If User Interface is enabled, perform the required actions to process data */
            if (UseUI)
            {
                UI.UpdateTimer(TimeStep/1000000);
                CharBuffer = 0;
                CharBuffer = getch();

                if (CharBuffer == KEY_UP) UI.MoveCurs(4);
                if (CharBuffer == KEY_DOWN) UI.MoveCurs(2);
                if (CharBuffer == KEY_RIGHT) UI.MoveCurs(1);
                if (CharBuffer == KEY_LEFT) UI.MoveCurs(3);
                if (CharBuffer == '+') UI.TempAdjust(0);
                if (CharBuffer == '-') UI.TempAdjust(1);
                if (CharBuffer == KEY_ENTER || CharBuffer == '\n' || CharBuffer == '\r') UI.GetCommand();
                if (CharBuffer == 'q') run = 0;


                WM.ClearBuffers();
                GP.DrawToWindow();
                WM.Wins[WM_Graph].DrawAlignedText(ALIGN_BOTTOM_CENTER,"Time Since Program Start (seconds)",7,0);
                WM.Wins[WM_Graph].DrawAlignedText(ALIGN__LEFT,"Temperature C",7,0);
                WM.Wins[WM_Graph].DrawAlignedText(ALIGN_TOP_CENTER,"Temperature vs. Time",7,0);
                WM.Wins[WM_Data].DrawAlignedText(ALIGN_TOP_CENTER,"-Main Menu- (tap q to exit)",0,7,A_BOLD & A_STANDOUT);
                UI.draw();
                move(WM.MAIN->_maxy,0);
                WM.DrawWindows();
            }

            if (!UseUI) usleep((int)(TimeStep));
        }
    }
    else 
    {
        fprintf(stderr,"Vector sizes uneven\n");
        return -1;
    }
    /* Clean up on exit */
    if (Prog != NULL)
    {
        pclose(Prog);
        Prog = NULL;
    }
#if HAVE_LIBNVIDIA_ML
    nvmlShutdown();
#endif
    ChipNames.clear();
    ChipFeats.clear();
    SubFeats.clear();
    if (File != NULL) fclose(File);
    if (Temp != NULL) fclose(Temp);
    sensors_cleanup();
    endwin();

    if (UseUI)
    {
        if (!WriteConfig(&UI)) return -1;
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
bool ProcessArgs(int argc, char** argv)
{
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i],"-p") == 0) {File = fopen(argv[i+1],"r"); i++;}
        else if (strcmp(argv[i],"-h") == 0) {printf(helptext.c_str()); run = 0;}
        else if (strcmp(argv[i],"-w") == 0) {TimeStep = (stoi(argv[i+1])*1000000); i++;}
        else if (strcmp(argv[i],"-i") == 0) {run = 0; PrtTmp = 1;}
        else if (strcmp(argv[i],"-f") == 0) 
        {
            Temp = fopen(argv[i+1],"r"); 
            i++;
            if (Temp == NULL || !ParseTemp()) 
            {
                fprintf(stderr,"Could not load temperatures: %s\n",strerror(errno));
                return 0;
            }
        }
        else if (strcmp(argv[i],"-v") == 0) PrtTmp = 1;
        else if (strcmp(argv[i],"-C") == 0) {Command = (string)argv[i+1]; i++;}
        else if (strcmp(argv[i],"-s") == 0) Stats = 1;
        else if (strcmp(argv[i],"-UI") == 0) UseUI = 1;
        else if (argv[i][0] == '-')
        {
            for (int j = 1; j < string(argv[i]).length(); j++) 
            {
                if (argv[i][j] == 'p') {File = fopen(argv[i+1],"r"); i++;}
                else if (argv[i][j] == 'h') {printf(helptext.c_str()); run = 0;}
                else if (argv[i][j] == 'w') {TimeStep = (stoi(argv[i+1])*1000000); i++;}
                else if (argv[i][j] == 'i') {run = 0; PrtTmp = 1;}
                else if (strcmp(argv[i],"f") == 0) 
                {
                    Temp = fopen(argv[i+1],"r"); 
                    i++;
                    if (Temp == NULL || !ParseTemp()) 
                    {
                        fprintf(stderr,"Could not load temperatures: %s\n",strerror(errno));
                        return 0;
                    }
                }
                else if (argv[i][j] == 'v') PrtTmp = 1;
                else if (argv[i][j] == 'C') {Command = (string)argv[i+1]; i++;}
                else if (argv[i][j] == 's') Stats = 1;
                else return 0;
            }
        }
        else return 0;
    }
    return 1;
};

/****************************************************************
ParseTemp:
	Returns:
	    1 upon successful reading of Critical Temperatures file

	This function reads Critical Temperatures from a user-
	    specified file and loads them into the MaxTemps
	    global variable.
****************************************************************/
bool ParseTemp()
{
    rewind(Temp);
    int IntBuffer;
    string StrBuffer;
    while (!feof(Temp))
    {
        MaxTemps.resize(MaxTemps.size()+1);
        StrBuffer = "";
        IntBuffer = fgetc(Temp);
        while (IntBuffer != ',' && IntBuffer != EOF)
        {
            StrBuffer += (char)IntBuffer;
            IntBuffer = fgetc(Temp);
        }
        MaxTemps.at(MaxTemps.size()-1) = stof(StrBuffer.c_str());
    }
    MinTemp = MaxTemps[0];
    for (int i = 1; i < MaxTemps.size(); i++)
    {
        if (MaxTemps[i] < MinTemp) MinTemp = MaxTemps[i];
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
bool ProcessTemp(int index,double value)
{
    if (Prog != NULL)
    {
        pclose(Prog);
        Prog = NULL;
    }
    if (MaxTemps.size() == 0) return 1;
    if (index <= MaxTemps.size()-1)
    {
        if (value >= MaxTemps[index])
        {
            if (PrtTmp) printf("Maximum temperature exceeded by sensor %d!\n",index);
            if (Command.length() > 0) Prog = popen(Command.c_str(),"r");
            return 0;
        }
        else return 1;
    }
    else
    {
        if (value >= MinTemp)
        {
            if (PrtTmp) printf("Maximum temperature exceeded by sensor %d!\n",index);
            if (Command.length() > 0) Prog = popen(Command.c_str(),"r");
            return 0;
        }
        else return 1;
    }
};

/****************************************************************
deriv:
	Takes:
	    y1: temperature value preceeding y2 from sensor
	    y2: temperature value from sensor
	Returns:
	    Time derivative of temperature between y1 and y2 over
		a TimeStep.
****************************************************************/
double deriv(double y1, double y2)
{
    if (TimeStep != 0)
    {
        return (y2 - y1)/((float)TimeStep/1000000.0);
    }
    else return 0;
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

/****************************************************************
EstMaxTemp:
	Takes:
	    y: vector of temperatures
	    yp: vector of temperature time derivatives
	    ypp: vector of temperature second time derivatives
	    t: values of time
	Returns:
	    An estimate of the maximum temperature that will be
		reached by the sensor whose data is given

	This function estimates the 98% maximum temperature based
		on the assumption of exponential decay with a 
		function T = a + b*e^(c*t).  
	The function calculates an exponential curve that fits
		three of the most recent points.  This is very
		open to errors based on noise and should not
		be trusted 100%.  
	I am thinking of instead performing some statistical
		analysis of the data to get a best-fit exponential
		curve, or perhaps removing this function outright.
****************************************************************/
double EstMaxTemp(vector<double> y, vector<double> yp, vector<double> ypp, vector<time_t> t)
{
    if (y.size() < 3 || yp.size() < 2 || ypp.size() < 1 || t.size() < 3) return 0;
    if (ypp[ypp.size()-1] == 0 || avg(yp[yp.size()-2],yp[yp.size()-1]) == 0) return  y[y.size()-1]; //suggests maximum

    double x = (float)t[t.size()-2] - (float)StartTime;
    double c = ypp[ypp.size()-1]/avg(yp[yp.size()-2],yp[yp.size()-1]);
    if (c == 0) {return y[y.size()-1];}
    double b = avg(yp[yp.size()-2],yp[yp.size()-1])/(c*exp(c*x));
    if (b == 0) {return y[y.size()-1];}
    double a = y[y.size()-2] - b*exp(c*x);

    //Tmax occurs as t -> inf, so if c is negative, Tmax -> a.  If c = 0 or b = 0: we're likely at maximum
    if (c < 0) return a;
    else return y[y.size()-1]; //we're likely just at startup if this happens; result is unreliable
};

/****************************************************************
EstMaxTime
	Takes:
	    y: vector of temperatures
	    yp: vector of temperature time derivatives
	    ypp: vector of temperature second time derivatives
	    t: values of time
	Returns:
	    An estimate of the time at which 98% maximum
		temperature will be reached.

	See comments for EstMaxTemp for more information.
****************************************************************/
double EstMaxTime(vector<double> y, vector<double> yp, vector<double> ypp, vector<time_t> t)
{
    if (y.size() < 3 || yp.size() < 2 || ypp.size() < 1 || t.size() < 3) return 0;
    if (ypp[ypp.size()-1] == 0 || avg(yp[yp.size()-2],yp[yp.size()-1]) == 0) return 0; //suggests maximum

    double x = (float)t[t.size()-2] - (float)StartTime;
    double c = ypp[ypp.size()-1]/avg(yp[yp.size()-2],yp[yp.size()-1]);
    if (c == 0) return 0;
    double b = avg(yp[yp.size()-2],yp[yp.size()-1])/(c*exp(c*x));
    if (b == 0) return 0;
    double a = y[y.size()-2] - b*exp(c*x);

    //Tmax occurs as t -> inf, so if c is negative, Tmax -> a.  If c = 0 or b = 0: we're likely at maximum
    if (c > 0) return 0;
    else return -(4.0/c) + (float)x;
};

#include <sys/types.h>
#include <pwd.h>
/****************************************************************
SetHomeDirectory:
	Reads user information and attempts to determine where
		the home directory is.
	
	Used for locating config file.
****************************************************************/
void SetHomeDirectory()
{
    //attempt to figure out home directory from environment variable
    HomeDir = string(getenv("HOME"));
    //otherwise, get current directory
    if (HomeDir == "") 
    {
        HomeDir = string(getpwuid(getuid())->pw_dir);
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
bool ReadConfig(UserInterface* UI, unsigned int NumSensors)
{
    FILE *f;
    string ConfFile = HomeDir + "/.config/TempSafe.cfg";
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
                    fprintf(stderr,"ERROR: inputs in TempSafe.cfg cannot be decimal numbers.\n");
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
        fprintf(stderr,"ERROR: TempSafe.cfg has incomplete lines.\n");
        fclose(f);
        return 0;
    }
    if (CritTemps.size() > NumSensors)
    {
        fprintf(stderr,"ERROR: TempSafe.cfg has too many config lines.\n");
        fclose(f);
        return 0;
    }
    else if (CritTemps.size() < NumSensors)
    {
        fprintf(stderr,"ERROR: TempSafe.cfg has too few config lines.\n");
        fclose(f);
        return 0;
    }
    
    if (!UI->SetupValues(CritTemps,Colours,Commands))
    {
        fprintf(stderr,"ERROR: Failed to load TempSafe.cfg into vectors.\n");
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
bool WriteConfig(UserInterface* UI)
{
    FILE *conf = NULL;
    string ConfLoc = HomeDir + "/.config/TempSafe.cfg";
    int BUFFER;
    string ConfData = "";

    string TextToPrint = "";

    vector<int> CritTemps = UI->GetCritTemps();
    vector<string> Commands = UI->GetCommands();
    vector<unsigned int> SensColours = UI->GetSensorColours();

    for (int i = 0; i < CritTemps.size(); i++)
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

    fprintf(stderr,"Configuration changed.  Writing changes...\n");
    remove(ConfLoc.c_str());
    conf = fopen(ConfLoc.c_str(),"w+");
    if (conf == NULL)
    {
        fprintf(stderr,"ERROR: Could not open %s for writing.\n",ConfLoc.c_str());
    }
    else
    {
        fprintf(conf,"%s",TextToPrint.c_str());
        fclose(conf);
        return 1;
    }
}
