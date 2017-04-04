#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <errno.h>
#include "ext2_utils.h"

unsigned char *disk;
void print_dir_contents(struct ext2_inode *inode, int i);

int main(int argc, char **argv) {
    
    if(argc != 3 || argv[2][0] != '/') {
        fprintf(stderr, "Usage: ext2_ls <image file name> <absolute path>\n");
        exit(EINVAL);
    }
    int fd = open(argv[1], O_RDWR);
	
    disk = mmap(NULL, 128 * 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(disk == MAP_FAILED) {
	perror("mmap");
	exit(1);
    }

    struct ext2_group_desc *bgd = (struct ext2_group_desc *)(disk + (EXT2_BLOCK_SIZE *  2));  
    struct ext2_inode *inode_table = (struct ext2_inode *)(disk + (EXT2_BLOCK_SIZE * bgd->bg_inode_table));

    int inode_num = get_inode_num(argv[2], disk);
   
    if (inode_num <= 0){
	fprintf(stderr, "‘%s’: No such file or directory\n", argv[2]);	
	exit(ENOENT);
    }

    struct ext2_inode *d = inode_table+(inode_num-1);

    if(!(d->i_mode & EXT2_S_IFDIR)){ // if inode is not a directory
	char *dirName = strrchr(argv[2], '/');
	if(dirName != NULL){
		dirName++;
	}
	printf("%s\n", dirName);
    } else {
    	print_dir_contents(inode_table, inode_num-1);
    }
    return 0;
}

// prints contents of directory
void print_dir_contents(struct ext2_inode *inode, int i){
	int c;
	struct ext2_inode *in = inode+i;
	
	if(in->i_mode & EXT2_S_IFDIR){
		for (c = 0; c < 12 && in->i_block[c]; c++){ // find blocks used by inode
			int size = EXT2_BLOCK_SIZE;
			struct ext2_dir_entry_2 *d = (struct ext2_dir_entry_2 *)(disk + (in->i_block[c] * EXT2_BLOCK_SIZE));

			while(size){  // iterate through the block
				char inode_name[d->name_len+1];
		 		strncpy(inode_name, d->name, d->name_len);
		 		inode_name[d->name_len] = '\0';
				printf("%s\n", inode_name);
				size -= d->rec_len;
				d = (struct ext2_dir_entry_2 *)((char *) d+d->rec_len);
			}
			
		}
	}
}

	

