#include "fs.h"
#include "disk.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

#define FS_MAGIC           0xf0f03410
#define INODES_PER_BLOCK   128
#define POINTERS_PER_INODE 5			// number of pointers in each inode structure
#define POINTERS_PER_BLOCK 1024			// number of pointers found in an indirect block

struct fs_superblock {
	int magic;
	int nblocks;
	int ninodeblocks;
	int ninodes;
};

struct fs_inode {
	int isvalid;
	int size;
	int direct[POINTERS_PER_INODE];
	int indirect;
};

union fs_block {
	struct fs_superblock super;
	struct fs_inode inode[INODES_PER_BLOCK];
	int pointers[POINTERS_PER_BLOCK];
	char data[DISK_BLOCK_SIZE];
};

void inode_load( int number, struct fs_inode *inode ) {
	// given inode number
	// calculate offset into inode region (# * sizeof(inode))
	// add to start address of inode table on the disk 

	// FROM TEXTBOOK
	/*blk = (inumber * sizeof(inode_t)) / blockSize;
	sector = ((blk * blockSize) + inodeStartAddr) / sectorSize;*/
}

void inode_save ( int number, struct fs_inode *inode) {

}

int fs_format()
{
	// creates a new filesystem on the disk, destroys present data 
	// sets aside 10% of blocks for inodes
	// clears inode tables
	// writes the superblock
	// returns 1 success, zero otherwise 
	return 0;
}

void fs_debug()
{
	// scan a mounted file system 
	// report how inodes and blocks are organized 
	union fs_block block;
	disk_read(0,block.data);

	printf("superblock:\n");
	printf("    %d blocks on disk\n",block.super.nblocks);
	printf("    %d inode blocks\n",block.super.ninodeblocks);
	printf("    %d inodes total\n",block.super.ninodes);
	
}

int fs_mount()
{
	// examine disk for a filesystem 
	
	// if disk present, read superblock, build free block bitmap, prepare fs for use
	
	// check if disk present and read superblock 
	disk_read(0, block.data);
	// do somethiing to get superblock data
	// build free block bitmap
	// scan through all inodes and record which blocks are in use 



	// return one on success, zero otherwise
	return 0;
}

int fs_create()
{
	// create a new inode of zero length 
	// on success, return the (positive) inumber
	// on failure, return 0 (0 cannot be a valid inumber)
	return 0;
}

int fs_delete( int inumber )
{
	// delete the inode indicated by the inumber
	// release all data and indirect blocks assigned to the inode 
	// return them to free block map
	// on success, return 1
	// on failure, return 0
	return 0;
}

int fs_getsize( int inumber )
{
	// return the logical size of the given inode, in bytes
	// on failure, return -1
	return -1;
}

int fs_read( int inumber, char *data, int length, int offset )
{
	// read data from valid inode
	// copy "length" bytes from the inode into "data" pointer, starting at "offset"
	// return the total number of bytes read
	// total number could be less than requested number
	return 0;
}

int fs_write( int inumber, const char *data, int length, int offset )
{
	// write data to valid inode
	// copy "length" bytes from the pointer "data" into the inode, starting at "offset"
	// allocate necessary direct and indirect blocks int he process
	// return the number of bytes actually written
	return 0;
}
