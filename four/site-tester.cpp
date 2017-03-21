// Madeline Gleason and Reilly Kearney 
// Project 4, site-tester.cpp
// 24 March 2017

#include <iostream>
#include <stdio.h>
#include <fstream>
#include <string.h>
#include <vector>
#include <algorithm>
#include <curl/curl.h>

#include "Config.cpp"
#include "getsites.cpp"

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

	for (unsigned i = 0; i < search_lines.size(); i++) {
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

	//vector<string> site_contents;

	for (unsigned i = 0; i < sites.size(); i++) {
		cout << sites[i] << endl;
		
		//cout << get_site_contents(sites[i]) << endl;
		string temp = get_site_contents(sites[i]);
		time_t rawtime;
		struct tm * timeinfo;

		time(&rawtime);
		timeinfo = localtime(&rawtime);
		string time_str = asctime(timeinfo);
		time_str.erase(remove(time_str.begin(), time_str.end(), '\n'), time_str.end());
		cout << time_str << ", ";
			
		int count = 0;	
		size_t start = 0;
		while ((start = temp.find("News", start)) != temp.npos) {
			count++;
			start += strlen("News"); // see the note
		}
		cout << "News" << ", " << sites[i] << ", " << count << endl;
	}

	return 0;
}




