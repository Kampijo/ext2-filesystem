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

int main(int argc, char **argv) {

    //Make sure the command has all information it needs
    if (argc != 4) {
        fprintf(stderr, "usage: ext2_ln <image file> <source file path> <target file path> \n");
        exit(1);
    }

    //File descriptor for source file
    int fd = open(argv[1], O_RDWR);

    //Map memory for source file
    disk = mmap(NULL, 128 * 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    //Make sure that if the map fails its properly dealt with
    if (disk == MAP_FAILED) {
       perror("mmap");
       exit(1);
    }

    //Declare important pointers
    struct ext2_group_desc *groupd = (struct ext2_group_desc *)(disk + EXT2_SB_OFFSET + EXT2_BLOCK_SIZE);
    struct ext2_inode *inode_ptr = (struct ext2_inode *)(disk + EXT2_BLOCK_SIZE * groupd->bg_inode_table);

    //Duplicate source path string 
    char *path = strdup(argv[2]);

    //To hold source path
    char *spa[10];

    //Count number of path of source path
    int spc = chop(path, spa);

    //Duplicate target path string 
    char *target_path = strdup(argv[3]);

    //To hold target path
    char *tpa[10];

    //Count number of path of target path
    int tpc = chop(dest_path, dest_path_array);

    //pointer to source directory
    struct ext2_dir_entry_2 *src_dir = find_inode(inode_ptr + 1, spa, spc, 0, groupd->bg_inode_table);
    
    //pointer to target directory
    struct ext2_dir_entry_2 *target_dir = find_inode(inode_ptr + 1, tpa, tpc, 0, groupd->bg_inode_table);


    //Check if the directory actually exists
    if((src_dir == 0) || (target_dir == 0)){
        fprint(stderr, "No such directory found \n");
        return -1;
    }

    //check if the traget directory exists
    if(check_if_dir_exists(target_dir, dpa[tpc - 1]) != 0){
        fprintf(stderr, "Directory already exists\n");
        exit(EISDIR);
    }

    int src_file_inode = check_if_dir_exists(src_dir, spa[spc - 1]);

    struct ext2_inode *src_inode = (struct ext2_inode *)(disk + EXT2_BLOCK_SIZE * groupd->bg_inode_table);
    
    src_inode = src_inode + src_dir -> inode - 1;

    struct ext2_inode *dest_inode = (struct ext2_inode *)(disk + EXT2_BLOCK_SIZE * groupd->bg_inode_table);
    
    dest_inode = dest_inode + dest_dir -> inode - 1;
    dest_inode->i_links_count++;

    int req_len = strlen(dest_path_array[dest_path_count - 1]) + 8;

    struct ext2_dir_entry_2 *dir_ptr = find_space_in_current_block(dest_inode, req_len);

    if(dir_ptr != 0){
        int new_rec_len = dir_ptr -> rec_len - (4 * (dir_ptr -> name_len) + 8);
        dir_ptr -> rec_len = 4 * (dir_ptr -> name_len) + 8;
        dir_ptr = (void*)dir_ptr + dir_ptr -> rec_len;
        dir_ptr -> rec_len = new_rec_len;
        dir_ptr -> inode = src_file_inode;
        dir_ptr -> name_len = strlen(dest_path_array[dest_path_count - 1]);
        dir_ptr -> file_type = EXT2_FT_REG_FILE;
        strcpy(dir_ptr -> name, dest_path_array[dest_path_count - 1]);
    }

    //Increment the number of dirs used
    groupd->bg_used_dirs_count++;

    return 0;
}

