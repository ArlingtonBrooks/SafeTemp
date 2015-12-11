# SafeTemp: preventing overheating

This is currently on its first release.

This software checks that your Linux system is operating at a safe temperature and optionally executes a command if it's not.  If the user does not specify a temperature threshold file, no temperature threshold is set and the program will not do anything (although if the -v modifier is used, the program will display the sensor temperatures)

To install, run ./configure, then make, then (as root) make install
If you want to compile with NVIDIA functionality, you'll need to install the NVML libraries; it is also possible that you'll need a copy of nvml.h in the local directory (which is not provided through this distribution).  See https://developer.nvidia.com/gpu-deployment-kit to install this library.

to run, run ./tempsafe from the install directory

Usage: 

-p	 Path to lm-sensors config file

-w	 time interval to wait between checks (seconds); default is 5 seconds

-T	 Load temperatures from a file

-s   Print estimates for maximum temperature and time until maximum temperature
        (VERY rough -- shouldn't be trusted for anything important).  This WILL NOT WORK if the timestep is too small; natural heating follows an exponential decay so if there are 3 consecutive timesteps where the temperature values don't change or change as an exponential growth this function will fail to return.

-i	 Don't run, just print temperatures and exit (implies -v)

-v	 Verbose output (print temperatures at each TIME interval)

-C	 execute a shell script; 
    		 SCRIPT path should be given in double-quotes.
    		 
-h	 Print this help file
