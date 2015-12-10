#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string>
#include <cstring>
#include <vector>
#include <sensors/sensors.h>
#include <sensors/error.h>

using namespace std;

/*
Global Variables
*/
int TimeStep = 5000000;
vector<double> MaxTemps;
double MinTemp;
FILE *File = NULL;
FILE *Temp = NULL;
FILE *Prog = NULL;
bool run = 1;
bool PrtTmp = 0;
string Command = "";

string helptext = "sensor-check -p FILE -w TIME -i -v -T FILE -C SCRIPT \nsensors-checking program\nKevin Brooks, 2015\nUsage: \n-p\t Path to lm-sensors config file\n-w\t time interval to wait between checks (seconds); default is 5 seconds\n-T\t Load temperatures from a file\n-i\t Don't run, just print temperatures and exit (implies -v)\n-v\t Verbose output (print temperatures at each TIME interval)\n-C\t execute a shell script; \n\t\t SCRIPT path should be given in double-quotes.\n-h\t Print this help file\n\n";

bool ProcessArgs(int, char**);
bool ParseTemp();
bool ProcessTemp(int,double);

int main(int argc,char** argv)
{
    if (!ProcessArgs(argc,argv))
    {
        fprintf(stderr,"Failed to parse input.\n");
        printf(helptext.c_str());
        return -3;
    }
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
        while (true)
        {
            for (int i = 0; i < ChipNames.size(); i++)
            {
                sensors_get_value(ChipNames[i],SubFeats[i]->number,&val);
                if (PrtTmp) printf("Sensor %d: %f\n",i,val);
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
        else if (strcmp(argv[i],"-T") == 0) 
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
        else if (argv[i][0] == '-')
        {
            for (int j = 1; j < string(argv[i]).length(); j++) 
            {
                if (argv[i][j] == 'p') {File = fopen(argv[i+1],"r"); i++;}
                else if (argv[i][j] == 'h') {printf(helptext.c_str()); run = 0;}
                else if (argv[i][j] == 'w') {TimeStep = (stoi(argv[i+1])*1000000); i++;}
                else if (argv[i][j] == 'i') {run = 0; PrtTmp = 1;}
                else if (strcmp(argv[i],"T") == 0) 
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
