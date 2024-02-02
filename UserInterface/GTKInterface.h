/**************************************************************
Graphical User Interface
    KNOWN ISSUES:
        -We do not restrict the graph data range.  As a result,
            datapoints get progressively squeezed together in 
            the x-axis as time progresses.
        -When loading a save configuration, we depend on the
            sensors being reported in the same order.  Otherwise
            the settings are mixed up.
            (in other words, configurations are not tied to
             a given sensor name)
        -GTK complains about GtkWindow size allocation.  This
            can flood the command line with useless information.
        -There is no debugger implemented as of yet
**************************************************************/

#if HAVE_GTK == 1 && HAVE_GNUPLOT == 1
#include <gtk/gtk.h>
#include "GuiDataHandler.h"

bool SaveGUIConfig(GUI::GUIDataHandler*);
bool ReadGUIConfig(GUI::GUIDataHandler*);

namespace GUI
{
    /*
    SensormLine:
        An set of interface elements for the "databox" information
    */
    const char* SensormLine = "<?xml version='1.0' encoding='UTF-8'?>"
        "<interface>"
        "  <requires lib='gtk+' version='3.20'/>"
        "  <object class='GtkBox' id='DataDisplay'>"
        "    <property name='visible'>True</property>"
        "    <property name='can_focus'>False</property>"
        "    <property name='valign'>start</property>"
        "    <property name='spacing'>10</property>"
        "    <child>"
        "      <object class='GtkLabel' id='lblSensor'>"
        "        <property name='visible'>True</property>"
        "        <property name='can_focus'>False</property>"
        "        <property name='label' translatable='yes'>SensorName</property>"
        "        <property name='justify'>center</property>"
        "        <property name='width_chars'>16</property>"
        "        <property name='max_width_chars'>16</property>"
        "      </object>"
        "      <packing>"
        "        <property name='expand'>False</property>"
        "        <property name='fill'>True</property>"
        "        <property name='position'>0</property>"
        "      </packing>"
        "    </child>"
        "    <child>"
        "      <object class='GtkSeparator'>"
        "        <property name='visible'>True</property>"
        "        <property name='can_focus'>False</property>"
        "      </object>"
        "      <packing>"
        "        <property name='expand'>False</property>"
        "        <property name='fill'>True</property>"
        "        <property name='position'>1</property>"
        "      </packing>"
        "    </child>"
        "    <child>"
        "      <object class='GtkSwitch' id='btnCollectData'>"
        "        <property name='visible'>True</property>"
        "        <property name='can_focus'>True</property>"
        "      </object>"
        "      <packing>"
        "        <property name='expand'>False</property>"
        "        <property name='fill'>True</property>"
        "        <property name='position'>2</property>"
        "      </packing>"
        "    </child>"
        "    <child>"
        "      <object class='GtkBox'>"
        "        <property name='visible'>True</property>"
        "        <property name='can_focus'>False</property>"
        "        <property name='orientation'>vertical</property>"
        "        <child>"
        "          <object class='GtkLabel'>"
        "            <property name='visible'>True</property>"
        "            <property name='can_focus'>False</property>"
        "            <property name='label' translatable='yes'>Temperature:</property>"
        "            <property name='width_chars'>12</property>"
        "          </object>"
        "          <packing>"
        "            <property name='expand'>False</property>"
        "            <property name='fill'>True</property>"
        "            <property name='position'>0</property>"
        "          </packing>"
        "        </child>"
        "        <child>"
        "          <object class='GtkLabel' id='lblTemp'>"
        "            <property name='visible'>True</property>"
        "            <property name='can_focus'>False</property>"
        "            <property name='label' translatable='yes'>(temp)</property>"
        "            <property name='width_chars'>5</property>"
        "            <property name='max_width_chars'>5</property>"
        "          </object>"
        "          <packing>"
        "            <property name='expand'>False</property>"
        "            <property name='fill'>True</property>"
        "            <property name='position'>1</property>"
        "          </packing>"
        "        </child>"
        "      </object>"
        "      <packing>"
        "        <property name='expand'>False</property>"
        "        <property name='fill'>False</property>"
        "        <property name='position'>3</property>"
        "      </packing>"
        "    </child>"
        "    <child>"
        "      <object class='GtkBox'>"
        "        <property name='visible'>True</property>"
        "        <property name='can_focus'>False</property>"
        "        <property name='orientation'>vertical</property>"
        "        <child>"
        "          <object class='GtkLabel'>"
        "            <property name='visible'>True</property>"
        "            <property name='can_focus'>False</property>"
        "            <property name='label' translatable='yes'>Critical Temp:</property>"
        "          </object>"
        "          <packing>"
        "            <property name='expand'>False</property>"
        "            <property name='fill'>True</property>"
        "            <property name='position'>0</property>"
        "          </packing>"
        "        </child>"
        "        <child>"
        "          <object class='GtkEntry' id='txtCritical'>"
        "            <property name='visible'>True</property>"
        "            <property name='can_focus'>True</property>"
        "            <property name='max_length'>6</property>"
        "            <property name='width_chars'>6</property>"
        "            <property name='text' translatable='yes'>0</property>"
        "            <property name='input_purpose'>number</property>"
        "          </object>"
        "          <packing>"
        "            <property name='expand'>False</property>"
        "            <property name='fill'>True</property>"
        "            <property name='position'>1</property>"
        "          </packing>"
        "        </child>"
        "      </object>"
        "      <packing>"
        "        <property name='expand'>False</property>"
        "        <property name='fill'>True</property>"
        "        <property name='position'>4</property>"
        "      </packing>"
        "    </child>"
        "    <child>"
        "      <object class='GtkColorButton' id='btnSensorCol'>"
        "        <property name='visible'>True</property>"
        "        <property name='can_focus'>True</property>"
        "        <property name='receives_default'>True</property>"
        "        <property name='show_editor'>True</property>"
        "      </object>"
        "      <packing>"
        "        <property name='expand'>False</property>"
        "        <property name='fill'>True</property>"
        "        <property name='position'>5</property>"
        "      </packing>"
        "    </child>"
        "    <child>"
        "      <object class='GtkEntry' id='txtCommand'>"
        "        <property name='visible'>True</property>"
        "        <property name='can_focus'>True</property>"
        "         <property name='placeholder_text' translatable='yes'>Command...</property>"
        "      </object>"
        "      <packing>"
        "        <property name='expand'>True</property>"
        "        <property name='fill'>True</property>"
        "        <property name='position'>6</property>"
        "      </packing>"
        "    </child>"
        "  </object>"
        "/<interface>";

    /*
    int2string class: this is a helper class
        ->Stores "object" names and window information
        ->returns an index location or array size when queried
    */
    class int2string
    {
        std::vector<std::string> StringDatabase;
        std::vector<unsigned int> IntDatabase;
        public:
        std::string FName;
        guint Width,Height;
        bool* prun;

        void Add(const char*);
        unsigned int Seek(const char*);
        unsigned int size();
    };

    /*
    Add for int2string:
        Adds a new "object" name to the database
    */
    void int2string::Add(const char* StringToAdd)
    {
        StringDatabase.push_back(std::string(StringToAdd));
        IntDatabase.push_back(StringDatabase.size()-1);
    };

    /*
    Seek for int2string:
        Returns an index location corresponding to an "object" name
    */
    unsigned int int2string::Seek(const char* StringToSeek)
    {
        for (int i = 0; i < IntDatabase.size(); i++)
        {
            if (strcmp(StringDatabase[i].c_str(),StringToSeek) == 0)
                return i;
        }
        return 0;
    };

    /*
    size for int2string:
        Returns the size of the currently held database
    */
    unsigned int int2string::size()
    {
        return IntDatabase.size();
    };

    /*
    GUI Terminate:
        Terminates the GUI program when called
        Takes:
            -Widget (unused), Object data
    */
    void Terminate(GtkWidget* Widget, gpointer data)
    {
        GObject** ObjData = (GObject**)data;
        int2string *IntData = (int2string*)ObjData[1];
        GUIDataHandler *DH = (GUIDataHandler*)ObjData[2];

        FILE* tmpPng = fopen(IntData->FName.c_str(),"r");
        if (tmpPng != NULL)
        {
            fclose(tmpPng);
            remove(IntData->FName.c_str());
        }
        *IntData->prun = 0;
        gtk_main_quit();
        gtk_widget_destroy((GtkWidget*)ObjData[0]);

        SaveGUIConfig(DH);
    };

    /*
    Global variables:
        Objects: all the GTK GUI elements that we need to keep track of
        Data: data associated with GTK GUI Element locations in 'Objects'
    */
    GObject** Objects = g_new(GObject*,3); //global object array
    int2string Data; //global array database
    GUIDataHandler Handle; //global data context

    /*
    GUI KeyPress_CMD:
        Processes a key press for "TXT_CMD" data objects
        -Takes: GtkEntry widget, Object data
    */
    void KeyPress_CMD(GtkWidget* Widget, gpointer data)
    {
        GObject** ObjData = (GObject**)data;

        int2string *IntData = (int2string*)ObjData[1];
        GUIDataHandler *DH = (GUIDataHandler*)ObjData[2];

        std::string CMD = (std::string)gtk_entry_get_text((GtkEntry*)Widget);

        for (int i = 0; i < DH->SensorNames.size(); i++)
        {
            if ((void*)ObjData[IntData->Seek((DH->SensorNames[i] + std::string(std::to_string(i)) + "TXT_CMD").c_str())] == (void*)Widget)
                DH->SensorCommands[i] = CMD.c_str();
        }
    };

    /*
    GUI KeyPress_TEMP:
        Processes a key press for the "TXT_TEMP" data objects
        -Takes: GtkEntry widget, Object data
    */
    void KeyPress_TEMP(GtkWidget* Widget, gpointer data)// GdkEvent *e, gpointer data)
    {
        GObject** ObjData = (GObject**)data;

        int2string *IntData = (int2string*)ObjData[1];
        GUIDataHandler *DH = (GUIDataHandler*)ObjData[2];

        std::string TEMP = (std::string)gtk_entry_get_text((GtkEntry*)Widget);

        if (TEMP[0] == '.')
        {
            TEMP = "0";
            gtk_entry_set_text((GtkEntry*)Widget,TEMP.c_str());
        }

        for (int i = TEMP.length()-1; i >= 0; i--)
        {
            if ((TEMP[i] > '9' || TEMP[i] < '0') && TEMP[i] != '.')
            {
                TEMP.erase(TEMP.begin()+i);
                gtk_entry_set_text((GtkEntry*)Widget,TEMP.c_str());
            }
        }

        if (TEMP.length() == 0) TEMP = "0";

        for (int i = 0; i < DH->SensorNames.size(); i++)
        {
            if ((void*)ObjData[IntData->Seek((DH->SensorNames[i] + std::string(std::to_string(i)) + "TXT_TEMP").c_str())] == (void*)Widget)
                DH->SensorCriticals[i] = std::stof(TEMP.c_str());
        }
    };

    /*
    SetCollectData:
        Was planned for use with GtkSwitches in the Databox, 
        but was discontinued.
    */
    void SetCollectData(GtkWidget* Widget, gpointer data)
    {
        GObject** ObjData = (GObject**)data;
        int2string *IntData = (int2string*)ObjData[1];
        GUIDataHandler *DH = (GUIDataHandler*)ObjData[2];
 
        for (int i = 0; i < IntData->size(); i++) //for correct usage, see SetColour.
        {
            if ((void*)ObjData[i] == (void*)Widget)
                printf("1 for %d\n",i);
        }
    };

    /*
    GUI SetColour:
        Retrieves colour information from an incorrectly spelled
            GtkColorChooser widget and converts the data to RGBA
        -Takes: GtkColorChooser widget, Object data
    */
    void SetColour(GtkWidget* Widget, gpointer data)
    {
        GObject** ObjData = (GObject**)data;
        int2string *IntData = (int2string*)ObjData[1];
        GUIDataHandler *DH = (GUIDataHandler*)ObjData[2];

        GdkRGBA RGBARet;

        gtk_color_chooser_get_rgba((GtkColorChooser*)Widget,&RGBARet);

        unsigned int COL[4];
        COL[0] = (unsigned int)(RGBARet.red*UCHAR_MAX);
        COL[1] = (unsigned int)(RGBARet.green*UCHAR_MAX);
        COL[2] = (unsigned int)(RGBARet.blue*UCHAR_MAX);
        COL[3] = (unsigned int)(RGBARet.alpha*UCHAR_MAX); //unused
        unsigned int HexVal = 0x00010000*(unsigned int)COL[0] + 0x00000100*(unsigned int)COL[1] + 0x00000001*(unsigned int)COL[2];

        for (int i = 0; i < DH->SensorNames.size(); i++)
        {
            if ((void*)ObjData[IntData->Seek((DH->SensorNames[i] + std::string(std::to_string(i)) + "COLOUR").c_str())] == (void*)Widget)
            {
                DH->SensorColours[i] = HexVal;
                break;
            }
        }
    };

    /*
    GUI UpdateTemps:
        Updates temperature readouts in LBL_TEMP objects
            and executes designated commands when required
        -Takes: GtkWidget (unused), Object data
    */
    void UpdateTemps(gpointer data)
    {
        GObject** ObjData = (GObject**)data;
        int2string *IntData = (int2string*)ObjData[1];
        GUIDataHandler *DH = (GUIDataHandler*)ObjData[2];

        //Update Temperature Labels:
        for (int i = 0; i < DH->SensorNames.size(); i++)
        {
            std::string TempDat = std::to_string(DH->SensorData[i].back());
            gtk_label_set_text((GtkLabel*)Objects[Data.Seek((DH->SensorNames[i] + (std::string(std::to_string(i))) + "LBL_TEMP").c_str())],std::to_string(DH->SensorData[i].back()).c_str());
            if (DH->SensorData[i].back() >= DH->SensorCriticals[i])
            {
                const char *FMT = "<span foreground=\"#FF0000\" weight=\"heavy\">\%s</span>";
                char *MKP = g_markup_printf_escaped(FMT,TempDat.c_str());
                gtk_label_set_markup((GtkLabel*)Objects[Data.Seek((DH->SensorNames[i] + (std::string(std::to_string(i))) + "LBL_TEMP").c_str())],MKP);
//                g_free(MKP); //this was causing a double-free issue
                if (DH->SensorCommands[i].length() > 0)
                {
                    std::thread App(popen,DH->SensorCommands[i].c_str(),"r");
                    App.detach();
                }
            }
            else if (DH->SensorData[i].back() >= DH->SensorCriticals[i]-5)
            {
                const char *FMT = "<span foreground=\"#FFA100\" weight=\"bold\">\%s</span>";
                char *MKP = g_markup_printf_escaped(FMT,TempDat.c_str());
                gtk_label_set_markup((GtkLabel*)Objects[Data.Seek((DH->SensorNames[i] + (std::string(std::to_string(i))) + "LBL_TEMP").c_str())],MKP);
//                g_free(MKP);
            }
            else
            {
                const char *FMT = "<span foreground=\"#000000\" weight=\"normal\">\%s</span>";
                char *MKP = g_markup_printf_escaped(FMT,TempDat.c_str());
                gtk_label_set_markup((GtkLabel*)Objects[Data.Seek((DH->SensorNames[i] + (std::string(std::to_string(i))) + "LBL_TEMP").c_str())],MKP);
//                g_free(MKP);
            }
        };
    };

    /*
    GUI replot:
        Re-draws the GNUPlot and sends a call to update
            temperature readouts.
        -Takes: Container (unused), Object data
    */
    bool replot(gpointer data)
    {
        GObject** ObjData = (GObject**)data;
        int2string *IntData = (int2string*)ObjData[1];
        GUIDataHandler *DH = (GUIDataHandler*)ObjData[2];

        for (int i = 0; i < DH->SensorNames.size(); i++)
        {
            Handle.SensorActive[i] = gtk_switch_get_active((GtkSwitch*)Objects[IntData->Seek((Handle.SensorNames[i] + (std::string(std::to_string(i))) + "COLLECT").c_str())]);
        }

        guint wid,hit;
        wid = gtk_widget_get_allocated_width((GtkWidget*)ObjData[IntData->Seek("Graph Socket")]);
        hit = gtk_widget_get_allocated_height((GtkWidget*)ObjData[IntData->Seek("Graph Socket")]);

        IntData->Width = wid;
        IntData->Height = hit;

        FILE *Gnuplot = popen("gnuplot","w");
        if (Gnuplot == NULL)
        {
            fprintf(stderr,"[gnuplot]: unable to start program\n");
            return false;
        }
        fprintf(Gnuplot,"set terminal pngcairo size %d,%d\n",wid,hit);
        fflush(Gnuplot);
        fprintf(Gnuplot,"set output '%s'\n",IntData->FName.c_str());
        fflush(Gnuplot);
        fprintf(Gnuplot,"set xrange [0<*:]\n");
        fflush(Gnuplot);
        fprintf(Gnuplot,"set xlabel \"Time (seconds since program start)\"\n");
        fflush(Gnuplot);
        fprintf(Gnuplot,"set ylabel \"Temperature (^OC)\"\n");
        fflush(Gnuplot);
        fprintf(Gnuplot,"set grid\n");
        fflush(Gnuplot);
        fprintf(Gnuplot,"set key on\n");
        fflush(Gnuplot);
        fprintf(Gnuplot,"set key outside right\n");
        fflush(Gnuplot);
        fprintf(Gnuplot,"set label \"Plotted with Gnuplot\" at screen 0.0,1.0 boxed offset 1,-1\n");
        fflush(Gnuplot);
        fprintf(Gnuplot,"set title \"Temperature vs Time plot\"\n");
        fflush(Gnuplot);
        for (int i = 0; i < Handle.SensorNames.size(); i++)
        {
            if (Handle.SensorActive[i])
            {
                fprintf(Gnuplot,"set style line %d lc rgb '#%.6X' lw 3 pt 7\n",i+1,Handle.SensorColours[i]);
                fflush(Gnuplot);
                fprintf(Gnuplot,"$%s << EOD\n",Handle.SensorNames_NoSpace[i].c_str());
                fflush(Gnuplot);
                for (int j = 0; j < Handle.SensorData[i].size(); j++) //TODO: LIMIT TO 249 DATAPOINTS
                {
                    fprintf(Gnuplot,"%d %f\n",Handle.Times[i][j],Handle.SensorData[i][j]);
                    fflush(Gnuplot);
                }
                fprintf(Gnuplot,"EOD\n");
                fflush(Gnuplot);
            }
        };
        bool NoPlotmLine = 1;
        for (int i = 0; i < Handle.SensorNames.size(); i++)
        {
            if (Handle.SensorActive[i] && NoPlotmLine)
            {
                fprintf(Gnuplot," plot $%s title \"%s\" with linespoints ls %d",Handle.SensorNames_NoSpace[i].c_str(),Handle.SensorNames[i].c_str(),i+1);
                NoPlotmLine = 0;
            }
            else if (Handle.SensorActive[i])
                fprintf(Gnuplot,", $%s title \"%s\" with linespoints ls %d",Handle.SensorNames_NoSpace[i].c_str(),Handle.SensorNames[i].c_str(),i+1);
        }
        fprintf(Gnuplot,"\n");
        fflush(Gnuplot);

        pclose(Gnuplot);

        gtk_image_set_from_file((GtkImage*)ObjData[IntData->Seek("Graph Surface")],IntData->FName.c_str());
        gtk_widget_queue_draw((GtkWidget*)ObjData[IntData->Seek("Graph Surface")]);

        UpdateTemps(data);
        return false;
    };

    /*
    GUI CheckResize:
        Checks if the window has been resized and calls replot if it has
        -Takes: Widget (unused) and Object data.
    */
    bool CheckResize(gpointer data)
    {
        GObject** ObjData = (GObject**)data;
        int2string *IntData = (int2string*)ObjData[1];
        GUIDataHandler *DH = (GUIDataHandler*)ObjData[2];

        guint wid,hit;
        wid = gtk_widget_get_allocated_width((GtkWidget*)ObjData[IntData->Seek("Graph Socket")]);
        hit = gtk_widget_get_allocated_height((GtkWidget*)ObjData[IntData->Seek("Graph Socket")]);

        if (hit != IntData->Height || wid != IntData->Width)
            replot(data);
        return false;
    };

    /*
    GUI SetDataFields:
        Re-populates the Databoxes based on the Handler object
    */
    void SetDataFields(gpointer data)
    {
        GObject** ObjData = (GObject**)data;
        int2string *IntData = (int2string*)ObjData[1];
        GUIDataHandler *DH = (GUIDataHandler*)ObjData[2];

        for (int i = 0; i < DH->SensorNames.size(); i++)
        {
            std::string CurrentActive = DH->SensorNames[i] + std::to_string(i);
            //Set Enabled
            gtk_switch_set_active((GtkSwitch*)ObjData[IntData->Seek((CurrentActive + "COLLECT").c_str())],DH->SensorActive[i]);

            //Set Critical Temp
            gtk_entry_set_text((GtkEntry*)ObjData[IntData->Seek((CurrentActive + "TXT_TEMP").c_str())],std::to_string(DH->SensorCriticals[i]).c_str());

            //Set Colour
            unsigned int R,G,B;
            R = DH->SensorColours[i]/0x00010000;
            G = (DH->SensorColours[i] - (0x00010000*R))/0x00000100;
            B = (DH->SensorColours[i] - (0x00010000*R + 0x00000100*G))/0x00000001;

            GdkRGBA ColSet = {(double)R/255.0,(double)G/255.0,(double)B/255.0,1.0};

            gtk_color_chooser_set_rgba((GtkColorChooser*)ObjData[IntData->Seek((CurrentActive + "COLOUR").c_str())],&ColSet);
            //Set Command
            gtk_entry_set_text((GtkEntry*)ObjData[IntData->Seek((CurrentActive + "TXT_CMD").c_str())],DH->SensorCommands[i].c_str());
        }
    };

    /*
    GUI BuildInterface:
        Builds the entire GUI interface to be used
    */
    void BuildInterface(int argc, char* argv[], std::vector<std::string> SensorNames, bool* run)
    {
        Data.prun = run;

        GdkGeometry Geo;
        Geo.min_width = 480;
        Geo.min_height = 360;

        gtk_init(&argc,&argv);
        int ErrorValue = 0;

        GtkBuilder* Builder = gtk_builder_new();
        ErrorValue = gtk_builder_add_from_file(Builder,"UserInterface/Main.ui",NULL);
        if (ErrorValue == 0)
        {
            fprintf(stderr,"[UI Builder]: Unable to build user interface\n");
            return;
        }

        Data.Add("Toplevel");
        Objects[0] = gtk_builder_get_object(Builder,"Toplevel");
        gtk_window_set_title((GtkWindow*)Objects[0],"SafeTemp (EXPERIMENTAL GUI)");
        gtk_window_set_geometry_hints(GTK_WINDOW(Objects[0]),NULL,&Geo,GdkWindowHints(GDK_HINT_MIN_SIZE));
        gtk_widget_show_all((GtkWidget*)Objects[0]);
        Data.Add("DataPtr");
        Objects[1] = (GObject*)&Data;
        Data.Add("Handler");
        Objects[2] = (GObject*)&Handle;

        Data.Add("Graph Surface");
        Objects = (GObject**)realloc(Objects,(Data.size())*sizeof(GObject*));
        Objects[Data.Seek("Graph Surface")] = gtk_builder_get_object(Builder,"imgGraph");

        Data.Add("Graph Socket");
        Objects = (GObject**)realloc(Objects,(Data.size())*sizeof(GObject*));
        Objects[Data.Seek("Graph Socket")] = gtk_builder_get_object(Builder,"GraphSocket");

        Data.Add("Graph Socket Parent");
        Objects = (GObject**)realloc(Objects,(Data.size())*sizeof(GObject*));
        Objects[Data.Seek("Graph Socket Parent")] = gtk_builder_get_object(Builder,"GraphSocket_parent");

        int FileNO = 0;
        while (FileNO <= 256)
        {
            FileNO++;
            std::string TmpFile = "SafeTemp";
            TmpFile += std::to_string(FileNO);
            TmpFile += ".png";
            FILE* chk = fopen(TmpFile.c_str(),"r");
            if (chk == NULL)
            {
                break;
            }
            fclose(chk);
            if (FileNO > 255)
            {
                fprintf(stderr,"UNABLE TO PLOT GRAPH!\nFileNO < 256 failed\n");
                break;
            }
        }

        Data.FName = "/tmp/SafeTemp" + std::to_string(FileNO) + ".png";

        Data.Add("DataBox");
        Objects = (GObject**)realloc(Objects,(Data.size())*sizeof(GObject*));
        Objects[Data.Seek("DataBox")] = gtk_builder_get_object(Builder,"DataBox");

        //Build "Databox"
        for (int i = 0; i < SensorNames.size(); i++)
        {
            /*
            Builder object is recycled to add sensor information lines.
                NOTE: It is best to set all callback information within this loop.
                Attempts to set those functions externally would be messy.
            */
            Handle.SensorNames.push_back(std::string(SensorNames[i].c_str()));
            Handle.SensorCriticals.push_back(0); //MaxTemps is global

            Builder = gtk_builder_new_from_string(SensormLine,-1);

            Data.Add((SensorNames[i] + (std::string(std::to_string(i)))).c_str());
            Objects = (GObject**)realloc(Objects,(Data.size())*sizeof(GObject*));
            //Add sensor information lines
            Objects[Data.Seek((SensorNames[i] + (std::string(std::to_string(i)))).c_str())] = gtk_builder_get_object(Builder,"DataDisplay");
            gtk_box_pack_start((GtkBox*)Objects[Data.Seek("DataBox")],(GtkWidget*)Objects[Data.Seek((SensorNames[i] + (std::string(std::to_string(i)))).c_str())],1,1,5);
            GtkWidget* sep = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
            gtk_box_pack_start((GtkBox*)Objects[Data.Seek("DataBox")],sep,1,1,0);
            //Set Label
            GObject* label = gtk_builder_get_object(Builder,"lblSensor");
            gtk_label_set_text((GtkLabel*)label,(SensorNames[i] + "\n(" + (std::string(std::to_string(i))) + ")").c_str());

            Data.Add((SensorNames[i] + (std::string(std::to_string(i))) + "TXT_CMD").c_str());
            Objects = (GObject**)realloc(Objects,(Data.size())*sizeof(GObject*));
            //Add sensor information lines
            Objects[Data.Seek((SensorNames[i] + (std::string(std::to_string(i))) + "TXT_CMD").c_str())] = gtk_builder_get_object(Builder,"txtCommand");

            Data.Add((SensorNames[i] + (std::string(std::to_string(i))) + "TXT_TEMP").c_str());
            Objects = (GObject**)realloc(Objects,(Data.size())*sizeof(GObject*));
            //Add sensor information lines
            Objects[Data.Seek((SensorNames[i] + (std::string(std::to_string(i))) + "TXT_TEMP").c_str())] = gtk_builder_get_object(Builder,"txtCritical");
            gtk_entry_set_input_purpose((GtkEntry*)Objects[Data.Seek((SensorNames[i]+(std::string(std::to_string(i)))+"TXT_TEMP").c_str())],GTK_INPUT_PURPOSE_NUMBER);

            Data.Add((SensorNames[i] + (std::string(std::to_string(i))) + "LBL_TEMP").c_str());
            Objects = (GObject**)realloc(Objects,(Data.size())*sizeof(GObject*));
            //Add sensor information lines
            Objects[Data.Seek((SensorNames[i] + (std::string(std::to_string(i))) + "LBL_TEMP").c_str())] = gtk_builder_get_object(Builder,"lblTemp");

            Data.Add((SensorNames[i] + (std::string(std::to_string(i))) + "COLLECT").c_str());
            Objects = (GObject**)realloc(Objects,(Data.size())*sizeof(GObject*));
            //Add sensor information lines
            Objects[Data.Seek((SensorNames[i] + (std::string(std::to_string(i))) + "COLLECT").c_str())] = gtk_builder_get_object(Builder,"btnCollectData");

            Data.Add((SensorNames[i] + (std::string(std::to_string(i))) + "COLOUR").c_str());
            Objects = (GObject**)realloc(Objects,(Data.size())*sizeof(GObject*));
            //Add sensor information lines
            Objects[Data.Seek((SensorNames[i] + (std::string(std::to_string(i))) + "COLOUR").c_str())] = gtk_builder_get_object(Builder,"btnSensorCol");
        }

        //Connect "Databox" widgets
        for (int i = 0; i < SensorNames.size(); i++)
        {
            std::string Current = (SensorNames[i] + (std::string(std::to_string(i))));

            ErrorValue = g_signal_connect(Objects[Data.Seek((Current + "TXT_CMD").c_str())],"changed",G_CALLBACK(KeyPress_CMD),Objects);
            if (ErrorValue < 0) fprintf(stderr,"[%s]: Failed to connect TXT_CMD\n",Current.c_str());

            ErrorValue = g_signal_connect(Objects[Data.Seek((Current + "TXT_TEMP").c_str())],"changed",G_CALLBACK(KeyPress_TEMP),Objects);
            if (ErrorValue < 0) fprintf(stderr,"[%s]: Failed to connect TXT_TEMP\n",Current.c_str());

            ErrorValue = g_signal_connect(Objects[Data.Seek((Current + "COLOUR").c_str())],"color-set",G_CALLBACK(SetColour),Objects);
            if (ErrorValue < 0) fprintf(stderr,"[%s]: Failed to connect COLOUR\n",Current.c_str());
        }

        Handle.Harmonize();
        if (!ReadGUIConfig(&Handle))
            fprintf(stderr,"Unable to load ~/.config/TempSafe_GUI.cfg...\n");
        else
            SetDataFields(Objects);

        ErrorValue = g_signal_connect(Objects[Data.Seek("Toplevel")],"destroy",G_CALLBACK(Terminate),Objects);
        if (ErrorValue < 0) fprintf(stderr,"[Toplevel]: Failed to connect window handler\n");

        gtk_widget_show_all((GtkWidget*)Objects[0]);
    };
}

#endif
