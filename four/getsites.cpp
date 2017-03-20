// Madeline Gleason and Reilly Kearney 
// Project 4, site-tester.cpp
// 24 March 2017

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

using namespace std;

struct MemoryStruct {
	char *memory;
	size_t size;
}

static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp){
	size_t realsize = size *nmemb;
	struct MemoryStruct *mem = (struct MemoryStruct *)userp;

	mem->memory = realloc(mem->memory, mem->size + realsize + 1);
	if (mem->memory == NULL){
		cout << "Not enouch memory (realloc returned NULL)" << endl;
		return 0;
	}
	memcpy(&(mem->memory[mem->size]), contents, realsize);
  	mem->size += realsize;
  	mem->memory[mem->size] = 0;
 
  return realsize;
}

int main() {
	CURL *curl_handle;
	CURLcode res;

	struct MemoryStruct chunk;

	chunk.memory = malloc(1);  	// will be grown as needed by the realloc above
	chunk.size = 0;    			// no data at this point

	curl_global_init(CURL_GLOBAL_ALL);

	//initialize the curl session 
	curl_handle = curl_easy_init();

	// specify URL to get 
	curl_easy_setopt(curl_handle, CURLOPT_URL, "http://www.nd.edu/");

	// send all data to this function 
	curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);

	// pass our 'chunk' struct to the callback function 
	curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);

	// so we provide a user-agent field 
	curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");

	// get it
	res = curl_easy_perform(curl_handle);

	// check for errors
	if(res != CURLE_OK) {
		fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
	}
	else {
		/*
		 * Now, our chunk.memory points to a memory block that is chunk.size
		 * bytes big and contains the remote file.
		 *
		 * Do something nice with it!
		 */ 
		printf("%lu bytes retrieved\n", (long)chunk.size);
	}

	// cleanup curl stuff
	curl_easy_cleanup(curl_handle);

	// free allocated memory 
	free(chunk.memory);

	// clean up libcurl 
	curl_global_cleanup();

	return 0;
}