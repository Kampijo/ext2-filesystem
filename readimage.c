#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include "ext2.h"

unsigned char *disk;
extern char* get_bit_map(char *bitmap, int count);
extern void print_inode(struct ext2_inode *inode, int i);
extern void print_dir_contents(struct ext2_inode *inode, int i);

int main(int argc, char **argv) {
    int i;

    if(argc != 2) {
        fprintf(stderr, "Usage: readimg <image file name>\n");
        exit(1);
    }
    int fd = open(argv[1], O_RDWR);

    disk = mmap(NULL, 128 * 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(disk == MAP_FAILED) {
	perror("mmap");
	exit(1);
    }

    struct ext2_super_block *sb = (struct ext2_super_block *)(disk + EXT2_BLOCK_SIZE);
    printf("Inodes: %d\n", sb->s_inodes_count);
    printf("Blocks: %d\n", sb->s_blocks_count);

    struct ext2_group_desc *bgd = (struct ext2_group_desc *)(disk + (EXT2_BLOCK_SIZE *  2));
    printf("Block group:\n");
    printf("    block bitmap: %d\n", bgd->bg_block_bitmap);
    printf("    inode bitmap: %d\n", bgd->bg_inode_bitmap);
    printf("    inode table: %d\n", bgd->bg_inode_table);
    printf("    free blocks: %d\n", bgd->bg_free_blocks_count);
    printf("    free inodes: %d\n", bgd->bg_free_inodes_count);
    printf("    used_dirs: %d\n", bgd->bg_used_dirs_count);
    
    char *block_map, *inode_map;
    printf("Block bitmap: ");
    block_map = get_bit_map((char *)(disk + (EXT2_BLOCK_SIZE * bgd->bg_block_bitmap)), sb->s_blocks_count);
    printf("Inode bitmap: ");
    inode_map = get_bit_map((char *)(disk + (EXT2_BLOCK_SIZE * bgd->bg_inode_bitmap)), sb->s_inodes_count);

    printf("\nInodes:\n");
    struct ext2_inode *inode_table = (struct ext2_inode *)(disk + (EXT2_BLOCK_SIZE * bgd->bg_inode_table));

    print_inode(inode_table, EXT2_ROOT_INO - 1);
    for(i = EXT2_GOOD_OLD_FIRST_INO - 1; i < sb->s_inodes_count; i++){
	 if (inode_map[i] == '1'){
		 print_inode(inode_table, i);
	}
    }
    printf("\nDirectory Blocks:\n");
    print_dir_contents(inode_table, EXT2_ROOT_INO - 1);
    for(i = EXT2_GOOD_OLD_FIRST_INO - 1; i < sb->s_inodes_count; i++){
	 if (inode_map[i] == '1'){
		 print_dir_contents(inode_table, i);
	}
    }
    
    free(inode_map);
    free(block_map);
    return 0;
}
char* get_bit_map(char *bitmap, int count){
    int i, c;
    char *map = malloc(count + 1);

    for (i = 0; i < count / 8; i++) {
        for (c = 0; c < 8; c++) {
            map[(i * 8) + c] = (bitmap[i] & 1 << c)?'1':'0';
            printf("%c", map[(i * 8) + c]);
        }
        printf(" ");
    }
   map[count] = '\0';
   printf("\n");
   return map;
}
void print_inode(struct ext2_inode *inode, int i){
    int c;
    char type;
    struct ext2_inode *in = inode + i;
    if(i+1 == EXT2_GOOD_OLD_FIRST_INO){
		return;
    }
    switch(in->i_mode >> 12){
        case(0x4): type = 'd'; break;
        case(0x8): type = 'f'; break;
        default:   type = '?'; break;
    }

    printf("[%d] type: %c size: %d links: %d blocks: %d\n", 
        i+1, type, in->i_size, in->i_links_count, in->i_blocks);
    printf("[%d] Blocks:", i+1);

    for (c = 0; c < 12; c++) if (in->i_block[c]) printf(" %d", in->i_block[c]);
    printf("\n");
   
}
void print_dir_contents(struct ext2_inode *inode, int i){
	int c;
	struct ext2_inode *in = inode+i;
	if(i+1 == EXT2_GOOD_OLD_FIRST_INO){
		return;
	}
	if(in->i_mode >> 12 & 0x4){
		for (c = 0; c < 12 && in->i_block[c]; c++){
			int size = EXT2_BLOCK_SIZE;
			char type;
			struct ext2_dir_entry_2 *d = (struct ext2_dir_entry_2 *)(disk + (in->i_block[c] * EXT2_BLOCK_SIZE));
			printf("   DIR BLOCK NUM: %d (for inode %d)\n", in->i_block[c], i+1);
			while(size > 0){
				 switch (d->file_type) {
        		case EXT2_FT_REG_FILE: type = 'f'; break;
        		case EXT2_FT_DIR:      type = 'd'; break;
        		default:               type = '?'; break;
        		}
				char name[d->name_len+1];
		 		strncpy(name, d->name, d->name_len);
		 		name[d->name_len] = '\0';
				printf("Inode: %d rec_len: %d name_len: %d type= %c name=%s\n", d->inode, d->rec_len, d->name_len, type, name);
				size -= d->rec_len;
				d = (struct ext2_dir_entry_2 *)((char *) d+d->rec_len);
			}
			
		}
	}	
}

	

