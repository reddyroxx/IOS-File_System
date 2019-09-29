#ifndef FFS
	#define FFS

	#define BLOCK_SIZE			4096
	#define MAX_FILE_NR			1024
	#define MAX_DIR_NR			128
	#define MAX_NAME		512

	typedef struct _block{
		char data[BLOCK_SIZE];
	} block;

	typedef struct {
		char name[MAX_NAME];

		int local_name_i;

		size_t size;
		int block_count;
		mode_t permissions;
		block *content;

		int lsib;
		int rsib;

		int parent;
	}fcb;


	typedef struct {
		char name[MAX_NAME];

		int local_name_i;
		
		int child_file_count;
		int first_child_file;

		int child_dir_count;
		int first_child_dir;
		
		int lsib;
		int rsib;

		int parent;
	} dir;

#endif