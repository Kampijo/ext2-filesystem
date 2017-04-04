all: ext2_ln ext2_ls ext2_rm ext2_cp ext2_mkdir

ext2_ln : ext2_ln.o ext2_utils.o
	gcc -Wall -g -o ext2_ln $^

ext2_ls : ext2_ls.o ext2_utils.o
	gcc -Wall -g -o ext2_ls $^

ext2_rm : ext2_rm.o ext2_utils.o
	gcc -Wall -g -o ext2_rm $^

ext2_cp : ext2_cp.o ext2_utils.o
	gcc -Wall -g -o ext2_cp $^

ext2_mkdir : ext2_mkdir.o ext2_utils.o
	gcc -Wall -g -o ext2_mkdir $^

%.o : %.c ext2_utils.h ext2.h
	gcc -Wall -g -c $<
clean:
	rm *.o ext2_ln ext2_ls ext2_rm ext2_cp ext2_mkdir 
