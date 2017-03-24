
Project 4: System for Verifying Web Placement 
Authors: 
	Maddie Gleason (mgleaso4)  ** submitted to dropbox
	Reilly Kearney (rkearne1)

Description: 

Site-tester.cpp imports "getsites.cpp" and "Config.cpp", files used to fetch content from listed sites and configure all parameters, respectively. 

Our site-tester executable accepts the configuration file as an argument. The configuration file, site file, and search file should be formatted as specified in the project document.  

To run the code, simply execute site-tester with the configuration file you wish to use. Note default values will be established for all parameters, except SITE_FILE and SEARCH_FILE. The program will explain you did not specify appropriate parameters. Additionally, if you forget to enter a file or enter an invalid file, our program with quit with an appropriate error message. 

We used SIGALRM as a timer. As the program runs a clock printed on the screen runs down until a new fetch is completed. The results are aggregated in a single "output.txt" file. Every time a new fetch occurs, the file is overwritten with the new data. Only when the user quits the program with ^C can you view the output file.

When a user quits the program, the program closes the output file and ends all thread use immediately.  

Please see our EC2.txt file for more information on how we pursued this extra credit. 