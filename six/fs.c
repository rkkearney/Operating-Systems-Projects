#include "fs.h"
#include "disk.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

#define FS_MAGIC           0xf0f03410
#define INODES_PER_BLOCK   128			// number of inodes per block
#define POINTERS_PER_INODE 5			// number of pointers in each inode structure
#define POINTERS_PER_BLOCK 1024			// number of pointers found in an indirect block

// GLOBAL VARIABLES 
int MOUNT = 0;
int *FREE_BLOCK_BITMAP;
int *INODE_TABLE;

struct fs_superblock {
	int magic;			// magic number
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

// use inode size to compute number of data blocks for direct/indirect 
int get_num_data_blocks(int size) {
	int num_data_blocks = size/4096;
	if (size%4096) num_data_blocks++;
	return num_data_blocks;
}

// build free block bitmap by scanning inodes 
void initialize_free_block_bitmap(int nblocks, int ninodeblocks) {
	FREE_BLOCK_BITMAP = malloc(sizeof(int *)*nblocks);
	int i;
	
	// mark 0 if block is in use
	for (i = 0; i <= ninodeblocks; i++) {
		FREE_BLOCK_BITMAP[i] = 0;
	}
	// mark 1 if block is free 
	for (i = ninodeblocks+1; i < nblocks; i++) {
		FREE_BLOCK_BITMAP[i] = 1;
	}
}

// used in fs_write to locate available block to write to
int find_free_block(int nblocks) {
	int blocknum = 1;
	// return first available block
	while (blocknum < nblocks) {
		if (FREE_BLOCK_BITMAP[blocknum] == 1) {
			return blocknum;
		} else {
			blocknum++;
		}
	}
	// check if all blocks are full
	return -1;
}

// build inode table to store which inodes are in use
void initialize_inode_table(int ninodes) {
	INODE_TABLE = malloc(sizeof(int *)*ninodes);
	int i;
	INODE_TABLE[0] = 1;	// inode 0 is root and unavailable for use
	// initalize all inodes to 0 for available
	for (i = 1; i < ninodes; i++) {
		INODE_TABLE[i] = 0;
	}
}

// Scan a mounted file system; report on how blocks organized
void fs_debug()
{
	union fs_block block;
	// read data from super block
	disk_read(0,block.data);

	// print superblock information
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

	// iterate through all inode blocks reading data from disk 
	for (ib = 0; ib < block.super.ninodeblocks; ib++) {
		disk_read(ib+1, block.data);
		
		// parse data from each inode and print information accordingly 
		for (i = 0; i < INODES_PER_BLOCK; i++) {
			if (block.inode[i].isvalid) {
				printf("inode %d:\n", i);
				printf("    size: %d bytes\n",block.inode[i].size);
				
				// use size to compute number of direct and indirect blocks
				num_data_blocks = get_num_data_blocks(block.inode[i].size);
				
				// if more than 5 blocks; indirect blocks exist
				if (num_data_blocks > 5) {
					num_direct_blocks = 5;
					num_indirect_blocks = num_data_blocks - 5;
				} else {
					num_direct_blocks = num_data_blocks;
					num_indirect_blocks = 0;
				}

				// print direct block information
				if (block.inode[i].size) {
					printf("    direct blocks: ");
					for (db = 0; db < num_direct_blocks; db++) {
						printf("%d ", block.inode[i].direct[db]);
					}
					printf("\n");
					
					// if indirect blocks exist, get and print those blocks
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

// create a new filesystem, clears inode table, writes superblock
int fs_format()
{
	// check if mounted; can't format already mounted disk 
	if (MOUNT) {
		return 0;
	} else {
		// create and write superblock information to disk 
		union fs_block sblock;
		sblock.super.magic = FS_MAGIC;
		sblock.super.nblocks = disk_size();
		sblock.super.ninodeblocks = .10 * sblock.super.nblocks; // set aside 10% of blocks for inodes
		sblock.super.ninodes = sblock.super.ninodeblocks * INODES_PER_BLOCK;
		disk_write(0, sblock.data);

		// initialize all inodes to invalid 
		int i, ib;
		union fs_block iblock;
		for (i = 0; i < INODES_PER_BLOCK; i++) {
			iblock.inode[i].isvalid = 0;
		}

		// write inode data to disk
		for (ib = 0; ib < sblock.super.ninodeblocks; ib++) {
			disk_write(ib+1, iblock.data);
		}

		return 1;
	}
}

// prepare filesystem for use 
int fs_mount()
{
	union fs_block block;
	disk_read(0, block.data);

	// filesystem present if valid superblock magic number 
	// can't remount a filesystem 
	if (block.super.magic && !MOUNT) {
		
		// initialize free block bitmap and inode table to prepare for use
		initialize_free_block_bitmap(block.super.nblocks, block.super.ninodeblocks);
		initialize_inode_table(block.super.ninodes);
		
		// iterate through all inode blocks reading data from disk 
		int i, ib;		
		for (ib = 0; ib < block.super.ninodeblocks; ib++) {
			
			union fs_block iblock;
			disk_read(ib+1, iblock.data);
			
			// parse data from each inode and print information accordingly 
			for (i = 0; i < INODES_PER_BLOCK; i++) {
				if (iblock.inode[i].isvalid) {
					// if an inode is valid mark it unavailable in inode table 
					INODE_TABLE[i] = 1;

					// use size to compute number of direct and indirect blocks
					int num_data_blocks = get_num_data_blocks(iblock.inode[i].size);
					int num_direct_blocks = 0, num_indirect_blocks = 0;

					// if more than 5 blocks; indirect blocks exist
					if (num_data_blocks > 5) {
						num_direct_blocks = 5;
						num_indirect_blocks = num_data_blocks - 5;
					} else {
						num_direct_blocks = num_data_blocks;
						num_indirect_blocks = 0;
					}

					// set up free block btimap given direct data blocks
					int db;
					for (db = 0; db < num_direct_blocks; db++) {
						FREE_BLOCK_BITMAP[iblock.inode[i].direct[db]] = 0;
					}
					
					// if indirect blocks exist, update bitmap with blocks
					if (num_indirect_blocks) {
						union fs_block indirect_block;
						FREE_BLOCK_BITMAP[iblock.inode[i].indirect] = 0;
						disk_read(iblock.inode[i].indirect, indirect_block.data);
						int idb;
						for (idb = 0; idb < num_indirect_blocks; idb++) {
							FREE_BLOCK_BITMAP[indirect_block.pointers[idb]] = 0;
						}
					}
				}
			}
		}
		MOUNT = 1;	// filesystem now mounted 
		return 1;
	} else {
		return 0;
	}
}

// create a new inode of length 0
int fs_create()
{
	// read superblock data from disk
	union fs_block block;
	disk_read(0, block.data);

	// check filesystem has been mounted
	if (MOUNT) {
		int i;
		// iterate through inodes
		for (i = 0; i < block.super.ninodes; i++) {
			// find the first available inode in inode table
			if (!INODE_TABLE[i]) {
				union fs_block iblock;
				int blocknum = (i/INODES_PER_BLOCK )+ 1;
				// read inode data from disk 
				disk_read(blocknum, iblock.data);
				// reset inode data to valid inode with size of 0
				iblock.inode[i%INODES_PER_BLOCK].isvalid = 1;
				iblock.inode[i%INODES_PER_BLOCK].size = 0;
				// update inode table to inode unavailable
				INODE_TABLE[i] = 1;
				// write new inode information to disk
				disk_write(blocknum, iblock.data);
				// return newly created inode number 
				return i;
			}
		}
	}
	return 0;
}

// delete inode indicated by inode number
int fs_delete( int inumber )
{	
	// check filesystem has been mounted and inode is valid 
	if (MOUNT && INODE_TABLE[inumber]) {
		int blocknum = inumber/INODES_PER_BLOCK + 1;
		int inode_index = (inumber)%INODES_PER_BLOCK;

		union fs_block block;
		// read inode information from disk 
		disk_read(blocknum, block.data);	

		// mark inode available in inode table and set inode to invalid
		INODE_TABLE[inumber] = 0;
		block.inode[inode_index].isvalid = 0;

		int size = block.inode[inode_index].size;
		block.inode[inode_index].size = 0;

		// use size of inode to compute number of direct and indirect blocks
		int num_data_blocks = get_num_data_blocks(size);
		int num_direct_blocks, num_indirect_blocks;

		// if more than 5 blocks; indirect blocks exist
		if (num_data_blocks > 5) {
			num_direct_blocks = 5;
			num_indirect_blocks = num_data_blocks - 5;
		} else {
			num_direct_blocks = num_data_blocks;
			num_indirect_blocks = 0;
		}

		// free direct and indirect blocks by setting bitmap to 1 for free block 
		int db, idb;
		for (db = 0; db < num_direct_blocks; db++) {
			FREE_BLOCK_BITMAP[block.inode[inode_index].direct[db]] = 1;
		}
	
		FREE_BLOCK_BITMAP[block.inode[inode_index].indirect] = 1;

		if (num_indirect_blocks) {
			union fs_block indirect_block;
			disk_read(block.inode[inode_index].indirect, indirect_block.data);
			for (idb = 0; idb < num_indirect_blocks; idb++) {
				FREE_BLOCK_BITMAP[indirect_block.pointers[idb]] = 1;
			}
		}

		// write newly deleted inode information to disk 
		disk_write(blocknum, block.data);
		return 1;
	}
	return 0;
}

// return the logical size of the given inode, in bytes
int fs_getsize( int inumber )
{
	// check filesystem has been mounted and inode is valid 
	if (MOUNT && INODE_TABLE[inumber]) {
		// compute index in data
		int blocknum = inumber/INODES_PER_BLOCK + 1;
		int inode_index = (inumber)%INODES_PER_BLOCK;
		// read inode information from disk 
		union fs_block block;
		disk_read(blocknum, block.data);
		// return the logical size of given inode in bytes
		return block.inode[inode_index].size;
	}
	return -1;
}

// read data from a valid inode 
int fs_read( int inumber, char *data, int length, int offset )
{
	// check filesystem has been mounted and inode is valid 
	if (MOUNT && INODE_TABLE[inumber]) {
		int bytes_read = 0;
		int blocknum = inumber/INODES_PER_BLOCK + 1;
		int inode_index = (inumber)%INODES_PER_BLOCK;

		union fs_block inode_block;

		// check if inode has a size greater than 0 to read from
		if (fs_getsize(inumber)) {
			// read data from appropriate block
			disk_read(blocknum, inode_block.data);
			
			// get total size of data to read 
			int size = inode_block.inode[inode_index].size;
			
			// compute amount of bytes to read given offset
			int stop_read = size - offset;
			if (stop_read <= 0) return 0;

			int num_data_blocks = get_num_data_blocks(size);
			int num_direct_blocks, num_indirect_blocks;

			// if more than 5 blocks; indirect blocks exist
			if (num_data_blocks > 5) {
				num_direct_blocks = 5;
				num_indirect_blocks = num_data_blocks - 5;
			} else {
				num_direct_blocks = num_data_blocks;
				num_indirect_blocks = 0;
			}

			union fs_block data_block;
			int direct_block_index = offset/4096;
			
			int i;
			// if offset is greater than (5*4096) read indirect block 
			if (direct_block_index >= num_direct_blocks && num_indirect_blocks) {
				int indirect_block_index = direct_block_index - num_direct_blocks;
				union fs_block indirect_data_block;
				
				disk_read(inode_block.inode[inumber].indirect, indirect_data_block.data);
				disk_read(indirect_data_block.pointers[indirect_block_index], data_block.data);
				
				for (i = 0; i < DISK_BLOCK_SIZE; i++) {
					data[i] = data_block.data[i];
					bytes_read++;
					// stop if total bytes have been read 
					if (bytes_read == stop_read) break;
				}
			} else {
				// read direct block at index from offset 
				disk_read(inode_block.inode[inumber].direct[direct_block_index], data_block.data);
				
				for (i = 0; i < DISK_BLOCK_SIZE; i++) {
					data[i] = data_block.data[i];
					bytes_read++;
					// stop if total bytes have been read 
					if (bytes_read == stop_read) break;
				}
			}
		}
		return bytes_read;
	}
	return 0;
}

// write data to a valid inode
int fs_write( int inumber, const char *data, int length, int offset )
{
	int original_length = length;
	int offset_index = offset;
	if (offset >= original_length) {
		original_length += offset;
		offset_index = 0;
	}
	
	// check filesystem has been mounted and inode is valid 
	if (MOUNT && INODE_TABLE[inumber]) {
		int total_bytes_written = 0;
		union fs_block super_block;
		disk_read(0, super_block.data);

		int blocknum = inumber/INODES_PER_BLOCK + 1;
		int inode_index = inumber%INODES_PER_BLOCK;
			
		union fs_block inode_block;
		disk_read(blocknum, inode_block.data);

		if (inode_block.inode[inode_index].size != offset) {
			int size = inode_block.inode[inode_index].size;
			int num_data_blocks = get_num_data_blocks(size);
			int num_direct_blocks, num_indirect_blocks;

			// if more than 5 blocks; indirect blocks exist
			if (num_data_blocks > 5) {
				num_direct_blocks = 5;
				num_indirect_blocks = num_data_blocks - 5;
			} else {
				num_direct_blocks = num_data_blocks;
				num_indirect_blocks = 0;
			}

			// free direct and indirect blocks by setting bitmap to 1 for free block 
			int db, idb;
			for (db = 0; db < num_direct_blocks; db++) {
				FREE_BLOCK_BITMAP[inode_block.inode[inode_index].direct[db]] = 1;
			}

			FREE_BLOCK_BITMAP[inode_block.inode[inode_index].indirect] = 1;
		
			if (num_indirect_blocks) {
				union fs_block indirect_block;
				disk_read(inode_block.inode[inode_index].indirect, indirect_block.data);
				for (idb = 0; idb < num_indirect_blocks; idb++) {
					FREE_BLOCK_BITMAP[indirect_block.pointers[idb]] = 1;
				}
			}
			inode_block.inode[inode_index].size = 0;
		}
		
		while (1) {
			int bytes_written = 0;
			int stop_write = original_length - offset;
			if (stop_write <= 0) break;

			union fs_block data_block;
			int direct_block_index = offset/4096;

			// read data from direct blocks 
			if (direct_block_index < POINTERS_PER_INODE) {
				// find available blocks to write to 
				int direct_blocknum = find_free_block(super_block.super.nblocks);
				if (direct_blocknum < 0) {
					printf("ERROR: not enough blocks on disk - not all bytes written\n");
					return total_bytes_written;
				} else {
					FREE_BLOCK_BITMAP[direct_blocknum] = 0;
				}
				
				inode_block.inode[inode_index].direct[direct_block_index] = direct_blocknum;

				int i;
				for (i = 0; i < DISK_BLOCK_SIZE; i++) {
					data_block.data[i] = data[offset_index+i];
					bytes_written++;
					if (bytes_written == stop_write) break;
				}
				disk_write(direct_blocknum, data_block.data);
			
			} else {
				// if there are indirect blocks read indirect blocks
				int indirect_block_index = direct_block_index - POINTERS_PER_INODE;
				int indirect_blocknum;
				union fs_block indirect_block;

				if (indirect_block_index == 0) {
					// find data block for pointers		
					indirect_blocknum = find_free_block(super_block.super.nblocks);

					if (indirect_blocknum < 0) {
						printf("ERROR: not enough blocks on disk - not all bytes written\n");
						return total_bytes_written;
					} else {
						FREE_BLOCK_BITMAP[indirect_blocknum] = 0;
					}

					inode_block.inode[inode_index].indirect = indirect_blocknum;
				} else {
					indirect_blocknum = inode_block.inode[inode_index].indirect;
					disk_read(indirect_blocknum, indirect_block.data);
				}

				int indirect_data_blocknum = find_free_block(super_block.super.nblocks);


				if (indirect_data_blocknum < 0){
					printf("ERROR: not enough blocks on disk - not all bytes written\n");
					return total_bytes_written;
				} else {
					FREE_BLOCK_BITMAP[indirect_data_blocknum] = 0;
				}

				// update pointers 
				indirect_block.pointers[indirect_block_index] = indirect_data_blocknum;
				disk_write(indirect_blocknum, indirect_block.data);

				// write data to inode 
				int i;
				for (i = 0; i < DISK_BLOCK_SIZE; i++) {
					data_block.data[i] = data[offset_index+i];
					bytes_written++;
					if (bytes_written == stop_write) break;
				}
				disk_write(indirect_data_blocknum, data_block.data);
			}

			inode_block.inode[inode_index].size += bytes_written;
			disk_write(blocknum, inode_block.data);

			// increment total number of bytes written by bytes written on this loop
			total_bytes_written += bytes_written;
			length -= bytes_written;	// decrement length by bytes written
			offset += bytes_written;	// increment offset by bytes written

			// once the correct total has been written, return total bytes 
			if (total_bytes_written == original_length) return total_bytes_written;
		}

		return total_bytes_written;
	}
	return 0;
}
