#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ext2_utils.h"

// gets inode number
int get_inode_num(char *path, unsigned char *disk){
	
	 struct ext2_group_desc *bgd = (struct ext2_group_desc *)(disk + (EXT2_BLOCK_SIZE *  2));
	 struct ext2_inode *inode_table = (struct ext2_inode *)(disk + (EXT2_BLOCK_SIZE * bgd->bg_inode_table));
	 struct ext2_inode *current = inode_table+(EXT2_ROOT_INO-1);
	 char *tokens;
	 char *absolutePath = malloc(sizeof(char) * (strlen(path) + 1));
    	 strncpy(absolutePath, path, strlen(path));
	 int inode_num = EXT2_ROOT_INO;
	 int block_num = 0;
	 
	// splits path by delimiter character '/'
	 tokens = strtok(absolutePath, "/");	
	 
 	 struct ext2_dir_entry_2 *d;

	// while tokens available and haven't reached end of i_block 
	 while(tokens != NULL && current->i_block[block_num] && (current->i_mode & EXT2_S_IFDIR)){
		unsigned int size = EXT2_BLOCK_SIZE;
		d = (struct ext2_dir_entry_2 *)(disk + (current->i_block[block_num] * EXT2_BLOCK_SIZE));
		while(size){
			// if current entry matches
			if(strncmp(tokens, d->name, d->name_len) == 0 && strlen(tokens) == d->name_len){
				current = inode_table+(d->inode - 1); // set current inode
				inode_num = d->inode; // set inode_num
				tokens = strtok(NULL, "/"); // go to next token
				break;
		 	}
			size -= d->rec_len;
			d = (struct ext2_dir_entry_2 *)((char *) d+d->rec_len);
		}
		if(!size){
			block_num++;
		}		
	}
	free(absolutePath); // FREE IT BABY

	// if no more tokens, then valid path, return inode number
	if(!tokens){
		return inode_num;
	} else {
		return -1; 
	}	

}
// get_bit_map function from Tutorials 11-13
char *get_bit_map(char *bitmap, int count){
    int i, c;
    char *map = malloc(count + 1);

    for (i = 0; i < count / 8; i++) {
        for (c = 0; c < 8; c++) {
            map[(i * 8) + c] = (bitmap[i] & 1 << c)?'1':'0';
        }
        printf(" ");
    }
   map[count] = '\0';
   printf("\n");
   return map;
}

// using get_bit_map, get first available inode
unsigned int get_free_inode(unsigned char *disk){
	 int i;
	 struct ext2_super_block *sb = (struct ext2_super_block *)(disk + EXT2_BLOCK_SIZE);
	 struct ext2_group_desc *bgd = (struct ext2_group_desc *)(disk + (EXT2_BLOCK_SIZE *  2));

	 char* inode_map =  get_bit_map((char *)(disk + (EXT2_BLOCK_SIZE * bgd->bg_inode_bitmap)), sb->s_inodes_count);

	 for(i = EXT2_GOOD_OLD_FIRST_INO - 1; i < sb->s_inodes_count; i++){ // start from 11 as first 11 inodes reserved (i.e 0-10)
	 	if (inode_map[i] == '0') return i+1;
	 }
	 return -1;	
}
// using get_bit_map, get first available block
unsigned int get_free_block(unsigned char *disk){
	 int i;
	 struct ext2_super_block *sb = (struct ext2_super_block *)(disk + EXT2_BLOCK_SIZE);
	 struct ext2_group_desc *bgd = (struct ext2_group_desc *)(disk + (EXT2_BLOCK_SIZE *  2));

	 char* block_map = get_bit_map((char *)(disk + (EXT2_BLOCK_SIZE * bgd->bg_block_bitmap)), sb->s_blocks_count);

	 for(i = 1; i < sb->s_blocks_count; i++){ // always start from 1
	 	if (block_map[i] == '0') return i+1;
	 }
	 return -1;	
}

// gets parent of our path (for use on absolute paths)
char* get_parent(char *path){
        int i;
	char *absolutePath = malloc(sizeof(char) * (strlen(path) + 1)); // make copy so as to not modify char *path
    	strncpy(absolutePath, path, strlen(path));

	// search for last '/' character
	for(i = strlen(path)-2; i >= 0; i--){
		if(absolutePath[i] == '/'){
			absolutePath[i] = '\0'; // terminate
			break;
		}
	}
	return absolutePath;
}


// type being the bitmap type; inode or block
void set_bitmap_zero(unsigned char *disk, int i, char type) {

  struct ext2_super_block *sb = (struct ext2_super_block *)(disk + EXT2_BLOCK_SIZE);
  struct ext2_group_desc *bgd = (struct ext2_group_desc *)(disk + (EXT2_BLOCK_SIZE *  2));

  char *bitmap; 

  if(type == 'i'){
	bgd->bg_free_inodes_count++;
	sb->s_free_inodes_count++;
        bitmap = (char *)(disk + (EXT2_BLOCK_SIZE * bgd->bg_inode_bitmap)); // set bitmap to location of inode bitmap
  } else {
	bgd->bg_free_blocks_count++;
	sb->s_free_blocks_count++;
	bitmap = (char *)(disk + (EXT2_BLOCK_SIZE * bgd->bg_block_bitmap)); // set bitmap to location of block bitmap
  }

  int offset = i % 8;
  char bit = bitmap[i/8];
  bitmap[i/8] = bit & ~(1 << offset);
}

// type being the bitmap type; inode or block
void set_bitmap_one(unsigned char *disk, int i, char type) {

  struct ext2_super_block *sb = (struct ext2_super_block *)(disk + EXT2_BLOCK_SIZE);
  struct ext2_group_desc *bgd = (struct ext2_group_desc *)(disk + (EXT2_BLOCK_SIZE *  2));

  char *bitmap; 

  if(type == 'i'){
	bgd->bg_free_inodes_count--;
	sb->s_free_inodes_count--;
        bitmap = (char *)(disk + (EXT2_BLOCK_SIZE * bgd->bg_inode_bitmap)); // set bitmap to location of inode bitmap
  } else {
	bgd->bg_free_blocks_count--;
	sb->s_free_blocks_count--;
	bitmap = (char *)(disk + (EXT2_BLOCK_SIZE * bgd->bg_block_bitmap)); // set bitmap to location of block bitmap
  }

  int offset = i % 8;
  char bit = bitmap[i/8];
  bitmap[i/8] = bit | (1 << offset);
}

/*******************************************************************************************/

int chop(char *path, char *path_array[10]){
    //path counter
    int pc = 0;

    //break apart path
    char *p_split = strtok(path, "/");
    
    while(p_split != NULL){
        path_array[pc] = p_split;
        pc++;
        p_split = strtok(NULL, "/");
    }
    return pc;
}

int check_if_dir_exists(struct ext2_dir_entry_2 *dir_ptr, char *dir){
    int size = 0;
    char *current = ".";
    char *parent = "..";

    while (size < EXT2_BLOCK_SIZE) {

        if (dir_ptr->file_type == EXT2_FT_REG_FILE){
            char *dir_name = malloc(sizeof(char *));
            strncpy(dir_name, dir_ptr -> name, dir_ptr -> name_len);
            if(strcmp(dir_name, dir) == 0){
                return dir_ptr -> inode;
            }
        }   
        if (dir_ptr->inode != 2 && dir_ptr->inode != 11 //we don't go deeper in reserved directories.
                && (strncmp(dir_ptr->name, parent, 2) != 0) //we don't recurse on parents -> infinite loop.
                && (strncmp(dir_ptr->name, current, 1) != 0)) {
            char *dir_name = malloc(sizeof(char *));
            strncpy(dir_name, dir_ptr -> name, dir_ptr -> name_len);
            if(strcmp(dir_name, dir) == 0){
                return dir_ptr -> inode;
            }
        }
        size += dir_ptr->rec_len;
        dir_ptr = (void *)dir_ptr + dir_ptr->rec_len;
    }

    return 0;
}

struct ext2_dir_entry_2 * find_space_in_current_block(struct ext2_inode *current_inode, int req_len){
    struct ext2_dir_entry_2 *dir_ptr;
    int i = 0;
    int data_block_num;
    int found = 0;

    for(i = 0; i < 12; i++){
        data_block_num = current_inode -> i_block[i];
        dir_ptr = (void *)disk + EXT2_BLOCK_SIZE * data_block_num;
        int size = 0;

        while (size < EXT2_BLOCK_SIZE) {
            size += dir_ptr -> rec_len;
            if(size == EXT2_BLOCK_SIZE && dir_ptr -> rec_len >= (4 * (dir_ptr -> name_len) + req_len + 8)) {
                return dir_ptr;
                break;
            }
            dir_ptr = (void *)dir_ptr + dir_ptr->rec_len;
        }
        if(found == 1){
            break;
        }
    }
    return 0;
}



struct ext2_dir_entry_2 * find_inode(struct ext2_inode *inode_ptr, char *path_array[10], int path_array_len, int index, int bg_inode_table) {
    //this function reads all the data_blocks in the inodes root directory
    int data_block_num;
    if(path_array_len == 1){
        int i = 0;
        for(i = 0; i < 11; i++){
            if (inode_ptr->i_block[i] > 0) {
                data_block_num = inode_ptr->i_block[i];
                struct ext2_dir_entry_2 *dir_ptr = (struct ext2_dir_entry_2 *)(disk + EXT2_BLOCK_SIZE * data_block_num);
                return dir_ptr; 
            }
        }
    }

    
    int i;

    for (i=0; i<12; i++) { //print direct pointers
        if (inode_ptr->i_block[i] > 0) {//if a block is asigned
            data_block_num = inode_ptr->i_block[i];
            struct ext2_dir_entry_2 *dir_ptr = (struct ext2_dir_entry_2 *)(disk + EXT2_BLOCK_SIZE * data_block_num);
            int dir_inode_ptr = check_if_dir_exists(dir_ptr, path_array[index]);
            if(dir_inode_ptr != 0){
                if(path_array_len - index == 2){
                    struct ext2_inode *inode_ptr2 = (struct ext2_inode *)(disk + EXT2_BLOCK_SIZE * bg_inode_table);
                    inode_ptr2 += dir_inode_ptr-1;
                    data_block_num = inode_ptr2->i_block[i];
                    dir_ptr = (struct ext2_dir_entry_2 *)(disk + EXT2_BLOCK_SIZE * data_block_num);
                    return dir_ptr;
                } else if (path_array_len - index == 1){
                    return 0;
                } else {
                    struct ext2_inode *inode_ptr2 = (struct ext2_inode *)(disk + EXT2_BLOCK_SIZE * bg_inode_table);
                    inode_ptr2 += dir_inode_ptr-1;
                    return find_inode(inode_ptr2, path_array, path_array_len, index+1, bg_inode_table);
                }
            }

        }
    }
    return 0;
}

