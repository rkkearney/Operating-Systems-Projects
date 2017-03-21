// Madeline Gleason and Reilly Kearney 
// Project 4, site-tester.cpp
// 24 March 2017

#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
using namespace std;

static size_t WriteMemoryCallback(void *, size_t, size_t, void *);
string get_site_contents(string);

struct MemoryStruct {
	char *memory;
	size_t size;
};

static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
	size_t realsize = size *nmemb;
	struct MemoryStruct *mem = (struct MemoryStruct *)userp;

	mem->memory = (char *)realloc(mem->memory, mem->size + realsize + 1);
	if (mem->memory == NULL){
		cout << "Not enouch memory (realloc returned NULL)" << endl;
		return 0;
	}
	memcpy(&(mem->memory[mem->size]), contents, realsize);
	mem->size += realsize;
	mem->memory[mem->size] = 0;
 
	return realsize;
}

string get_site_contents(string site) {
	CURL *curl_handle;
	CURLcode res;

	struct MemoryStruct chunk;
	string site_content;

	chunk.memory = (char *)malloc(1);  	// will be grown as needed by the realloc above
	chunk.size = 0;    					// no data at this point

	curl_global_init(CURL_GLOBAL_ALL);

	//initialize the curl session 
	curl_handle = curl_easy_init();

	// specify URL to get 
	curl_easy_setopt(curl_handle, CURLOPT_URL, site.c_str());

	// send all data to this function 
	curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);

	// pass our 'chunk' struct to the callback function 
	curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);

	// so we provide a user-agent field 
	curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");

	// get it
	res = curl_easy_perform(curl_handle);

	// check for errors
	if (res != CURLE_OK) {
		fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
	} else {
		for (unsigned i = 0; i < chunk.size; i++) {
			site_content.push_back(chunk.memory[i]);
		}
	}

	// cleanup curl stuff
	curl_easy_cleanup(curl_handle);

	// free allocated memory 
	free(chunk.memory);

	// clean up libcurl 
	curl_global_cleanup();

	return site_content;
}