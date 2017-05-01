#ifndef FS_H
#define FS_H

void fs_debug();
int  fs_format();
int  fs_mount();

int  fs_create();
int  fs_delete( int inumber );
int  fs_getsize();

int  fs_read( int inumber, char *data, int length, int offset );
int  fs_write( int inumber, const char *data, int length, int offset );

// Helper Functions
int get_num_data_blocks(int size);
void initialize_free_block_bitmap(int nblocks);
void initialize_inode_table(int ninodes);
int find_free_block();

#endif