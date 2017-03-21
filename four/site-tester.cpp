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

vector<string> get_search_terms(string);
vector<string> get_site_names(string);
string get_timestamp();


/* Psuedo-code notes from class 
// global variables are OK 
	// fetch queue and parse queue 
// whatever you have in start-up (all files) won't change while program is running 
// have a big loop running
// 1) need to consume - see if there is work to do 

void * threadFetch (void * pData)
	while (1) or while (gKeepRunning) // globalKeepRunning should go until you bailout
		lock fetch_queue mutex
		while (fetch_queue.getCount() == 0) //no work to do 
			pthread_cond_wait(mutex, cond_variable) //conditionally wait 
		pop first item from queue (it's one of the sites)
		unlock fetch_queue mutex 
		CURL // each thread can't share the same handler 
		lock parse_queue 
		put data / work into parse_queue // probably a big C++ string 
		signal or broadcast // signal wakes up one thread, bcast wakes up all 
		// more general use bcast
		unlock parse_queue
*/

int main() {
	
	Config config_class;
	config_class.read_config_file();

	// Get search terms from file 
	vector<string> search_lines;
	search_lines = get_search_terms(config_class.SEARCH_FILE);

	// Get sites from file 
	vector<string> sites;
	sites = get_site_names(config_class.SITE_FILE);

	ofstream output_file;
	output_file.open ("output.csv");
	for (unsigned i = 0; i < sites.size(); i++) {
		string temp = get_site_contents(sites[i]);

		string time_str = get_timestamp();
		
		for (unsigned j = 0; j < search_lines.size(); j++) {
			int count = 0;	
			size_t start = 0;
			while ((start = temp.find(search_lines[j], start)) != temp.npos) {
				count++;
				start += search_lines[j].length(); // see the note
			}
			output_file << time_str << "," << search_lines[j] << "," << sites[i] << "," << count << endl;
		}
	}
	output_file.close();

	return 0;
}

vector<string> get_search_terms(string file) {
	ifstream search_file;
	search_file.open(file);
	string line;
	vector<string> search_lines;
	
	if (search_file.is_open()){
		while (getline (search_file, line)) {
			search_lines.push_back(line);
		}
		search_file.close();
	}

	return search_lines;
}

vector<string> get_site_names(string file) {
	ifstream site_file;
	site_file.open(file);
	string siteName;
	vector<string> sites;
	
	if (site_file.is_open()){
		while (getline (site_file, siteName)) {
			sites.push_back(siteName);
		}
		site_file.close();
	}

	return sites;
}

string get_timestamp() {
	time_t rawtime;
	struct tm * timeinfo;
	time(&rawtime);
	timeinfo = localtime(&rawtime);
	string time_str = asctime(timeinfo);
	time_str.erase(remove(time_str.begin(), time_str.end(), '\n'), time_str.end());
	return time_str;
}

