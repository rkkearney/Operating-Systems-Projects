// Madeline Gleason and Reilly Kearney 
// Project 4, config.cpp
// 24 March 2017

#include <iostream>
#include <stdio.h>
#include <fstream>
#include <string.h>
#include <vector>
#include <map>
#include <algorithm>

using namespace std;

class Config {
	public:
		Config();
		int read_config_file(string);
		int PERIOD_FETCH;
		int NUM_FETCH;
		int NUM_PARSE;
		string SEARCH_FILE;
		string SITE_FILE;
};

Config::Config() {

	// Configuration Parameters - Default Values 
	PERIOD_FETCH = 180;				// time between fetches of sites
	NUM_FETCH = 1;					// number of fetch threads
	NUM_PARSE = 1;					// number of parsing threads
	SEARCH_FILE = "Search.txt";	// file containing the search strings
	SITE_FILE = "Sites.txt";		// file containing the sites to query
}

int Config::read_config_file(string fileName) { 
	string line, param, value, delimiter = "=";
	int position;

	map<string,string> config_parameters;		// map paramters to value

	ifstream config_file;
	config_file.open(fileName);

	if (config_file.is_open()){								// open specified configuration file
		while (getline (config_file, line)){				// search through each lines
			position = line.find(delimiter);				// delimit based on =
			param = line.substr(0, line.find(delimiter));	// create parameter
			value = line.substr(position + 1);				// and value
			config_parameters.emplace(param, value);		// add to map
		}		
		config_file.close();								// close file
	} else {
		cout << "File not found." << endl;					// print error message if problem with file
		return 1;
	}

	// check for necessary parameters - SEARCH_FILE and SITE_FILE
	auto it = config_parameters.find("SEARCH_FILE");	
	auto iter = config_parameters.find("SITE_FILE");
	
	if (it == config_parameters.end() && iter == config_parameters.end()) {
		cout << "error: SEARCH_FILE and SITE_FILE not specified" << endl;
		return 1;
	}
	if (it == config_parameters.end()) {
		cout << "error: SEARCH_FILE not specified" << endl;
		return 1;
	}
	if (iter == config_parameters.end()) {
		cout << "error: SITE_FILE not specified" << endl;
		return 1;
	}

	if (config_parameters.size() > 5) {
		cout << "warning: unknown parameter passed" << endl;
	}

	// ESTABLISH ALL PARAMTERS GIVEN MAP KEYS / VALUES
	for (auto it = config_parameters.begin(); it != config_parameters.end(); it++) {
		if (it->first == "PERIOD_FETCH") {
			PERIOD_FETCH = stoi(it->second);
		} else if (it->first == "NUM_FETCH") {
			NUM_FETCH = stoi(it->second);
		} else if (it->first == "NUM_PARSE") {
			NUM_PARSE = stoi(it->second);
		} else if (it->first == "SEARCH_FILE") {
			SEARCH_FILE = it->second;
		} else if (it->first == "SITE_FILE") {
			SITE_FILE = it->second;
		}
	}
	return 0;
}