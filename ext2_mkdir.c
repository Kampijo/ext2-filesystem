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
void makedir(struct ext2_inode *inode, int i, char *path);

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

    int parent_inode = get_inode_num(get_parent(argv[2]), disk); // get inode of parent directory of absolute path
    int inode_num = get_inode_num(argv[2], disk);

    if (parent_inode <= 0){ // if parent does not exist
	fprintf(stderr, "cannot create directory ‘%s’: No such file or directory\n", argv[2]);	
	exit(ENOENT);

    }
    if (inode_num > 0){ // if file/directory of same name already exists
	fprintf(stderr, "cannot create directory ‘%s’: File exists\n", argv[2]);	
	exit(EEXIST);
    }
    makedir(inode_table, parent_inode-1, argv[2]);
    return 0;
}
void makedir(struct ext2_inode *inode, int i, char *path){
	char *absolutePath = malloc(sizeof(char) * (strlen(path) + 1));
    	strncpy(absolutePath, path, strlen(path));
	
	char *dirName = strrchr(absolutePath, '/');
	
	if(dirName != NULL){
		dirName++;
	}

	struct ext2_inode *in = inode+i;
	struct ext2_dir_entry_2 *d; 
	int c;

	if(in->i_mode & EXT2_S_IFDIR){
		
		unsigned int free_inode = get_free_inode(disk); // get free inode and free block
		unsigned int free_block = get_free_block(disk); 
		in->i_links_count++; // increase link count of parent directory

		struct ext2_dir_entry_2 *new_dir = (struct ext2_dir_entry_2 *)(disk + (EXT2_BLOCK_SIZE * free_block)); // go to free block
  
		// set . and .. directory entries in our new directory
  		new_dir->inode = free_inode; // our free inode
		new_dir->name_len = 1;
  		new_dir->rec_len = 12;
  		new_dir->file_type = EXT2_FT_DIR;
		strncpy(new_dir->name, ".", 1);
  		new_dir = (struct ext2_dir_entry_2 *)((char *) new_dir+new_dir->rec_len);

		new_dir->inode = i+1; // parent inode
		new_dir->name_len = 2;
  		new_dir->rec_len = 1012;
  		new_dir->file_type = EXT2_FT_DIR;
		strncpy(new_dir->name, "..", 2);
	
		// update the block bitmap
		set_bitmap_one(disk, free_block-1, 'b'); // set free block to used

		struct ext2_inode* new_inode = inode+free_inode-1;
  		new_inode->i_mode = EXT2_S_IFDIR; // set to directory type
  		new_inode->i_size = EXT2_BLOCK_SIZE;
  		new_inode->i_links_count = 2;
  		new_inode->i_blocks = 2; 
  		new_inode->i_block[0] = free_block;

  		// update inode bitmap
  		set_bitmap_one(disk, free_inode-1, 'i');
		
		int allocated = 0;
		for (c = 0; c < 12 && in->i_block[c]; c++){ // find blocks used by inode
			int size = EXT2_BLOCK_SIZE;
			d = (struct ext2_dir_entry_2 *)(disk + (in->i_block[c] * EXT2_BLOCK_SIZE));

			while(size){  // iterate through the block
				if((size - d->rec_len) == 0){ // get last directory entry
					int temp_len = d->rec_len;
    					int adjusted = d->name_len + 8; 
					int newDir = strlen(dirName) + 8;
					if(adjusted % 4){
						adjusted += 4 - (adjusted % 4);
					}
					if(newDir % 4){
						newDir += 4 - (newDir % 4);
					}
					if(d->rec_len >= adjusted + newDir){ // if enough space for the new dir entry
						allocated = 1;
						d->rec_len = adjusted; // adjust last directory entry rec_len
				    		d = (struct ext2_dir_entry_2 *)((char *) d+adjusted);
				    		d->inode = free_inode;
				    		d->rec_len = temp_len - adjusted;
				    		d->name_len = strlen(dirName);
				    		d->file_type = EXT2_FT_DIR;
				    		strncpy(d->name, dirName, d->name_len);
						break;
					}
				} 
				// move onto next directory entry
				size -= d->rec_len;
				d = (struct ext2_dir_entry_2 *)((char *) d+d->rec_len);
			}
			if(size){
				break;
			}
		}
		if(!allocated){ // Otherwise allocate a new block to make space for new dir entry
			int free_block = get_free_block(disk); 
			
			// update the block bitmap
			set_bitmap_one(disk, free_block-1, 'b'); // set free block to used

			struct ext2_dir_entry_2 *dirBlock = (struct ext2_dir_entry_2 *)(disk + (EXT2_BLOCK_SIZE * free_block)); // go to free block
			dirBlock->inode = free_inode;
		    	dirBlock->rec_len = 1024;
		    	dirBlock->name_len = strlen(dirName);
		    	dirBlock->file_type = EXT2_FT_DIR;
		    	strncpy(dirBlock->name, dirName, dirBlock->name_len);
			for (c = 0; c < 12 && in->i_block[c]; c++); // find last unused spot in i_block
			in->i_block[c] = free_block;
			in->i_blocks += 2;
			in->i_size += EXT2_BLOCK_SIZE;
		
		}
 	} else { // if the parent is not a directory
		fprintf(stderr, "‘%s’: Not a directory\n", absolutePath);	
		exit(ENOTDIR);
	}
}	

