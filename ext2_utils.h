#include <stdio.h>
#include <stdlib.h>
#include "ext2.h"

int get_inode_num(char *path, unsigned char *disk);
char *get_bit_map(char *bitmap, int count);
unsigned int get_free_inode(unsigned char *disk);
unsigned int get_free_block(unsigned char *disk);
void set_bitmap_zero(unsigned char *disk, int i, char type);
void set_bitmap_one(unsigned char *disk, int i, char type);
char *get_parent(char *path);

unsigned char *disk;
struct ext2_dir_entry_2 *find_space_in_current_block(struct ext2_inode *current_inode, int req_len);
int chop(char *path, char *path_array[10]);
int check_if_dir_exists(struct ext2_dir_entry_2 *dir_ptr, char *dir);
struct ext2_dir_entry_2 * find_inode(struct ext2_inode *inode_ptr, char *path_array[10], int path_array_len, int index, int bg_inode_table );
