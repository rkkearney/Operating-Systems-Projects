// Madeline Gleason and Reilly Kearney 
// Project 4, site-tester.cpp
// 24 March 2017

#include <iostream>
#include <stdio.h>
#include <fstream>
#include <string.h>
#include <vector>
#include <queue>
#include <algorithm>
#include <curl/curl.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>

#include "Config.cpp"
#include "getsites.cpp"

using namespace std;

// Global Variables
queue<string> FETCH_QUEUE;
queue<string> PARSE_QUEUE;
queue<string> SITE_NAMES;
vector<string> SITES;
int TIMER_COUNT, PERIOD;
vector<string> SEARCH_LINES;
int KEEPLOOPING = 1;
ofstream OUTPUT_FILE;

// Global fetch / parse locks and condition variables 
pthread_mutex_t fetch_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t parse_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t output_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  fetch_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t parse_cond = PTHREAD_COND_INITIALIZER;

// Function prototypes
void get_search_terms(string);
void get_site_names(string);
string get_timestamp();
void * thread_fetch(void *);
void * thread_parse(void *);
void timer_handler(int s);
void sigint_handler(int s);

int main(int argc, char* argv[]) {
	string file;
	
	if (argc != 2){
		cout << "usage: ./site-tester <configuration_file>" << endl;
		exit(1);
	}	
	file = argv[1];
	
	// Initialize signal hangup given ^C in terminal 
	signal(SIGINT, sigint_handler);

	while (1) {
		
		// Read and set up parameters from configuration file 
		Config config_class;	
		int bad = config_class.read_config_file(file);
		if (bad){
			exit(1);
		}

		// Get search terms from file 
		get_search_terms(config_class.SEARCH_FILE);

		// Get sites from SITE_FILE for first fetch
		get_site_names(config_class.SITE_FILE);
		PERIOD = config_class.PERIOD_FETCH;
		
		// Initialize correct number of threads given NUM_FETCH and NUM_PARSE
		pthread_t *fetch_threads = (long unsigned int*) malloc(config_class.NUM_FETCH * sizeof(pthread_t));
		pthread_t *parse_threads = (long unsigned int*) malloc(config_class.NUM_PARSE * sizeof(pthread_t));

		// Create fetch and parse threads
		for (int i = 0; i < config_class.NUM_FETCH; i++) {
			pthread_create(&fetch_threads[i], NULL, thread_fetch, NULL);
		}

		for (int i = 0; i < config_class.NUM_PARSE; i++) {
			pthread_create(&parse_threads[i], NULL, thread_parse, NULL);
		}

		// singal to timer
		signal(SIGALRM, timer_handler);						
		TIMER_COUNT = 0;
		alarm(1);	

		// join fetch and parse threads 
		for (int i = 0; i < config_class.NUM_FETCH; i++) {
			pthread_join(fetch_threads[i], NULL);
		}
		for (int i = 0; i < config_class.NUM_PARSE; i++) {
			pthread_join(parse_threads[i], NULL);
		}
	}
	return 0;
}

// Populate global vector with search terms from SEARCH_FILE
void get_search_terms(string file) {
	ifstream search_file;
	search_file.open(file);								// open search file specified by configuration file
	string line;
	
	if (search_file.is_open()){
		while (getline (search_file, line)) {			// read search file line by line
			SEARCH_LINES.push_back(line);				// push search terms to a vector of terms
		}
		search_file.close();							// close file				
	}
}

// Populate global vector with site names from SITE_FILE
void get_site_names(string file) {
	ifstream site_file;	
	site_file.open(file);								// open site file specified by configuration file
	string siteName;
	
	if (site_file.is_open()){
		while (getline (site_file, siteName)) {			// read site file line by line
			SITES.push_back(siteName);					// push sites to a vector of site names	
		}
		site_file.close();								// close site file					
	}
}

// Get timestamp from each fetch for output file
string get_timestamp() {
	time_t rawtime;
	struct tm * timeinfo;
	time(&rawtime);
	timeinfo = localtime(&rawtime);
	string time_str = asctime(timeinfo);
	time_str.erase(remove(time_str.begin(), time_str.end(), '\n'), time_str.end());
	return time_str;
}

// Handler function for fetch threads
void * thread_fetch(void * pData) {
	string site;
	while (1){
		pthread_mutex_lock(&fetch_lock);							// lock fetch mutex
		while (FETCH_QUEUE.empty() && KEEPLOOPING) {					
			pthread_cond_wait(&fetch_cond, &fetch_lock);			// wait if there is no work for thread 
		}	
		site = FETCH_QUEUE.front();									// pop first item from queue for libcurl call
		SITE_NAMES.push(site);
		FETCH_QUEUE.pop();	

		pthread_mutex_unlock(&fetch_lock);							// unlock fetch mutex
		
		string temp = get_site_contents(site);						// perform libcurl on site

		pthread_mutex_lock(&parse_lock);							// lock parse mutex
		PARSE_QUEUE.push(temp);										// put data into parse_queue for consumers 
		pthread_cond_signal(&parse_cond);							// signal thread to wake up 		
		pthread_mutex_unlock(&parse_lock);							// unlock parse mutex	
	}
	return 0;
}

// Handler function for parse threads
void * thread_parse (void * pData) {
	while (1){
		pthread_mutex_lock(&parse_lock);								// lock parse mutex

		while (PARSE_QUEUE.empty() && KEEPLOOPING) {					// wait if there is no work for thread
			pthread_cond_wait(&parse_cond, &parse_lock);
		}
		
		string time_str = get_timestamp();								// get timestamp from fetch
		string site_content = PARSE_QUEUE.front();						// get string content from queue
		//cout << site_content << endl;

		for (unsigned i = 0; i < SEARCH_LINES.size(); i++) {			// iterate through all search terms in file
			int count = 0;	
			unsigned position = 0;
			unsigned end = 0;

			// set position and end to only search HTML body 
			position = site_content.find("<body", 0);					// start HTML body 
			end = site_content.find("</body>");							// end HTML body											
			
			while ((position = site_content.find(SEARCH_LINES[i], position)) < end) {	// use substring notation to search for term		
				count++;	
				position = position + SEARCH_LINES[i].size();	
			}

			pthread_mutex_lock(&output_lock);																			// lock output file mutex
			OUTPUT_FILE << time_str << "," << SEARCH_LINES[i] << "," << SITE_NAMES.front() << "," << count << endl;		// print output to file
			pthread_mutex_unlock(&output_lock);																			// unlock output file mutex
		}

		PARSE_QUEUE.pop();								// remove content from queue once processed	
		SITE_NAMES.pop();								// remove site name from name queue 
		pthread_mutex_unlock(&parse_lock);				// unlock mutex
	}
	return 0;
}

// Handler function for timer
void timer_handler(int s) {
	if (!TIMER_COUNT) {	
		pthread_mutex_lock(&output_lock);				// lock output file mutex
		OUTPUT_FILE.open ("output.csv");				// Create output file 
		pthread_mutex_unlock(&output_lock);				// unlock output file mutex 
		
		pthread_mutex_lock(&fetch_lock);				// lock fetch mutex
		for (unsigned i = 0; i < SITES.size(); i++) {	// repopulate fetch queue for producers
			FETCH_QUEUE.push(SITES[i]);
		}		
		pthread_cond_broadcast(&fetch_cond);			// signal producers fetch_queue contains sites	
		pthread_mutex_unlock(&fetch_lock);				/* DOES THIS WAKE UP THE THREADS AND THEN UNLOCK? */
		TIMER_COUNT = PERIOD;							// reset timer	
	}

	TIMER_COUNT--;										// decrement timer count
	cout << "timer: " << TIMER_COUNT << endl;
	alarm(1);											// re-arm timer to go off 1 second later
}

// Hanlder function for sighup signal catching
void sigint_handler(int s) {

	KEEPLOOPING = 0;
	
	pthread_mutex_lock(&output_lock);				// lock output file mutex
	OUTPUT_FILE.close();							// close output file
	pthread_mutex_unlock(&output_lock);				// unlock output file mutex
	
	cout << endl << "Program terminated" << endl;				
	exit(s);										// terminate program
}
