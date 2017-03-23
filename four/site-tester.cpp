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
void * thread_timer (void * pData);
void timer_handler(int s);
void sighup_handler(int s);

int main() {
	
	signal(SIGHUP, sighup_handler);

	/* PRIMARY SET UP OF CONFIG, SEARCH, SITE PARAMETERS */
	// Read and set up parameters from configuration file 
	Config config_class;	
	config_class.read_config_file();

	// Get search terms from file 
	get_search_terms(config_class.SEARCH_FILE);

	// Get sites from file 
	get_site_names(config_class.SITE_FILE);
	for (unsigned i = 0; i < SITES.size(); i++) {
		FETCH_QUEUE.push(SITES[i]);
	}

	// initialize Timer and Timer Thread 
	TIMER_COUNT = config_class.PERIOD_FETCH;

	pthread_mutex_t fetch_lock, parse_lock;
	pthread_cond_t  fetch_cond, parse_cond;

	pthread_t *timer_thread = (long unsigned int*) malloc(sizeof(pthread_t));
	timer_args timer_thread_args;
	timer_thread_args.fetch_lock = fetch_lock;
	timer_thread_args.fetch_cond = fetch_cond;
	timer_thread_args.period = 	config_class.PERIOD_FETCH;
	pthread_create(timer_thread, NULL, thread_timer, &timer_thread_args);

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

// Populates global vector with search terms from SEARCH_FILE
void get_search_terms(string file) {
	ifstream search_file;

	// open search file specified by configuration file
	search_file.open(file);	
	string line;
	
	if (search_file.is_open()){
		// read search file line by line
		while (getline (search_file, line)) {	
			// push search terms to a vector of terms
			SEARCH_LINES.push_back(line);		
		}
		// close file
		search_file.close();					
	}
}

// Populates global vector with site names from SITE_FILE
void get_site_names(string file) {
	ifstream site_file;

	// open site file specified by configuration file
	site_file.open(file);

	string siteName;
	
	if (site_file.is_open()){
		// read site file line by line
		while (getline (site_file, siteName)) {	
			// push sites to a vector of site names
			SITES.push_back(siteName);			
		}
		// close site file
		site_file.close();						
	}
}

// Gets timestamp from each fetch for output file
string get_timestamp() {
	time_t rawtime;
	struct tm * timeinfo;
	time(&rawtime);
	timeinfo = localtime(&rawtime);
	string time_str = asctime(timeinfo);
	time_str.erase(remove(time_str.begin(), time_str.end(), '\n'), time_str.end());
	return time_str;
}

/*	while (1) or while (gKeepRunning) // globalKeepRunning should go until you bailout
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

void * thread_fetch(void * pData) {
	fetch_args *args = (fetch_args *) pData;
	//cout << "thread_fetch\n";
	string site;
	pthread_mutex_lock(&args->fetch_lock);
	
	while (FETCH_QUEUE.empty()) {
		//cout << "waiting: thread_fetch" << endl;
		pthread_cond_wait(&args->fetch_cond, &args->fetch_lock);
	}
	
	/* THIS. IS. THE. FETCH. CRITICAL. SECTION. */
	site = FETCH_QUEUE.front();
	FETCH_QUEUE.pop();
	/* THIS. IS. THE. END. OF. THE. FETCH. CRITICAL. SECTION. */
	
	pthread_mutex_unlock(&args->fetch_lock);

	// perform libcurl on site
	string temp = get_site_contents(site);

	pthread_mutex_lock(&args->parse_lock);
	
	/* THIS. IS. THE. PARSE. CRITICAL. SECTION. */
	PARSE_QUEUE.push(temp);							// contents placed in queue for process from consumers
	pthread_cond_signal(&args->parse_cond);
	/* THIS. IS. THE. END. OF. THE. PARSE. CRITICAL. SECTION. */
	
	pthread_mutex_unlock(&args->parse_lock);
	return 0;
}

void * thread_parse (void * pData) {
	parse_args *args = (parse_args *) pData;
	//cout << "thread_parse:" << PARSE_QUEUE.size() << endl;

	pthread_mutex_lock(&args->parse_lock);
	
	while (PARSE_QUEUE.empty()) {
		//cout << "waiting: thread_parse" << endl;
		pthread_cond_wait(&args->parse_cond, &args->parse_lock);
	}
	
	/* THIS. IS. THE. CRITICAL. SECTION. */
	string time_str = get_timestamp();
	string site_content = PARSE_QUEUE.front();

	for (unsigned i = 0; i < SEARCH_LINES.size(); i++) {
		int count = 0;	
		size_t start = 0;

		// use search term as substring and count occurences 
		while ((start = site_content.find(SEARCH_LINES[i], start)) != site_content.npos) {	
			count++;
			start += SEARCH_LINES[i].length(); // see the note
		}

		// output occurences in proper format to output file
		// output_file << time_str << "," << SEARCH_LINES[j] << "," << FETCH_QUEUE.front() << "," << count << endl;
		cout << time_str << "," << SEARCH_LINES[i] << "," << SITES.front() << "," << count << endl;
	}
	PARSE_QUEUE.pop();
	/* THIS. IS. THE. END. OF. THE. CRITICAL. SECTION. */
	
	pthread_mutex_unlock(&args->parse_lock);
	return 0;
}

void * thread_timer (void * pData) {
	timer_args *args = (timer_args *) pData;
	//cout << "thread_timer\n";

	signal(SIGALRM, timer_handler);
	alarm(1);

	//cout << "TIMER_COUNT=" << TIMER_COUNT << endl;
	if (!TIMER_COUNT) {
		pthread_mutex_lock(&args->fetch_lock);
		for (unsigned i = 0; i < SITES.size(); i++) {
			FETCH_QUEUE.push(SITES[i]);
		}
		pthread_cond_signal(&args->fetch_cond);
		pthread_mutex_unlock(&args->fetch_lock);
		TIMER_COUNT = args->period;
	}
	return 0;
}

void timer_handler(int s) {
	TIMER_COUNT--;
	signal(SIGALRM, timer_handler);
	alarm(1);
}

void sighup_handler(int s) {
	cout << "caught " << s << " signal" << endl;
	exit(s);
}



