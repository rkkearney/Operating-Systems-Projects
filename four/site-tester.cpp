// Madeline Gleason and Reilly Kearney 
// Project 4, site-tester.cpp
// 24 March 2017

#include <iostream>
#include <stdio.h>
#include <fstream>
#include <string.h>
#include <vector>
#include <algorithm>

#include "Config.cpp"

using namespace std;


int main() {
	
	Config config_class;
	config_class.read_config_file();
	config_class.print();

	// Get search terms from file 
	ifstream search_file;
	search_file.open(config_class.SEARCH_FILE);
	string line;
	vector<string> search_lines;
	
	if (search_file.is_open()){
		while (getline (search_file, line)) {
			search_lines.push_back(line);
		}
		search_file.close();
	}

	for (auto i = 0; i < search_lines.size(); i++) {
		cout << search_lines[i] << endl;
	}

	// Get sites from file 
	ifstream site_file;
	site_file.open(config_class.SITE_FILE);
	string siteName;
	vector<string> sites;
	
	if (site_file.is_open()){
		while (getline (site_file, siteName)) {
			sites.push_back(siteName);
		}
		site_file.close();
	}

	for (auto i = 0; i < sites.size(); i++) {
		cout << sites[i] << endl;
	}

	return 0;
}




