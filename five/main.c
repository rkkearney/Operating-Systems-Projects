/*
Main program for the virtual memory project.
Make all of your modifications to this file.
You may add or rearrange any code or data as you need.
The header files page_table.h and disk.h explain
how to use the page table and disk interfaces.
*/

#include "page_table.h"
#include "disk.h"
#include "program.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

// Global Variables 
int NPAGES;
int NFRAMES;
const char *ALGORITHM;
const char *PROGRAM;


void page_fault_handler( struct page_table *pt, int page )
{
	//printf("page fault on page #%d\n",page);
	//exit(1);

	int frame, bits;
	char *physmem = page_table_get_physmem (pt);

	// frame array where index is frame and page is value 

	// start by reading a page from virtual memory 
	// this read results in a page fault - look at existing permissions
	// conclude fault occurs due to mising permisions 
	// no permissions? add PROT_READ
	// PROT READ? Add PROT_WRITE 
	// page fault handler should choose a free frame
	// adjust page table to map page to frame with correct permissions
	// load correct page from disk into correct frame on physical memory 

}

void rand_algorithm(){

}

void fifo_algorithm(){

}

void custom_algorithm(){

}

int main( int argc, char *argv[] )
{
	if(argc!=5) {
		printf("use: virtmem <NPAGES> <NFRAMES> <rand|fifo|lru|custom> <sort|scan|focus>\n");
		return 1;
	}

	NPAGES = atoi(argv[1]);
	NFRAMES = atoi(argv[2]);
	ALGORITHM = argv[3];
	PROGRAM = argv[4];

	struct disk *disk = disk_open("myvirtualdisk", NPAGES);
	if(!disk) {
		fprintf(stderr,"couldn't create virtual disk: %s\n", strerror(errno));
		return 1;
	}

	struct page_table *pt = page_table_create( NPAGES, NFRAMES, page_fault_handler );
	if(!pt) {
		fprintf(stderr,"couldn't create page table: %s\n", strerror(errno));
		return 1;
	}

	char *virtmem = page_table_get_virtmem(pt);

	char *physmem = page_table_get_physmem(pt);

	if(!strcmp(PROGRAM,"sort")) {
		sort_program(virtmem, NPAGES*PAGE_SIZE);

	} else if(!strcmp(PROGRAM,"scan")) {
		scan_program(virtmem, NPAGES*PAGE_SIZE);

	} else if(!strcmp(PROGRAM,"focus")) {
		focus_program(virtmem, NPAGES*PAGE_SIZE);

	} else {
		fprintf(stderr,"unknown program: %s\n",argv[4]);
		return 1;
	}

	page_table_delete(pt);
	disk_close(disk);

	return 0;
}