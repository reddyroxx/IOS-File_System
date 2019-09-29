#define _LARGEFILE64_SOURCE
#include "disk_mgr.h"
#include "ffs.h"
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

#define GIG (1024 * 1024 * 1024)

int df_fd; // disk.file_file.descriptor
char cwd[1024];


int disk_init(char *df_path){
	// getcwd(cwd, sizeof(cwd));
	// printf("working directory: %s\n\n\n", cwd);
	printf("path to open db : %s\n\n", df_path);
	df_fd = open(df_path, O_RDWR | O_CREAT, 0705);
	
	if (df_fd == -1){
		// open() failed
		printf("disk_mgr	:	OPEN ERROR\n");
		printf("errno : %d\n", errno);
		printf("Is access error : %d\n", errno == EACCES);
		printf("Does not exist  : %d\n", errno == ENOENT);
		return -1;
	}
	return 0;
}

int disk_get_block(int fileno, off_t block_offset, char *io_buf){
	// Perform basic sanity checks
	printf("-------------------------\n--|%d|--\n-------------------------\n", df_fd);
	lseek(df_fd, (fileno * GIG) + block_offset, SEEK_SET);
	int nr_bytes_read = read(df_fd, io_buf, BLOCK_SIZE);

	if(nr_bytes_read != BLOCK_SIZE){
		// WHAT DOES THIS MEAN !?
		return -1;
	}

	return BLOCK_SIZE;
}

int disk_set_block(int fileno, off_t block_offset, char *io_buf){
	// Perform basic sanity checks
	printf("-------------------------\n--|%d|--\n-------------------------\n", df_fd);
	lseek(df_fd, (fileno * GIG) + block_offset, SEEK_SET);
	int nr_bytes_read = write(df_fd, io_buf, BLOCK_SIZE);

	if(nr_bytes_read != BLOCK_SIZE){
		// WHAT DOES THIS MEAN !?
		return -1;
	}

	return BLOCK_SIZE;
}

void disk_destroy(void){
	close(df_fd);
}