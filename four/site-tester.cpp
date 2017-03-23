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
vector<string> SITES;
int TIMER_COUNT;
vector<string> SEARCH_LINES;

// Structs
typedef struct fetch_args {
	pthread_mutex_t fetch_lock;
	pthread_cond_t fetch_cond;
	pthread_mutex_t parse_lock;
	pthread_cond_t parse_cond;
} fetch_args;

typedef struct parse_args {
	pthread_mutex_t parse_lock;
	pthread_cond_t parse_cond;
} parse_args;

typedef struct timer_args {
	pthread_mutex_t fetch_lock;
	pthread_cond_t fetch_cond;
	int period;
} timer_args;

// Function prototypes
void get_search_terms(string);
void get_site_names(string);
string get_timestamp();
void * thread_fetch(void *);
void * thread_parse(void *);
void * thread_timer (void *);
void timer_handler(int s);
void sighup_handler(int s);

int main() {
	
	// Initialize signal hangup given ^C in terminal 
	signal(SIGHUP, sighup_handler);

	// Read and set up parameters from configuration file 
	Config config_class;	
	config_class.read_config_file();

	// Get search terms from file 
	get_search_terms(config_class.SEARCH_FILE);

	// Initialize Timer
	TIMER_COUNT = config_class.PERIOD_FETCH;

	// intialize fetch / parse locks and condition variables 
	pthread_mutex_t fetch_lock = PTHREAD_MUTEX_INITIALIZER;
	pthread_mutex_t parse_lock = PTHREAD_MUTEX_INITIALIZER;
	pthread_cond_t  fetch_cond = PTHREAD_COND_INITIALIZER;
	pthread_cond_t parse_cond = PTHREAD_COND_INITIALIZER;

	// Initialize thread timer with appropriate arguments
	pthread_t *timer_thread = (long unsigned int*) malloc(sizeof(pthread_t));
	timer_args timer_thread_args;
	timer_thread_args.fetch_lock = fetch_lock;
	timer_thread_args.fetch_cond = fetch_cond;
	timer_thread_args.period = 	config_class.PERIOD_FETCH;
	pthread_create(timer_thread, NULL, thread_timer, &timer_thread_args);

	cout << "Timer thread initialized" << endl;

	// Get sites from SITE_FILE for first fetch
	get_site_names(config_class.SITE_FILE);
	for (unsigned i = 0; i < SITES.size(); i++) {
		FETCH_QUEUE.push(SITES[i]);
	}

	cout << "FETCH_QUEUE.size()=" << FETCH_QUEUE.size() << endl;

	/* Handle Output File */
	// Create output file 
	// ofstream output_file;
	// output_file.open ("output.csv");

	// Initialize correct number of threads given NUM_FETCH and NUM_PARSE
	pthread_t *fetch_threads = (long unsigned int*) malloc(config_class.NUM_FETCH * sizeof(pthread_t));
	pthread_t *parse_threads = (long unsigned int*) malloc(config_class.NUM_PARSE * sizeof(pthread_t));

	fetch_args fetch_thread_args;
	parse_args parse_thread_args;

	fetch_thread_args.fetch_lock = fetch_lock;
	fetch_thread_args.parse_lock = parse_lock;
	fetch_thread_args.fetch_cond = fetch_cond;
	fetch_thread_args.parse_cond = parse_cond;
	parse_thread_args.parse_lock = parse_lock;
	parse_thread_args.parse_cond = parse_cond;

	for (int i = 0; i < config_class.NUM_FETCH; i++) {
		pthread_create(&fetch_threads[i], NULL, thread_fetch, &fetch_thread_args);
	}

	for (int i = 0; i < config_class.NUM_PARSE; i++) {
		pthread_create(&parse_threads[i], NULL, thread_parse, &parse_thread_args);
	}

	/* WHERE DO WE JOIN? */

	// output_file.close();

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
	fetch_args *args = (fetch_args *) pData;						// create fetch thread arguments
	cout << "thread_fetch\n";
	string site;

	/* DO WE NEED ANOTHER WHILE LOOP HERE? */

	
	pthread_mutex_lock(&args->fetch_lock);							// lock fetch mutex
	while (FETCH_QUEUE.empty()) {					
		pthread_cond_wait(&args->fetch_cond, &args->fetch_lock);	// wait if there is no work for thread 
	}	
	site = FETCH_QUEUE.front();										// pop first item from queue for libcurl call
	FETCH_QUEUE.pop();	
	cout << site << endl;
	pthread_mutex_unlock(&args->fetch_lock);						// unlock fetch mutex
	
	string temp = get_site_contents(site);							// perform libcurl on site
	
	pthread_mutex_lock(&args->parse_lock);							// lock parse mutex
	PARSE_QUEUE.push(temp);											// put data into parse_queue for consumers 
	pthread_cond_signal(&args->parse_cond);							// signal thread to wake up 		
	pthread_mutex_unlock(&args->parse_lock);						// unlock parse mutex	

	return 0;
}

// Handler function for parse threads
void * thread_parse (void * pData) {
	parse_args *args = (parse_args *) pData;						// create parse thread arguments
	cout << "thread_parse:" << endl;

	pthread_mutex_lock(&args->parse_lock);							// lock parse mutex

	while (PARSE_QUEUE.empty()) {									// wait if there is no work for thread
		pthread_cond_wait(&args->parse_cond, &args->parse_lock);
	}
	cout << "PARSE_QUEUE.size()=" << PARSE_QUEUE.size() << endl;
	
	string time_str = get_timestamp();								// get timestamp from fetch
	string site_content = PARSE_QUEUE.front();						// get string content from queue

	for (unsigned i = 0; i < SEARCH_LINES.size(); i++) {			// iterate through all search terms in file
		int count = 0;	
		size_t start = 0;

		while ((start = site_content.find(SEARCH_LINES[i], start)) != site_content.npos) {	// use substring notation to search for term
			count++;																		// increment count for search term
			start += SEARCH_LINES[i].length();												// increment start by lenght of string to continue
		}

		cout << time_str << "," << SEARCH_LINES[i] << "," << SITES.front() << "," << count << endl;	// output counts to cout right now 
	}

	PARSE_QUEUE.pop();									// remove content from queue once processed
	pthread_mutex_unlock(&args->parse_lock);			// unlock mutex

	return 0;
}

// Handler function for timer thread 
void * thread_timer (void * pData) {
	timer_args *args = (timer_args *) pData;			// create timer thread arguments
	cout << "thread_timer\n";

	signal(SIGALRM, timer_handler);						// signal alarm to decrement counter
	alarm(1);											// count 1 second at a time
	
	if (!TIMER_COUNT) {									// when timer runs out
		pthread_mutex_lock(&args->fetch_lock);			// lock fetch mutex
		for (unsigned i = 0; i < SITES.size(); i++) {	// repopulate fetch queue for producers
			FETCH_QUEUE.push(SITES[i]);
		}
		
		pthread_cond_signal(&args->fetch_cond);			// signal producers fetch_queue contains sites	
		pthread_mutex_unlock(&args->fetch_lock);		/* DOES THIS WAKE UP THE THREADS AND THEN UNLOCK? */
		TIMER_COUNT = args->period;						// reset timer				
	}
	return 0;
}

// Handler function for timer
void timer_handler(int s) {
	TIMER_COUNT--;										// decrement counter 
	alarm(1);											// one second at a time
	signal(SIGALRM, timer_handler);
}

// Hanlder function for sighup signal catching
void sighup_handler(int s) {
	cout << "Caught " << s << " Signal" << endl;		// catch sighup signal
	cout << "Program terminated" << endl;				
	exit(s);											// terminate program
}



