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

int MOUNT = 0;
int *FREE_BLOCK_BITMAP;
int *INODE_TABLE;

struct fs_superblock {
	int magic;
	int nblocks;		// number of blocks on disk
	int ninodeblocks;	// number of inode blocks
	int ninodes;		// number of inodes total
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
	if (MOUNT) {
		return 0;
	} else {
		union fs_block sblock;
		sblock.super.magic = FS_MAGIC;
		sblock.super.nblocks = disk_size();
		sblock.super.ninodeblocks = .10 * sblock.super.nblocks;
		sblock.super.ninodes = sblock.super.ninodeblocks * INODES_PER_BLOCK;
		disk_write(0, sblock.data);

		int i, ib;
		union fs_block iblock;
		for (i = 0; i < INODES_PER_BLOCK; i++) {
			iblock.inode[i].isvalid = 0;
		}

		for (ib = 0; ib < sblock.super.ninodeblocks; ib++) {
			disk_write(ib+1, iblock.data);
		}

		return 1;
	}
}

void fs_debug()
{
	// scan a mounted file system 
	// report how inodes and blocks are organized 
	union fs_block block;
	//union fs_block iblock;
	disk_read(0,block.data);

	printf("superblock:\n");
	if (block.super.magic)
		printf("    magic number is valid\n");
	else		
		printf("    magic number is invalid\n");
	printf("    %d blocks on disk\n", block.super.nblocks);
	printf("    %d inode blocks\n", block.super.ninodeblocks);
	printf("    %d inodes total\n", block.super.ninodes);

	int ib, i, db, idb;
	int num_data_blocks = 0, num_indirect_blocks = 0, num_direct_blocks = 0;

	for (ib = 0; ib < block.super.ninodeblocks; ib++) {
		disk_read(ib+1, block.data);
		
		for (i = 0; i < INODES_PER_BLOCK; i++) {
			if (block.inode[i].isvalid) {
				printf("inode %d:\n", i+1);
				printf("    size: %d bytes\n",block.inode[i].size);
				
				num_data_blocks = get_num_data_blocks(block.inode[i].size);
				
				if (num_data_blocks > 5) {
					num_direct_blocks = 5;
					num_indirect_blocks = num_data_blocks - 5;
				} else {
					num_direct_blocks = num_data_blocks;
					num_indirect_blocks = 0;
				}

				if (block.inode[i].size) {
					printf("    direct blocks: ");
					for (db = 0; db < num_direct_blocks; db++) {
						printf("%d ", block.inode[i].direct[db]);
					}
					printf("\n");
					
					if (num_indirect_blocks) {
						union fs_block indirect_block;
						disk_read(block.inode[i].indirect, indirect_block.data);

						printf("    indirect block: %d\n",block.inode[i].indirect);
						printf("    indirect data blocks: ");
						for (idb = 0; idb < num_indirect_blocks; idb++) {
							printf("%d ", indirect_block.pointers[idb]);
						}
						printf("\n");
					}
				}
			}
		}
	}
}

int get_num_data_blocks(int size) {
	int num_data_blocks = size/4096;
	if (size%4096) num_data_blocks++;
	return num_data_blocks;
}

void initialize_free_block_bitmap(int nblocks) {
	FREE_BLOCK_BITMAP = malloc(sizeof(int *)*nblocks);
	int i;
	for (i = 0; i < nblocks; i++) {
		FREE_BLOCK_BITMAP[i] = 1;
	}
}

void initialize_inode_table(int ninodes) {
	INODE_TABLE = malloc(sizeof(int *)*ninodes);
	int i;
	//INODE_TABLE[0] = 1;
	for (i = 0; i < ninodes; i++) {
		INODE_TABLE[i] = 0;
	}
}

int fs_mount()
{
	// examine disk for a filesystem
	union fs_block block;
	disk_read(0, block.data);

	if (block.super.magic) {
		
		initialize_free_block_bitmap(block.super.nblocks);
		initialize_inode_table(block.super.ninodes);
		
		int i, ib;		
		for (ib = 0; ib < block.super.ninodeblocks; ib++) {
			
			union fs_block iblock;
			disk_read(ib+1, iblock.data);
			
			for (i = 0; i < INODES_PER_BLOCK; i++) {
				if (iblock.inode[i].isvalid) {
					INODE_TABLE[i] = 1;

					int num_data_blocks = get_num_data_blocks(block.inode[i].size);
					int num_direct_blocks, num_indirect_blocks;

					if (num_data_blocks > 5) {
						num_direct_blocks = 5;
						num_indirect_blocks = num_data_blocks - 5;
					} else {
						num_direct_blocks = num_data_blocks;
						num_indirect_blocks = 0;
					}

					int db;
					for (db = 0; db < num_direct_blocks; db++) {
						FREE_BLOCK_BITMAP[block.inode[i].direct[db]] = 0;
					}
					
					if (num_indirect_blocks) {
						union fs_block indirect_block;
						disk_read(block.inode[i].indirect, indirect_block.data);

						int idb;
						for (idb = 0; idb < num_indirect_blocks; idb++) {
							FREE_BLOCK_BITMAP[indirect_block.pointers[idb]] = 0;
						}
					}
				}
			}
		}
		MOUNT = 1;
		return 1;
	} else {
		return 0;
	}
	
	// if disk present, read superblock, build free block bitmap, prepare fs for use
	// build free block bitmap
	// scan through all inodes and record which blocks are in use 
}

int fs_create()
{
	union fs_block block;
	disk_read(0, block.data);

	if (MOUNT) {
		int i;
		for (i = 0; i < block.super.ninodes; i++) {
			if (!INODE_TABLE[i]) {
				union fs_block iblock;
				int blocknum = (i/INODES_PER_BLOCK )+ 1;
				disk_read(blocknum, iblock.data);
				iblock.inode[i%INODES_PER_BLOCK].isvalid = 1;
				iblock.inode[i%INODES_PER_BLOCK].size = 0;
				INODE_TABLE[i] = 1;
				disk_write(blocknum, iblock.data);
				return i+1;
			}
		}
	}
	return 0;
}

int fs_delete( int inumber )
{	
	if (MOUNT && INODE_TABLE[inumber-1]) {
		int blocknum = inumber/INODES_PER_BLOCK + 1;
		int inode_index = (inumber-1)%INODES_PER_BLOCK;

		union fs_block block;
		disk_read(blocknum, block.data);	

		INODE_TABLE[inumber-1] = 0;
		block.inode[inode_index].isvalid = 0;

		int size = block.inode[inode_index].size;
		block.inode[inode_index].size = 0;

		int num_data_blocks = get_num_data_blocks(size);
		int num_direct_blocks, num_indirect_blocks;

		if (num_data_blocks > 5) {
			num_direct_blocks = 5;
			num_indirect_blocks = num_data_blocks - 5;
		} else {
			num_direct_blocks = num_data_blocks;
			num_indirect_blocks = 0;
		}

		int db, idb;
		for (db = 0; db < num_direct_blocks; db++) {
			FREE_BLOCK_BITMAP[block.inode[inode_index].direct[db]] = 1;
		}
	
		if (num_indirect_blocks) {
			union fs_block indirect_block;
			disk_read(block.inode[inode_index].indirect, indirect_block.data);
			for (idb = 0; idb < num_indirect_blocks; idb++) {
				FREE_BLOCK_BITMAP[indirect_block.pointers[idb]] = 1;
			}
		}

		disk_write(blocknum, block.data);
		return 1;
	}
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
