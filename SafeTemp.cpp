#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string>
#include <cstring>
#include <vector>
#include <time.h>
#include <cmath>
#include <sensors/sensors.h>
#include <sensors/error.h>

using namespace std;

/*
Global Variables
*/
int TimeStep = 5000000;
time_t StartTime = time(NULL);
vector<double> MaxTemps;
double MinTemp;
FILE *File = NULL;
FILE *Temp = NULL;
FILE *Prog = NULL;
bool run = 1;
bool PrtTmp = 0;
bool Stats = 0;
string Command = "";

string helptext = "tempsafe -p FILE -w TIME -i -v -f FILE -C SCRIPT \nsensors-checking program\nKevin Brooks, 2015\nUsage: \n-p\t Path to lm-sensors config file\n-w\t time interval to wait between checks (seconds); default is 5 seconds\n-s\t Calculate statistics (when possible) and give information as to when the max temperature will be reached, and what that maximum temperature might be\n\t\t NOTE: this is VERY rough and shouldn't be trusted for anything critical.\n-f\t Load temperatures from a file\n-i\t Don't run, just print temperatures and exit (implies -v)\n-v\t Verbose output (print temperatures at each TIME interval)\n-C\t execute a shell script; \n\t\t SCRIPT path should be given in double-quotes.\n-h\t Print this help file\n\n";

bool ProcessArgs(int, char**);
bool ParseTemp();
bool ProcessTemp(int,double);
double deriv(double,double);
double avg(double, double);
double EstMaxTemp(vector<double>, vector<double>, vector<double>, vector<time_t>);
double EstMaxTime(vector<double>, vector<double>, vector<double>, vector<time_t>);

int main(int argc,char** argv)
{
    if (!ProcessArgs(argc,argv))
    {
        fprintf(stderr,"Failed to parse input.\n");
        printf(helptext.c_str());
        return -3;
    }

    //The following is for the stats option
    vector<vector<time_t>> X_pts;
    vector<vector<double>> Y_pts;
    vector<vector<double>> Yp_pts;
    vector<vector<double>> Ypp_pts;

    int ERR,ChipNo,FeatNo;
    ChipNo = 0;
    bool running = 1;

    sensors_chip_name const * Chip;
    sensors_feature const * feat;
    sensors_subfeature const * subfeat;

    vector<sensors_chip_name const*> ChipNames;
    vector<sensors_feature const*> ChipFeats;
    vector<sensors_subfeature const*> SubFeats;

    ERR = sensors_init(File);
    if (ERR != 0)
    {
        fprintf(stderr,"An error has occurred on initialization: %s\n",sensors_strerror(ERR));
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
    //We should now have knowledge as to what chips and features correspond to what temp sensors.
    if (ChipNames.size() == 0 || ChipFeats.size() == 0 || SubFeats.size() == 0)
    {
        fprintf(stderr,"Could not find sensors\n");
        return -2;
    }
    if (ChipNames.size() == ChipFeats.size() && ChipFeats.size() == SubFeats.size())
    {
        double val;
        
        double MaxTemp[ChipNames.size()];
        double MaxTime[ChipNames.size()];
        if (Stats)
        {
            X_pts.resize(ChipNames.size());
            Y_pts.resize(ChipNames.size());
            Yp_pts.resize(ChipNames.size());
            Ypp_pts.resize(ChipNames.size());
        }

        while (true)
        {
            for (int i = 0; i < ChipNames.size(); i++)
            {
                sensors_get_value(ChipNames[i],SubFeats[i]->number,&val);
                if (Stats) X_pts[i].push_back(time(NULL));
                if (Stats) Y_pts[i].push_back(val);
                if (Stats && Y_pts[i].size() >= 2) Yp_pts[i].push_back(deriv(Y_pts[i][Y_pts.size()-2],Y_pts[i][Y_pts[i].size()-1]));
                if (Stats && Yp_pts[i].size() >= 2) Ypp_pts[i].push_back(deriv(Yp_pts[i][Yp_pts.size()-2],Yp_pts[i][Yp_pts[i].size()-1]));
                if (Stats) MaxTemp[i] = EstMaxTemp(Y_pts[i],Yp_pts[i],Ypp_pts[i],X_pts[i]);
                if (Stats) MaxTime[i] = EstMaxTime(Y_pts[i],Yp_pts[i],Ypp_pts[i],X_pts[i]);
                if (PrtTmp) printf("Sensor %d: %f\n",i,val);
                if (PrtTmp && Stats && Ypp_pts[i].size() >= 1) printf("Estimated max temperature is %f in %f seconds for sensor %d\n",MaxTemp[i],MaxTime[i],i);
                ProcessTemp(i,val); 
                    //TODO: we should have this execute a user-specified command
            }
            if (PrtTmp) printf("Finished Line\n");
            if (!run) break;
            usleep((int)(TimeStep));
        }
    }
    else 
    {
        fprintf(stderr,"Vector sizes uneven\n");
        return -1;
    }
    if (Prog != NULL)
    {
        pclose(Prog);
        Prog = NULL;
    }
    ChipNames.clear();
    ChipFeats.clear();
    SubFeats.clear();
    if (File != NULL) fclose(File);
    if (Temp != NULL) fclose(Temp);
    sensors_cleanup();
    return 0;
}

/*
Process Command Line Arguments
*/
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

/*
Load temperatures from file
*/
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

/*
Process temperature data
	NOTE: our TEMP file should have sufficient data for ALL sensors;
	    if not, we'll just use the minimum value.
*/
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

double deriv(double y1, double y2)
{
    if (TimeStep != 0)
    {
        return (y2 - y1)/((float)TimeStep/1000000.0);
    }
    else return 0;
};

double avg(double x1, double x2)
{
    return (x1+x2)/2;
};

/*
Estimate the 98% temperature estimating an exponential function T = a+b*e^(c*t)
	if time constant is -1/c, then -4/c SHOULD be our equilibrium time.
		(active word: SHOULD)
*/
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
