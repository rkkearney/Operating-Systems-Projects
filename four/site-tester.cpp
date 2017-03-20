// Madeline Gleason and Reilly Kearney 
// Project 4, site-tester.cpp
// 24 March 2017

#include <iostream>
#include <stdio.h>
#include <fstream>
#include <string.h>
#include <vector>
#include <algorithm>

#include <config.cpp>

using namespace std;


int main() {
	Config config_class();
	config_class.read_config_file();
	config_class.print();

	return 0;
	
}




