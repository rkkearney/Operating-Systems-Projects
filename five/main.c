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
int *FRAME_TABLE_CHANCES;
int FRAME_TABLE_ENTRIES = 0;
int TOTAL_ENTRIES = 0;
struct disk *DISK;
int PAGE_FAULTS=0;
int DISK_READ=0;
int DISK_WRITE=0;

//time_t t;

void rand_algorithm(int * frame){
	// randomly choose a frame number to evict 
	//srand((unsigned)time(&t));
	*frame = rand() % (NFRAMES);
	//printf("%d\t", rand());
}

void fifo_algorithm(int * frame){
	// queue implementation which stores the page to evict
	*frame = TOTAL_ENTRIES % NFRAMES;
} 

void custom_algorithm(int * frame){
	int curr_frame = TOTAL_ENTRIES % NFRAMES;
	while (1) {
		if (FRAME_TABLE_CHANCES[curr_frame]) {
			FRAME_TABLE_CHANCES[curr_frame] = 0;
		} else {
			*frame = curr_frame;
			break;
		}
		curr_frame += 1;
		if (curr_frame == NFRAMES) {
			curr_frame = 0;
		}
	}
}

void initialize_frame_table() {
	FRAME_TABLE = malloc(NFRAMES*sizeof(int *));
	int i;
	for (i = 0; i < NFRAMES; i++) {
		FRAME_TABLE[i] = -1;
	}
}

void initialize_chance_table() {
	FRAME_TABLE_CHANCES = malloc(NFRAMES*sizeof(int *));
	int i;
	for (i = 0; i < NPAGES; i++) {
		FRAME_TABLE_CHANCES[i] = 0;
	}
}

// we could pass in the correct function to call and the frame number to evict 
void page_fault_handler( struct page_table *pt, int page)
{
	//printf("page=%d\n", page);
	
	int frame;
	int bits;
	char *physmem = page_table_get_physmem(pt);
	PAGE_FAULTS += 1;
	//printf("Page Fault\n");

	// start by reading a page from virtual memory 
	page_table_get_entry(pt, page, &frame, &bits);
	
	if (FRAME_TABLE_ENTRIES < NFRAMES && FRAME_TABLE[frame] != page) {
		int i;
		for (i = 0; i < NFRAMES; i++) {
			if (FRAME_TABLE[i] == -1) {
				page_table_set_entry(pt, page, i, PROT_READ);
				FRAME_TABLE[i] = page;
				FRAME_TABLE_ENTRIES += 1;
				TOTAL_ENTRIES += 1;
				FRAME_TABLE_CHANCES[i] = 1;
				//printf("FRAME_TABLE[%d] = %d\n", i, page);
				return;
			}
		}
	}
	
	
	page_table_get_entry(pt, page, &frame, &bits);
	if (bits != PROT_READ) {
		// not just rand but dependent on ALGORITHM 
		//rand_algorithm(&frame);
		if(!strcmp(ALGORITHM,"rand")) {
			rand_algorithm(&frame);
		} else if(!strcmp(ALGORITHM,"fifo")) {
			fifo_algorithm(&frame);
		} else if(!strcmp(ALGORITHM,"custom")) {
			custom_algorithm(&frame);
		} else {
			fprintf(stderr,"unknown algorithm: %s\n",ALGORITHM);
			return;
		}
		//printf("frame=%d\n", frame);
		int evict_bits;
		int evict_frame;
		int evict_page = FRAME_TABLE[evict_frame];
		page_table_get_entry(pt, evict_page, &evict_frame, &evict_bits);
		//printf("evict_page=%d\tevict_frame=%d\tevict_bits=%d \t", evict_page, evict_frame, evict_bits);
		if (evict_bits == 3) {
			disk_write(DISK, evict_page, &physmem[evict_frame*PAGE_SIZE]);
			//printf("write occured");
			DISK_WRITE += 1;
		}
		disk_read(DISK, page, &physmem[frame*PAGE_SIZE]);
		DISK_READ += 1;
		page_table_set_entry(pt, page, frame, PROT_READ);
		FRAME_TABLE_CHANCES[frame] = 1;
		page_table_set_entry(pt, evict_page, frame, 0);
		FRAME_TABLE[frame] = page;
		TOTAL_ENTRIES += 1;
		//printf("\n");
	} else if (FRAME_TABLE[frame] == page){
		page_table_set_entry(pt, page, frame, PROT_READ|PROT_WRITE);
		//printf("set to read|write\n");
	} 
	//printf("Page Faults:\t%d\nDisk Reads:\t%d\nDisk Writes:\t%d\n", PAGE_FAULTS, DISK_READ, DISK_WRITE);
}

int main( int argc, char *argv[] )
{
	if(argc!=5) {
		printf("use: virtmem <NPAGES> <NFRAMES> <rand|fifo|lru|custom> <sort|scan|focus>\n");
		return 1;
	}

	//printf("PROT_READ: %d\tPROT_WRITE: %d\tPROT_READ|PROT_WRITE: %d\t\n", PROT_READ, PROT_WRITE, (PROT_READ|PROT_WRITE));

	NPAGES = atoi(argv[1]);
	NFRAMES = atoi(argv[2]);
	//printf("NPAGES=%d\tNFRAMES=%d\n", NPAGES, NFRAMES);
	ALGORITHM = argv[3];
	PROGRAM = argv[4];

	initialize_frame_table();
	initialize_chance_table();

	DISK = disk_open("myvirtualdisk", NPAGES);
	if(!DISK) {
		fprintf(stderr,"couldn't create virtual disk: %s\n", strerror(errno));
		return 1;
	}

	struct page_table *pt = page_table_create( NPAGES, NFRAMES, page_fault_handler );
	if(!pt) {
		fprintf(stderr,"couldn't create page table: %s\n", strerror(errno));
		return 1;
	}

	char *virtmem = page_table_get_virtmem(pt);
	//char *physmem = page_table_get_physmem(pt);

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

	printf("Page Faults:\t%d\nDisk Reads:\t%d\nDisk Writes:\t%d\n", PAGE_FAULTS, DISK_READ, DISK_WRITE);

	page_table_delete(pt);
	disk_close(DISK);

	return 0;
}