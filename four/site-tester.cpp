// Madeline Gleason and Reilly Kearney 
// Project 4, site-tester.cpp
// 24 March 2017

// Goal: Periodically queuery a set of URLs
// to see if a pre-specified list of keywords
// (or links) appears on those sites

/* 
	argument: configuration file
		configuration file: 1 parameter per line
		PARAM = XXXXXX
		unknown parameters should convey a warning, but may be ignored
		parameters should have a defualt value 
		if search_file or site_file cannot be found
			program should convey error and not start up 

		Parameter		Description													Default
		PERIOD_FETCH	The time (in seconds) between fetches of the various sites	180
		NUM_FETCH		Number of fetch threads										1
		NUM_PARSE		Number of parsing threads									1
		SEARCH_FILE		File containing the search strings							Search.txt
		SITE_FILE		File containing the sites to query							Sites.txt


	search file: 
		one string per line
		any set of characters (spaces) but no carriage return or comma
		a match occurs if there is an exact match (case sensitive)

	site file: 
		one site per line
		line prefaced by http://

	output file: 
		filenames: 1.csv, 2.csv 
		Date and Time, Search Phrase, Site, Count 
		should combine all these files into one bigger file 
*/

//Every N seconds (specified by the period)
	// fetching threads should fetch the various site content
	// You can use SIG_ALRM or something different
// The fetch threads use libcurl to get content
	// Data is copied and placed into a queue for the parse threads (word counters)
// The parse threads then take that content and count all of the various search words
// As each parse thread finishes a result, it should append the data into the appropriate output file