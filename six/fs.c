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

int get_num_data_blocks(int size) {
	int num_data_blocks = size/4096;
	if (size%4096) num_data_blocks++;
	return num_data_blocks;
}

void initialize_free_block_bitmap(int nblocks, int ninodeblocks) {
	FREE_BLOCK_BITMAP = malloc(sizeof(int *)*nblocks);
	int i;
	for (i = 0; i <= ninodeblocks; i++) {
		FREE_BLOCK_BITMAP[i] = 0;
	}
	for (i = ninodeblocks+1; i < nblocks; i++) {
		FREE_BLOCK_BITMAP[i] = 1;
	}

	//print_bitmap(nblocks);
}

void print_bitmap(int nblocks) {
	int i;
	for (i = 0; i < nblocks; i++) {
		printf("%d : %d\n", i, FREE_BLOCK_BITMAP[i]);
	}
}

int find_free_block(int nblocks) {
	int blocknum = 0;
	while (1) {
		if (FREE_BLOCK_BITMAP[blocknum] == 1) break;
		else blocknum++;
	}
	//printf("blocknum = %d\n", blocknum);
	if (blocknum < nblocks) {
		return blocknum;
	}
	return -1;
}

void initialize_inode_table(int ninodes) {
	INODE_TABLE = malloc(sizeof(int *)*ninodes);
	int i;
	INODE_TABLE[0] = 1;
	for (i = 1; i < ninodes; i++) {
		INODE_TABLE[i] = 0;
	}
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
				printf("inode %d:\n", i);
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

int fs_mount()
{
	union fs_block block;
	disk_read(0, block.data);

	if (block.super.magic && !MOUNT) {
		
		initialize_free_block_bitmap(block.super.nblocks, block.super.ninodeblocks);
		initialize_inode_table(block.super.ninodes);
		
		int i, ib;		
		for (ib = 0; ib < block.super.ninodeblocks; ib++) {
			
			union fs_block iblock;
			disk_read(ib+1, iblock.data);
			
			for (i = 0; i < INODES_PER_BLOCK; i++) {
				if (iblock.inode[i].isvalid) {
					INODE_TABLE[i] = 1;

					int num_data_blocks = get_num_data_blocks(iblock.inode[i].size);
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
						FREE_BLOCK_BITMAP[iblock.inode[i].direct[db]] = 0;
					}
					
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
		MOUNT = 1;
		return 1;
	} else {
		return 0;
	}
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
				return i;
			}
		}
	}
	return 0;
}

int fs_delete( int inumber )
{	
	if (MOUNT && INODE_TABLE[inumber]) {
		int blocknum = inumber/INODES_PER_BLOCK + 1;
		int inode_index = (inumber)%INODES_PER_BLOCK;

		union fs_block block;
		disk_read(blocknum, block.data);	

		INODE_TABLE[inumber] = 0;
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
	if (MOUNT && INODE_TABLE[inumber]) {
		int blocknum = inumber/INODES_PER_BLOCK + 1;
		int inode_index = (inumber)%INODES_PER_BLOCK;

		union fs_block block;
		disk_read(blocknum, block.data);

		return block.inode[inode_index].size;
	}
	return -1;
}

int fs_read( int inumber, char *data, int length, int offset )
{
	if (MOUNT && INODE_TABLE[inumber]) {
		int bytes_read = 0;
		int blocknum = inumber/INODES_PER_BLOCK + 1;
		int inode_index = (inumber)%INODES_PER_BLOCK;

		union fs_block inode_block;

		if (fs_getsize(inumber)) {
			disk_read(blocknum, inode_block.data);
			
			int size = inode_block.inode[inode_index].size;
			
			int stop_read = size - offset;
			if (stop_read <= 0) return 0;

			int num_data_blocks = get_num_data_blocks(size);
			int num_direct_blocks, num_indirect_blocks;

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
			if (direct_block_index >= num_direct_blocks && num_indirect_blocks) {
				int indirect_block_index = direct_block_index - num_direct_blocks;
				union fs_block indirect_data_block;
				
				disk_read(inode_block.inode[inumber].indirect, indirect_data_block.data);
				disk_read(indirect_data_block.pointers[indirect_block_index], data_block.data);
				
				for (i = 0; i < DISK_BLOCK_SIZE; i++) {
					data[i] = data_block.data[i];
					bytes_read++;
					if (bytes_read == stop_read) break;
				}
			} else {
				disk_read(inode_block.inode[inumber].direct[direct_block_index], data_block.data);
				
				for (i = 0; i < DISK_BLOCK_SIZE; i++) {
					data[i] = data_block.data[i];
					bytes_read++;
					if (bytes_read == stop_read) break;
				}
			}
		}
		return bytes_read;
	}
	return 0;
}

int fs_write( int inumber, const char *data, int length, int offset )
{
	int original_length = length;
	
	if (MOUNT && INODE_TABLE[inumber]) {
		int total_bytes_written = 0;
		union fs_block super_block;
		disk_read(0, super_block.data);

		int blocknum = inumber/INODES_PER_BLOCK + 1;
		int inode_index = inumber%INODES_PER_BLOCK;
			
		union fs_block inode_block;
		disk_read(blocknum, inode_block.data);
		
		while (1) {
			int bytes_written = 0;
			int stop_write = original_length - offset;
			if (stop_write <= 0) return 0;

			union fs_block data_block;
			int direct_block_index = offset/4096;

			if (direct_block_index < POINTERS_PER_INODE) {
				int direct_blocknum = find_free_block(super_block.super.nblocks);
				
				if (direct_blocknum < 0) return total_bytes_written;
				else FREE_BLOCK_BITMAP[direct_blocknum] = 0;
				
				inode_block.inode[inode_index].direct[direct_block_index] = direct_blocknum;

				int i;
				for (i = 0; i < DISK_BLOCK_SIZE; i++) {
					data_block.data[i] = data[offset+i];
					bytes_written++;
					if (bytes_written == stop_write) break;
				}
				disk_write(direct_blocknum, data_block.data);
			
			} else {
				int indirect_block_index = direct_block_index - POINTERS_PER_INODE;
				
				if (indirect_block_index == 0) {
					int indirect_blocknum = find_free_block(super_block.super.nblocks);

					if (indirect_blocknum < 0) return total_bytes_written;
					else FREE_BLOCK_BITMAP[indirect_blocknum] = 0;

					inode_block.inode[inode_index].indirect = indirect_blocknum;
		
					union fs_block indirect_block;
					int indirect_data_blocknum = find_free_block(super_block.super.nblocks);

					if (indirect_data_blocknum < 0) return total_bytes_written;
					else FREE_BLOCK_BITMAP[indirect_data_blocknum] = 0;

					indirect_block.pointers[indirect_block_index] = indirect_data_blocknum;
					disk_write(indirect_blocknum, indirect_block.data);
					
					int i;
					for (i = 0; i < DISK_BLOCK_SIZE; i++) {
						data_block.data[i] = data[offset+i];
						bytes_written++;
						if (bytes_written == stop_write) break;
					}
					disk_write(indirect_data_blocknum, data_block.data);
				}
			}

			inode_block.inode[inode_index].size += bytes_written;
			disk_write(blocknum, inode_block.data);

			total_bytes_written += bytes_written;
			length -= bytes_written;
			offset += bytes_written;

			if (total_bytes_written == original_length) return total_bytes_written;
		}
		return total_bytes_written;
	}
	return 0;
}
