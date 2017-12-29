# SafeTemp: preventing overheating

**NOTICE: Due to lack of upstream development for lm_sensors, especially as it relates to modern hardware releases such as AMD's Ryzen architecture, SafeTemp is no longer under active development.  Any bugs will continue to be fixed depending on their severity, however feature updates will no longer be pushed.**

**NOTICE2: This software is spread out in its construction and does not correctly implement static libraries.  Code clarification and cleaning was intended to be completed in a future feature update, however should no longer be expected.**

This software checks that your Linux system is operating at a safe temperature and optionally executes a command if it's not.  If the user does not specify a temperature threshold file, no temperature threshold is set and the program will not do anything (although if the -v modifier is used, the program will display the sensor temperatures)

To install, run ./configure, then make, then (as root) make install
If you want to compile with NVIDIA functionality, you'll need to install the NVML libraries; it is also possible that you'll need a copy of nvml.h in the local directory (which is not provided through this distribution).  See https://developer.nvidia.com/gpu-deployment-kit to install this library.

to run, run ./tempsafe from the install directory

Usage: 

-p	        Path to lm-sensors config file

-f              Specify file containing critical temperatures to check for

-w	        time interval to wait between checks (seconds); default is 5 seconds

-i	        Don't run, just print temperatures and exit (implies -v)

-v	        Verbose output (print temperatures at each TIME interval)

-C              execute a shell script; 
    		        SCRIPT path should be given in double-quotes.
    		 
-UI             EXPERIMENTAL: Starts new user interface (overrides -v, -c, -f, and -s) Start with User Interface (overrides -v, -C, -f, and -s).  User Interface reads and writes a config file from /etc/TempSafe.cfg which contains the sensor critical temperature, the sensor colour code, and the command to be executed when triggered.  The user interface also graphs the temperature over time in the command line environment.  The User Interface updates at least every 500 ms.  If a time interval is set, the user should expect up to 500 ms additional waiting time (in addition to what is specified in -w) before a sensor's data is updated.
                         The user interface is experimental and has not been thoroughly tested.  Use at your own risk.
                
--use-gtk       EXPERIMENTAL: Use GTK graphical interface.  Reads config file from ~/.config/TempSafe_GUI.cfg
                        *This argument can only be used with the -w argument*.  Other combinations are undefined.
                        This user interface is experimental.  Use at your own risk.
                        
**WARNING**: the GTK interface is known to crash without warning.  It should not be used outside of evaluating the capabilities of the SafeTemp program at this time.  A fix will be released in the future.  Currently, use of the GTK interface is **DISCOURAGED**.
    		 
-h	        Print this help file
