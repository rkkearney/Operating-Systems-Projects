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
const char *ALGORITHM;
int PAGE_FAULTS;
int DISK_READS;
int DISK_WRITES;
int NPAGES;
int NFRAMES;

void page_fault_handler( struct page_table *pt, int page )
{
	page_table_set_entry(pt, page, page, PROT_READ|PROT_WRITE);
	
	if(!strcmp(ALGORITHM,"rand")) {
		
		// have to start by reading a page --> gives a page fault
		// depending on the page fault we know what to do 
		// then need to check if a frame is free
		
		srand(time(NULL));
		int frame = rand() % NFRAMES;
		page_table_set_entry(pt, page, frame, PROT_READ);
		disk_read(disk, page, &physmem[frame*frame_size]);

	} 
	/*else if(!strcmp(ALGORITHM,"fifo")) {
		
	} else if(!strcmp(ALGORITHM,"custom")) {
		
	} else {
		fprintf(stderr,"unknown algorithm: %s\n",argv[3]);
		return 1;
	}*/
}

int main( int argc, char *argv[] )
{
	if(argc!=5) {
		printf("use: virtmem <npages> <nframes> <rand|fifo|lru|custom> <sort|scan|focus>\n");
		return 1;
	}

	NPAGES = atoi(argv[1]);
	NFRAMES = atoi(argv[2]);
	ALGORITHM = argv[3];
	const char *program = argv[4];

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

	if(!strcmp(program,"sort")) {
		sort_program(virtmem, NPAGES*PAGE_SIZE);

	} else if(!strcmp(program,"scan")) {
		scan_program(virtmem, NPAGES*PAGE_SIZE);

	} else if(!strcmp(program,"focus")) {
		focus_program(virtmem, NPAGES*PAGE_SIZE);

	} else {
		//fprintf(stderr,"unknown program: %s\n",argv[3]);
		fprintf(stderr,"unknown program: %s\n",argv[4]);
		return 1;
	}

	page_table_delete(pt);
	disk_close(disk);

	return 0;
}
