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
void rm(struct ext2_inode *inode, int i, char *path);

int main(int argc, char **argv) {
    if(argc != 3 || argv[2][0] != '/') {
        fprintf(stderr, "Usage: ext2_rm <image file name> <absolute path>\n");
        exit(EINVAL);
    }
    int fd = open(argv[1], O_RDWR);
	
    disk = mmap(NULL, 128 * 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(disk == MAP_FAILED) {
	perror("mmap");
	exit(1);
    }
    int inode_num = get_inode_num(argv[2], disk);
    if(inode_num <= 0){
	fprintf(stderr, "‘%s’: No such file or directory\n", argv[2]);	
	exit(ENOENT);
    }
    struct ext2_group_desc *bgd = (struct ext2_group_desc *)(disk + (EXT2_BLOCK_SIZE *  2));  
    struct ext2_inode *inode_table = (struct ext2_inode *)(disk + (EXT2_BLOCK_SIZE * bgd->bg_inode_table));
    rm(inode_table, inode_num-1, argv[2]);
    
    return 0;
} 
void rm(struct ext2_inode *inode, int i, char *path){
	int parent_inode =  get_inode_num(get_parent(path), disk);
	int c;
	
	struct ext2_inode *in = inode+i; // inode of file

	char *absolutePath = malloc(sizeof(char) * (strlen(path) + 1));
    	strncpy(absolutePath, path, strlen(path));

	char *fileName = strrchr(absolutePath, '/');
	
	if(fileName != NULL){
		fileName++;
	}

	if(in->i_mode & EXT2_S_IFDIR){ 
		fprintf(stderr, "‘%s’: Cannot remove directory\n", absolutePath);
		exit(EISDIR);
	} else {
		in = inode+(parent_inode-1); // move pointer to parent inode

		if(in->i_mode & EXT2_S_IFDIR) { // if it is a directory
			for (c = 0; c < 12 && in->i_block[c]; c++){ // iterate through directory entries
				int size = EXT2_BLOCK_SIZE;
				struct ext2_dir_entry_2 *d = (struct ext2_dir_entry_2 *)(disk + (in->i_block[c] * EXT2_BLOCK_SIZE));
				struct ext2_dir_entry_2 *prev = (struct ext2_dir_entry_2 *)(disk + (in->i_block[c] * EXT2_BLOCK_SIZE));
				while(size){
					if(strncmp(fileName, d->name, d->name_len) == 0 && strlen(fileName) == d->name_len){ // if we find our file
						if(d->rec_len == EXT2_BLOCK_SIZE) { // if our file is only directory entry	
							memset(d, 0, sizeof(struct ext2_dir_entry_2));
							set_bitmap_zero(disk, in->i_block[c]-1, 'b');
							in->i_block[c] = 0;
							break;
						} if (size == EXT2_BLOCK_SIZE) { // if our file is the first entry in the block
							int temp = (int)d->rec_len;
							struct ext2_dir_entry_2 *q = (struct ext2_dir_entry_2 *)((char *) d+d->rec_len);
							d->inode = q->inode;
							d->rec_len = q->rec_len + temp;
							d->name_len = q->name_len;
							d->file_type = q->file_type;
							strncpy(d->name, q->name, d->name_len);	
							break;

						} else { // pad the preceding entry
							prev->rec_len += d->rec_len;		
							memset(d, 0, sizeof(struct ext2_dir_entry_2));
							break;
						}
		 			}
					// move onto next directory entry
					size -= d->rec_len;
					prev = d;
					d = (struct ext2_dir_entry_2 *)((char *) d+d->rec_len); 
				} if(size){
					break;
				}
			
			}
	   	 } else { // if the parent is not a directory
			fprintf(stderr, "‘%s’: Not a directory\n", absolutePath);	
			exit(ENOTDIR);
		}
			
		struct ext2_inode *file = inode+i;
		
		if(file->i_links_count == 1){ // if just the file exists, delete
			
			// set inode bit to 0
			set_bitmap_zero(disk, i, 'i');

			struct ext2_dir_entry_2 *del; 
			// set blocks that inode uses to 0
			for (c = 0; c < 12 && file->i_block[c]; c++) {
			
				set_bitmap_zero(disk, file->i_block[c]-1, 'b');
				del = (void *)(disk + (file->i_block[c] * EXT2_BLOCK_SIZE));
				memset(del, 0, EXT2_BLOCK_SIZE); // set block to 0
			}
			int k; 
		
			if (file->i_block[12]){ // if singly pointer used
				set_bitmap_zero(disk, file->i_block[12]-1, 'b');
				unsigned int *p = (unsigned int *)(disk + (file->i_block[12] * EXT2_BLOCK_SIZE));
				for(k=0; k<256 && *(p+k); k++){
					set_bitmap_zero(disk, *(p+k)-1, 'b');
					del = (void *)(disk + (*(p+k) * EXT2_BLOCK_SIZE));
					memset(del, 0, EXT2_BLOCK_SIZE); // set block to 0
				}	
			}
			memset(file, 0, sizeof(struct ext2_inode));
		} else {
			file->i_links_count--;
		}
	}
}
