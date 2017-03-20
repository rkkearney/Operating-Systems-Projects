// Madeline Gleason and Reilly Kearney 
// Project 4, site-tester.cpp
// 24 March 2017

#include <iostream>
#include <stdio.h>
#include <fstream>
#include <string.h>
#include <vector>
#include <algorithm>

using namespace std;

void config();

int main() {
	config();
	return 0;
	
}


void config() {

	// Configuration Parameters - Default Values 
	int PERIOD_FETCH = 180;				// time between fetches of sites
	int NUM_FETCH = 1;					// number of fetch threads
	int NUM_PARSE = 1;					// number of parsing threads
	string SEARCH_FILE = "Search.txt";	// file containing the search strings
	string SITE_FILE = "Sites.txt";		// file containing the sites to query 

	string line, param, value, delimiter = "=";
	int position;

	vector<string> parameters;
	vector<string> values;

	ifstream config_file;
	config_file.open("Config.txt");
	
	if (config_file.is_open()){
		while (getline (config_file, line)){
			position = line.find(delimiter);
			param = line.substr(0, line.find(delimiter));
			value = line.substr(position + 1);
			parameters.push_back(param);
			values.push_back(value);
		}
		config_file.close();
	}

	vector<string>::iterator it;
	it = find(parameters.begin(), parameters.end(), "SEARCH_FILE");
	if (it == parameters.end()){
		cout << "ERROR" << endl;
	}
}




