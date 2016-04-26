#if HAVE_GTK == 1 && HAVE_GNUPLOT == 1
#include <sys/time.h>
namespace GUI
{
    /*
    GUIDataHandler:
        Stores information required for the GUI to operate
        (mostly user settings)
    */
    class GUIDataHandler
    {
        long int StartTime, LastTime;
        struct timeval TV_timer;
        bool Started = 0;
        public:
        std::vector<std::string> SensorNames;
        std::vector<std::string> SensorNames_NoSpace;
        std::vector<std::string> SensorCommands;
        std::vector<unsigned int> SensorColours;
        std::vector<std::vector<float>> SensorData; //1 vector per sensor
        std::vector<std::vector<int>> Times;
        std::vector<float> SensorCriticals; //critical temperatures
        std::vector<bool> SensorActive;

        int NumDataPts = 250;
        int CallInterval;

        GUIDataHandler();
        void Harmonize();
        bool GetTimeTrigger(int);
        void AddData(float,unsigned int);
        void clear();
    };

    /*
    Constructor for GUIDataHandler:
        Starts a time context for timed events
    */
    GUIDataHandler::GUIDataHandler()
    {
        gettimeofday(&TV_timer,NULL);
        StartTime = TV_timer.tv_sec;
        LastTime = StartTime;
    };

    /*
    Harmonize for GUIDataHandler:
        Resizes all vectors to match the number of SensorNames
    */
    void GUIDataHandler::Harmonize()
    {
        SensorCommands.resize(SensorNames.size());
        SensorColours.resize(SensorNames.size());
        SensorData.resize(SensorNames.size());
        Times.resize(SensorNames.size());
        SensorCriticals.resize(SensorNames.size());
        SensorActive.resize(SensorNames.size());
        SensorNames_NoSpace.resize(SensorNames.size());

        for (int i = 0; i < SensorNames.size(); i++)
        {
            std::string TMP = "";
            for (int j = 0; j < SensorNames[i].length(); j++)
            {
                if (SensorNames[i][j] != ' ')
                    TMP += SensorNames[i][j];
            }
            SensorNames_NoSpace[i] = TMP;
        }
    };

    /*
    GetTimeTrigger for GUIDataHandler:
        Determines if the given "interval" has been exceeded
        -Takes: integer time interval (in seconds)
        -Returns: 1 if 'interval' seconds have elapsed since last call
    */
    bool GUIDataHandler::GetTimeTrigger(int interval)
    {
        gettimeofday(&TV_timer,NULL);
        if (TV_timer.tv_sec - LastTime >= interval || (LastTime == StartTime && !Started))
        {
            LastTime = TV_timer.tv_sec;
            Started = 1;
            return 1;
        }
        else
        {
            return 0;
        }
    };

    /*
    AddData for GUIDataHandler
        Adds temperature data for sensor at 'Index'
        -Takes: Temperature Data 'Data', sensor index
    */
    void GUIDataHandler::AddData(float Data, unsigned int Index)
    {
        gettimeofday(&TV_timer,NULL);
        SensorData[Index].push_back(Data);
        Times[Index].push_back(TV_timer.tv_sec - StartTime);
    };

    /*
    clear for GUIDataHandler:
        Deletes all information stored in the object
        (useful while terminating the program)
    */
    void GUIDataHandler::clear()
    {
        SensorNames.clear();
        SensorNames_NoSpace.clear();
        SensorCommands.clear();
        SensorColours.clear();
        SensorData.clear();
        Times.clear();
        SensorCriticals.clear();
        SensorActive.clear();
    };
}
#endif
