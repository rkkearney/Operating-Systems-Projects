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
#include <time.h>

// Global Variables 
int NPAGES;
int NFRAMES;
const char *ALGORITHM;
const char *PROGRAM;
int *FRAME_TABLE;

void rand_algorithm(int * frame){
	// randomly choose a frame number to evict 
	srand(time(NULL));
	*frame = rand() % (NFRAMES);
}

void fifo_algorithm(){
	// queue implementation which stores the page to evict 
}

void custom_algorithm(){

}

void initialize_frame_table() {
	FRAME_TABLE = malloc(NFRAMES*sizeof(int *));
	int i;
	for (i = 0; i < NFRAMES; i++) {
		FRAME_TABLE[i] = -1;
	}
}

// we could pass in the correct function to call and the frame number to evict 
void page_fault_handler( struct page_table *pt, int page, char *physmem)
{
	
	int frame;
	int bits;
	//char *physmem = page_table_get_physmem(pt);

	// start by reading a page from virtual memory 
	page_table_get_entry(pt, page, &frame, &bits);
		
	int i;
	for (i = 0; i < NFRAMES; i++) {
		if (FRAME_TABLE[i] < 0) {
			page_table_set_entry(pt, page, i, PROT_READ);
			FRAME_TABLE[i] = page;
			return;
		}
	}
	
	page_table_get_entry(pt, page, &frame, &bits);
	if (FRAME_TABLE[frame] == page && bits == PROT_READ && bits == PROT_READ|PROT_WRITE){
		page_table_set_entry(pt, page, frame, PROT_READ|PROT_WRITE);
	} else {
		// not just rand but dependent on ALGORITHM 
		rand_algorithm(&frame);
		int evict_bits;
		int evict_frame;
		int evict_page = FRAME_TABLE[frame];
		page_table_get_entry(pt, evict_page, &evict_frame, &evict_bits);
		if (evict_bits == PROT_WRITE) {
			disk_write(disk, evict_page, &physmem[evict_frame*PAGE_SIZE]);
		}
		disk_read(disk, page, &physmem[frame*PAGE_SIZE]);
		page_table_set_entry(pt, page, frame, PROT_READ);
		page_table_set_entry(pt, evict_page, 0, 0);
		FRAME_TABLE[frame] = page;
	}
	
	// this read results in a page fault - look at existing permissions
	// conclude fault occurs due to mising permisions 
	// no permissions? add PROT_READ
	// PROT READ? Add PROT_WRITE 
	// page fault handler should choose a free frame
	// adjust page table to map page to frame with correct permissions
	// load correct page from disk into correct frame on physical memory 
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

	initialize_frame_table();

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