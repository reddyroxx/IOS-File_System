#define FUSE_USE_VERSION 31

#include <fuse.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include "ffs.h"

char io_buffer[BLOCK_SIZE];

fcb files[MAX_FILE_NR];

dir dirs[MAX_DIR_NR];

int used_file_indices[MAX_FILE_NR];
int used_dir_indices[MAX_DIR_NR];


int _get_file_index(){
	int i = 0;
	while((i < MAX_FILE_NR) && used_file_indices[i]==1){ i++; }
	
	if(i == MAX_FILE_NR){ return -1; } // file table full

	return i;
}

int _get_dir_index(){
	int i = 0;
	while((i < MAX_DIR_NR) && used_dir_indices[i]==1){ i++; }
	
	if(i == MAX_DIR_NR){ return -1; } // directory table full

	return i;
}

int _find_file_by_path(const char *path){
	printf("finding file	:	%s\n-----------------\n\n", path);
	if(path == NULL){ return -1; }

	int i = 0;
	while(i < MAX_FILE_NR){
		if ( used_file_indices[i] ){
			if(strcmp(path, files[i].name) == 0){ return i; } // Match found at index i
		}
		i++;
	}
	// No match found
	return -1;
}

int _find_dir_by_path(const char *path){
	printf("finding dir	:	%s\n-----------------\n\n", path);
	if(path == NULL){ return -1; }

	int i = 0;
	while(i < MAX_DIR_NR){
		if ( used_dir_indices[i] ){
			if(strcmp(path, dirs[i].name) == 0){ return i; } // Match found at index i
		}
		i++;
	}
	// No match found
	return -1;
}

int _unlink_file_helper(int file_i){

	printf("Unlinking	:	%s\n\n", files[file_i].name);
	
	if(used_file_indices[file_i] == 0){ return 0; }

	dirs[files[file_i].parent].child_file_count -= 1;

	if(dirs[files[file_i].parent].first_child_file == file_i){
		dirs[files[file_i].parent].first_child_file = files[file_i].rsib;
	}

	if(files[file_i].lsib != -1) {
		files[files[file_i].lsib].rsib = files[file_i].rsib;
	}

	if(files[file_i].rsib != -1) {
		files[files[file_i].rsib].lsib = files[file_i].lsib;
	}

	used_file_indices[file_i] = 0;

	return 0;
}

static void *f_init(struct fuse_conn_info *conn, struct fuse_config *cfg){
	(void) conn;
	cfg->kernel_cache = 1;

	dirs[0] = (dir) {"/", 0, 0, -1, 0, -1, -1, -1, -1};

	int i;
	for(i=0;i < MAX_DIR_NR;i++){ used_dir_indices[i] = 0; }
	for(i=0;i < MAX_FILE_NR;i++){ used_file_indices[i] = 0; }
	used_dir_indices[0] = 1;

	return NULL;
}

// /abc/def.txt

static int f_create(const char *path, mode_t mode, struct fuse_file_info *fi){
	
	printf("f_create	:	%s\n", path+1);
	
	if (path == NULL){
		return -1;
	}

	int new_fcb_i = _get_file_index();

	if (new_fcb_i == -1){
		return -1; // Make this a valid error code, No free entries in file table
	}

	used_file_indices[new_fcb_i] = 1;

	// The following code extracts the lowest level directory in which the file will exist

	int i = strlen(path)-1;
	while(path[i] != '/'){ i--; }

	
	char *tmp_buf = NULL;
	int parent_dir_i;

	if (i == 0){ // file being made in root dir '/'
		parent_dir_i = 0;
	}
	else {
		tmp_buf = strndup(path + 1, i-1);

		parent_dir_i = _find_dir_by_path(tmp_buf);
		printf("finding parent directory: %s\n", tmp_buf);
		free(tmp_buf);
	}
	
	fcb *new_fcb = files + new_fcb_i;
	
	strcpy(new_fcb->name, path + 1);

	new_fcb->local_name_i = i;
	new_fcb->content = (block *) malloc(BLOCK_SIZE);
	new_fcb->size = 0;
	new_fcb->block_count = 1;
	
	new_fcb->permissions = mode;
	
	int first_born = dirs[parent_dir_i].child_file_count;

	new_fcb->lsib = -1;
	new_fcb->rsib = dirs[parent_dir_i].first_child_file;
	new_fcb->parent = parent_dir_i;

	dirs[parent_dir_i].first_child_file = new_fcb_i;
	dirs[parent_dir_i].child_file_count += 1;
	
	if( !first_born )	files[new_fcb->rsib].lsib = new_fcb_i;

	fi->fh = new_fcb_i;
	
	return 0;
}

static int f_mkdir(const char *path, mode_t mode){
	printf("f_mkdir		:	%s\n", path+1);
	
	if (path == NULL){
		return -1;
	}

	int new_dir_i = _get_dir_index();

	if (new_dir_i == -1){
		return -1; // Make this a valid error code, No free entries in directory table
	}

	used_dir_indices[new_dir_i] = 1;

	// The following code extracts the lowest level directory in which the directory will exist

	int i = strlen(path - 1);
	while(path[i] != '/'){ i--; }

	
	char *tmp_buf = NULL;
	int parent_dir_i;

	if (i == 0){ // file being made in root dir '/'
		parent_dir_i = 0;
	}
	else {
		tmp_buf = strndup(path + 1, i-1);

		parent_dir_i = _find_dir_by_path(tmp_buf);

		free(tmp_buf);
	}
	
	dir *new_dir = dirs + new_dir_i;
	
	strcpy(new_dir->name, path + 1);

	new_dir->local_name_i = i;
	new_dir->child_file_count = 0;
	new_dir->first_child_file = -1;

	new_dir->child_dir_count = 0;
	new_dir->first_child_dir = -1;
	
	int first_born = dirs[parent_dir_i].child_dir_count; // This is a boolean flag

	new_dir->lsib = -1;
	new_dir->rsib = dirs[parent_dir_i].first_child_dir;
	new_dir->parent = parent_dir_i;

	dirs[parent_dir_i].first_child_dir = new_dir_i;
	dirs[parent_dir_i].child_dir_count += 1;
	
	if( !first_born )	dirs[new_dir->rsib].lsib = new_dir_i;
	
	return 0;
}

static int f_getattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi){
	printf("f_getattr	:	%s\n-----------------\n\n", path);
	int res = 0;

	(void) fi;

	memset(stbuf, 0, sizeof(struct stat));

	if ( strcmp(path, "/") == 0 ) {
		printf("attrs of root\n");
		stbuf->st_mode = S_IFDIR | 0755;
		// printf("root's children: %d\n", dirs[0].child_dir_count);
		stbuf->st_nlink = 2 + dirs[0].child_dir_count;
		printf("Exiting getattr root\n");
		return 0;
	}

	int dir_i = _find_dir_by_path(path + 1);
	
	if (dir_i != -1) { // path is a directory
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2 + dirs[dir_i].child_dir_count;
	}
	else { // path is a file
		int file_i = _find_file_by_path(path + 1);

		if (file_i != -1){
			stbuf->st_mode = S_IFREG | 0755;
			stbuf->st_nlink = 1;
			stbuf->st_size = files[file_i].size;
		}
		else {
			printf("not found	:	%s\n", path);
			return -ENOENT;
		}
	}
	return res;
}

static int f_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi, enum fuse_readdir_flags flags){
	
	printf("f_readdir	:	%s\n-----------------\n\n", path);
	
	(void) offset;
	(void) fi;
	(void) flags;

	int root_fcb_i;

	if(strcmp(path, "/") == 0){
		root_fcb_i = 0;
	}
	else{
		root_fcb_i = _find_dir_by_path(path + 1);
	}
	
	if(root_fcb_i == -1){
		printf("BAD%sDAB\n", path);
		return -ENOENT;
	}

	filler(buf, ".", NULL, 0, 0);
	filler(buf, "..", NULL, 0, 0);
	
	// Handle all subdirectories
	// -------------------------------------------------------------
	int cur_child_dir_i = dirs[root_fcb_i].first_child_dir;

	while(cur_child_dir_i != -1){
		printf("d---->	%s\n", dirs[cur_child_dir_i].name + dirs[cur_child_dir_i].local_name_i);
		filler(buf, dirs[cur_child_dir_i].name + dirs[cur_child_dir_i].local_name_i, NULL, 0, 0);
		cur_child_dir_i = dirs[cur_child_dir_i].rsib;

	}

	// Handle all files
	// -------------------------------------------------------------
	int cur_child_file_i = dirs[root_fcb_i].first_child_file;

	while(cur_child_file_i != -1){
		printf("f---->	%s\n", files[cur_child_file_i].name + files[cur_child_file_i].local_name_i);
		filler(buf, files[cur_child_file_i].name + files[cur_child_file_i].local_name_i, NULL, 0, 0);
		cur_child_file_i = files[cur_child_file_i].rsib;
	
	}
	// -------------------------------------------------------------

	return 0;
}

static int f_open(const char *path, struct fuse_file_info *fi){
	printf("f_open	:	%s\n-----------------\n\n", path+1);

	int fp = _find_file_by_path(path+ 1);

	if(fp == -1){
		return -ENOENT;
	}

	fi->fh = fp;
	
	return 0;
}

static int f_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi){
	printf("f_read	:	%s\n-----------------\n\n", path);
	// if(!validate(fi->fh)){
	// 	// ERR file ka problem hai
	// }
	
	fcb r_fcb = files[fi->fh];

	int ret_val = size;
	if(size > (r_fcb.size - offset) ){
		size = r_fcb.size - offset;
		ret_val = size;
	}

	int block_i = offset / BLOCK_SIZE;
	// block_i++;

	int in_block_offset = offset % BLOCK_SIZE;

	int first_size = size;
	if(size > (BLOCK_SIZE - in_block_offset) ){
		first_size = BLOCK_SIZE - in_block_offset;
	}
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	memcpy(io_buffer, r_fcb.content + block_i, BLOCK_SIZE);
	memcpy(buf, io_buffer + in_block_offset, first_size);
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	
	block_i++;
	size -= first_size;

	while(size >= BLOCK_SIZE){
		
		memcpy(io_buffer, r_fcb.content + block_i, BLOCK_SIZE);
		memcpy(buf, io_buffer, BLOCK_SIZE);
		
		size -= BLOCK_SIZE;
		block_i++;
	}

	if(size > 0){
		memcpy(io_buffer, r_fcb.content+block_i, BLOCK_SIZE);
		memcpy(buf, io_buffer, size);
		size = 0; // Useful only for a sanity check.
	}

	return ret_val;
}

static int f_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi){
	printf("f_write	:	%s\n-----------------\n\n", path);
	(void) path;
	
	fcb w_fcb = files[fi->fh];
	
	int ret_val = size;
	
	int blocks_req = (offset + size)/ BLOCK_SIZE;
	if((offset+size) % BLOCK_SIZE){ blocks_req++; }

	if(blocks_req > w_fcb.block_count){
		w_fcb.content = (block *)realloc(w_fcb.content, blocks_req * BLOCK_SIZE);
		w_fcb.block_count = blocks_req;
	}

	if((offset + size) > w_fcb.size){
		w_fcb.size = offset + size;
	}
	
	int block_i = offset / BLOCK_SIZE;
	// block_i++;

	int in_block_offset = offset % BLOCK_SIZE;

	int first_size = size;
	if(size > (BLOCK_SIZE - in_block_offset) ){
		first_size = BLOCK_SIZE - in_block_offset;
	}

	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	memcpy(io_buffer, w_fcb.content+block_i, BLOCK_SIZE);
	memcpy(io_buffer + in_block_offset, buf, first_size);
	memcpy(w_fcb.content + block_i, io_buffer, BLOCK_SIZE);
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	
	block_i++;
	size -= first_size;

	while(size >= BLOCK_SIZE){
		memcpy(io_buffer, buf, BLOCK_SIZE);
		memcpy(w_fcb.content + block_i, io_buffer, BLOCK_SIZE);
		size -= BLOCK_SIZE;
		block_i++;
	}

	if(size > 0){
		memcpy(io_buffer, w_fcb.content + block_i, BLOCK_SIZE);
		memcpy(io_buffer, buf, size);
		memcpy(w_fcb.content + block_i, io_buffer, BLOCK_SIZE);
		size = 0; // Useful only for a sanity check.
	}

	files[fi->fh] = w_fcb;
	return ret_val;
}

int f_rename(const char *oldpath, const char *newpath, unsigned int flags){
	printf("rename -%s- to -%s-\n-----------------\n\n", oldpath, newpath);


	int dest_index = _find_file_by_path(newpath + 1);
	
	int src_index  = _find_file_by_path(oldpath + 1);
	
	if(dest_index != -1){
		
		files[dest_index].size = files[src_index].size;
		files[dest_index].block_count = files[src_index].block_count;
		files[dest_index].content = files[src_index].content;
		
		_unlink_file_helper(src_index);
	}
	
	return 0;
}



static struct fuse_operations my_ops = {
	.init		=	f_init,		// G T
	.readdir	=	f_readdir,	// G T
	.open		=	f_open,		// G T
	.read		=	f_read,		// G T
	.getattr	=	f_getattr,	// G T
	.write		=	f_write,	// G T
	.create		=	f_create,	// G T
	.rename		=	f_rename,	// G NT
	.mkdir		=	f_mkdir		// G T
};

int main(int argc, char *argv[]){

	return fuse_main(argc, argv, &my_ops, NULL);

}