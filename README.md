# SafeTemp: preventing overheating

This is currently on its first release.

This software checks that your Linux system is operating at a safe temperature and optionally executes a command if it's not.  If the user does not specify a temperature threshold file, no temperature threshold is set and the program will not do anything (although if the -v modifier is used, the program will display the sensor temperatures)

How to install: run "make"; installs to local directory

to run, run ./tempsafe from the install directory

Usage: 
-p	 Path to lm-sensors config file
-w	 time interval to wait between checks (seconds); default is 5 seconds
-T	 Load temperatures from a file
-i	 Don't run, just print temperatures and exit (implies -v)
-v	 Verbose output (print temperatures at each TIME interval)
-C	 execute a shell script; 
    		 SCRIPT path should be given in double-quotes.
-h	 Print this help file
