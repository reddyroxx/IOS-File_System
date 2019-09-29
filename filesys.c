#define FUSE_USE_VERSION 30

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

/*#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>*/

#define MAX_NAME 512
struct stati {
	/*mode_t			st_mode;
	ino_t			st_ino;
	dev_t			st_dev;
	dev_t			st_rdev;
	nlink_t			st_nlink;
	uid_t			st_uid;
	gid_t			st_gid;
	off_t			st_size;
	struct timespec	st_atim;
	struct timespec	st_mtim;
	struct timespec st_ctim;
	blksize_t		st_blksize;
	blkcnt_t		st_blocks;*/
	__dev_t st_dev;		/* Device.  */
#ifndef __x86_64__
    unsigned short int __pad1;
#endif
#if defined __x86_64__ || !defined __USE_FILE_OFFSET64
    __ino_t st_ino;		/* File serial number.	*/
#else
    __ino_t __st_ino;			/* 32bit file serial number.	*/
#endif
#ifndef __x86_64__
    __mode_t st_mode;			/* File mode.  */
    __nlink_t st_nlink;			/* Link count.  */
#else
    __nlink_t st_nlink;		/* Link count.  */
    __mode_t st_mode;		/* File mode.  */
#endif
    __uid_t st_uid;		/* User ID of the file's owner.	*/
    __gid_t st_gid;		/* Group ID of the file's group.*/
#ifdef __x86_64__
    int __pad0;
#endif
    __dev_t st_rdev;		/* Device number, if device.  */
#ifndef __x86_64__
    unsigned short int __pad2;
#endif
#if defined __x86_64__ || !defined __USE_FILE_OFFSET64
    __off_t st_size;			/* Size of file, in bytes.  */
#else
    __off64_t st_size;			/* Size of file, in bytes.  */
#endif
    __blksize_t st_blksize;	/* Optimal block size for I/O.  */
#if defined __x86_64__  || !defined __USE_FILE_OFFSET64
    __blkcnt_t st_blocks;		/* Number 512-byte blocks allocated. */
#else
    __blkcnt64_t st_blocks;		/* Number 512-byte blocks allocated. */
#endif
#ifdef __USE_XOPEN2K8
    /* Nanosecond resolution timestamps are stored in a format
       equivalent to 'struct timespec'.  This is the type used
       whenever possible but the Unix namespace rules do not allow the
       identifier 'timespec' to appear in the <sys/stat.h> header.
       Therefore we have to handle the use of this header in strictly
       standard-compliant sources special.  */
    struct timespec st_atim;		/* Time of last access.  */
    struct timespec st_mtim;		/* Time of last modification.  */
    struct timespec st_ctim;		/* Time of last status change.  */
# define st_atime st_atim.tv_sec	/* Backward compatibility.  */
# define st_mtime st_mtim.tv_sec
# define st_ctime st_ctim.tv_sec
#else
    __time_t st_atime;			/* Time of last access.  */
    __syscall_ulong_t st_atimensec;	/* Nscecs of last access.  */
    __time_t st_mtime;			/* Time of last modification.  */
    __syscall_ulong_t st_mtimensec;	/* Nsecs of last modification.  */
    __time_t st_ctime;			/* Time of last status change.  */
    __syscall_ulong_t st_ctimensec;	/* Nsecs of last status change.  */
#endif
#ifdef __x86_64__
    __syscall_slong_t __glibc_reserved[3];
#else
# ifndef __USE_FILE_OFFSET64
    unsigned long int __glibc_reserved4;
    unsigned long int __glibc_reserved5;
# else
    __ino64_t st_ino;			/* File serial number.	*/
# endif
#endif
};
  typedef struct __data {
  	char name[MAX_NAME];
  	int  isdir;
  	struct stati st;
  } Ndata;

  typedef struct element {
  	Ndata data;
  	char * filedata;
  	struct element * parent;
  	struct element * firstchild;
  	struct element * next;
  } Node;

  int freememory;
  Node * Root;

//below for extra credit
  char filedump[MAX_NAME];

	//Function to allocate memory based on available memory
  int allocate_node(Node ** node) {

  	if (freememory < sizeof(Node)) {
  		return -ENOSPC;
  	}

  	*node = calloc(1, sizeof(Node));
  	if (*node == NULL) {
  		return -ENOSPC;
  	} else {
  		freememory = freememory - sizeof(Node);
  		return 0;
  	}
  }

  void change_timestamps_dir(Node * parent) {
  	time_t T;
  	time(&T);
  	parent->data.st.st_ctime = T;
  	parent->data.st.st_mtime = T;
  }

	//Function to validate the existence of given path
  int check_path(const char * path, Node ** n) {

  	char temp[MAX_NAME];
  	strncpy(temp, path, MAX_NAME);

  	if(strcmp(path, "/") == 0 || strcmp(path, "") == 0) {
  		*n = Root;
  		return 1;		
  	}

  	char * ele = strtok(temp, "/");
  	Node * parent = Root;
  	Node * childptr = NULL;
  	while (ele != NULL) {
  		int found = 0;
  		childptr = parent->firstchild;
  		while (childptr != NULL) {
  			if(strcmp(childptr->data.name, ele) == 0) {
  				found = 1;
  				break;
  			}
  			childptr = childptr->next;
  		}
  		if (!found) {*n = NULL; return 0;}

  		ele = strtok(NULL, "/");
  		parent = childptr;
  	}
  	*n = childptr;
  	return 1;
  }

	//Function to fetch attributes of a file/directory
  static int fsys_getattr(const char *path, struct stati *stbuf)
  {
  	Node *t = NULL;
  	int valid = check_path(path, &t);
  	if (!valid) {
  		return -ENOENT;
  	} else {
  		*stbuf = t->data.st;
  		return 0;
  	}
  }

	//Function to read directory contents
  static int fsys_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
  	off_t offset, struct fuse_file_info *fi)
  {    
  	time_t T;
  	time(&T);

  	Node * parent = NULL;

  	int valid = check_path(path, &parent);
  	if (!valid) {
  		return -ENOENT;
  	}

  	filler(buf, ".", NULL, 0);
  	filler(buf, "..", NULL, 0);
  	
  	Node * temp = NULL;

  	for(temp = parent->firstchild; temp; temp = temp->next) {
  		filler(buf, temp->data.name, NULL, 0);
  	}
  	parent->data.st.st_atime = T;


  	return 0;
  }

//Funtion to open file/directory
  static int fsys_open(const char *path, struct fuse_file_info *fi)
  {	
  	Node *p= NULL;
  	int valid = check_path(path, &p);
  	if (!valid) {
  		return -ENOENT;
  	}
  	return 0;
  }

//Function to read from a file/directory
  static int fsys_read(const char *path, char *buf, size_t size, off_t offset,
  	struct fuse_file_info *fi)
  {
  	time_t T;

  	Node * node = NULL;
  	int valid = check_path(path, &node);

  	if (!valid) {
  		return -ENOENT;
  	}
  	int filesize = node->data.st.st_size;

  	if (node->data.isdir) {
  		return -EISDIR;
  	}

  	time(&T);
  	
  	if (offset < filesize) {
  		if (offset + size > filesize) {
  			size = filesize - offset;
  		}
  		memcpy(buf, node->filedata + offset, size);
  	} else {
  		size = 0;
  	}

  	return size;
  }


  static int fsys_utime(const char *path, struct utimbuf *ubuf)
  {
	// Do Nothing
  	return 0;
  }


	//Function to initialize a directory
  void init_for_dir(Node * newchild, char * dname) {
  	
  	newchild->data.isdir = 1;
  	strcpy(newchild->data.name, dname);

	newchild->data.st.st_nlink = 2;   // . and ..
	newchild->data.st.st_uid = getuid();
	newchild->data.st.st_gid = getgid();
	newchild->data.st.st_mode = S_IFDIR |  0755; //755 is default directory permissions

	newchild->data.st.st_size = 4096;
	newchild->data.st.st_blocks = 8;
	time_t T;
	time(&T);

	newchild->data.st.st_atime = T;
	newchild->data.st.st_mtime = T;
	newchild->data.st.st_ctime = T;
}


//Function to initialize a file
void init_for_file(Node * newchild, char * fname) {
	
	newchild->data.isdir = 0;
	strcpy(newchild->data.name, fname);

	newchild->data.st.st_size = 0;
	newchild->data.st.st_nlink = 1;   
	newchild->data.st.st_uid = getuid();
	newchild->data.st.st_gid = getgid();
	newchild->data.st.st_mode = S_IFREG | 0644;
	newchild->data.st.st_blocks = 0;
	time_t T;
	time(&T);

	newchild->data.st.st_atime = T;
	newchild->data.st.st_mtime = T;
	newchild->data.st.st_ctime = T;
}


//Function to make directory
static int fsys_mkdir(const char *path, mode_t mode) {

	Node *parent = NULL;
	int valid = check_path(path, &parent);

	if(valid) {
		return -EEXIST;
	}

	char * ptr = strrchr(path, '/');
	char tmp[MAX_NAME];
	strncpy(tmp, path, ptr - path);
	tmp[ptr - path] = '\0';

	valid = check_path(tmp, &parent);
	if (!valid) {
		return -ENOENT;
	}
	Node * newchild = NULL;
	int ret = allocate_node(&newchild);

	if(ret != 0) {
		return ret;
	}

	ptr++;
	init_for_dir(newchild, ptr);
	
	parent->data.st.st_nlink = parent->data.st.st_nlink + 1;

	newchild->parent = parent;
	newchild->next = parent->firstchild;
	parent->firstchild = newchild;

	change_timestamps_dir(parent);

	return 0;
}

//Fuction to truncate file contents
static int fsys_truncate(const char* path, off_t size) {
	
	time_t T;
	Node * node = NULL;
	int valid = check_path(path, &node);
	int filelen = node->data.st.st_size;
	if (!valid) {
		return -ENOENT;
	}
	if (node->data.isdir)   { return -EISDIR; }
	if (size == filelen)    { return 0; }
        //if (freememory < size)  { return -ENOSPC; }

	if (size == 0) {
		if (node->filedata != NULL) {
			free(node->filedata);
			node->filedata = NULL;
			freememory = freememory+filelen;
		}
	} else  {

		long int space_needed = size - filelen;
		if (space_needed > freememory) { return -ENOSPC; }

		char * newptr = realloc(node->filedata, size * sizeof(char));
		if (!newptr) {
			return -ENOSPC;
		} else {
                	// Old pointer becomes invalid , we have to amke it valid again
			node->filedata = newptr;
			if (size > filelen) {
				memset(node->filedata + filelen, 0 , size - filelen);
			}
			freememory = freememory - space_needed;
		}
	}
	node->data.st.st_size = size;
	time(&T);
	node->data.st.st_ctime = T;
	node->data.st.st_mtime = T;
	return 0;
}

//Function to write to a file
static int fsys_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {

	time_t T;
	Node * node = NULL;
	int valid = check_path(path, &node);
	int filelen = node->data.st.st_size;
	if (!valid) {
		return -ENOENT;
	}
	if (node->data.isdir)   { return -EISDIR; }
	if (size == 0)          { return 0; }
	if(freememory - (sizeof(char) * size) < 0)
		  { return -ENOSPC; }
	if(filelen == 0) //file initially empty and ignore offset
	{	
		node->filedata = malloc(sizeof(char) * size);		
		node->data.st.st_size = size;
		freememory -= size;
		memcpy(node->filedata,buf,size);
	}
	else if((offset + size) > filelen)
	{
		node->filedata = realloc(node->filedata, sizeof(char) * (offset + size));
		memcpy(node->filedata + offset, buf, size);      
		node->data.st.st_size = offset + size;
		freememory -= (sizeof(char) * (offset + size - filelen));
	}
	//need a lil size changes here
	else if((offset + size) < filelen)	
	{
		node->filedata = realloc(node->filedata, sizeof(char) * (offset + size));
		memcpy(node->filedata + offset, buf, size);      
		node->data.st.st_size = offset + size;
		freememory -= (sizeof(char) * (offset + size - filelen));
	}
	float store = (node->data.st.st_size)/(512.0);
	if(node->data.st.st_size == 0) {node->data.st.st_blocks = 0; }
	else if(((int)store/8 == 0 || store == 8.0) && node->data.st.st_size != 0) { node->data.st.st_blocks = 8;}
	else if(store > (node->data.st.st_blocks))
		node->data.st.st_blocks += 8;
	else if(node->data.st.st_blocks - store > 8)
		node->data.st.st_blocks -= 8; 
	time(&T);
	node->data.st.st_ctime = T;
	node->data.st.st_mtime = T;
	return size;	  


/*
//HERe --------------------------------------------------//
	if(mem_avail - (sizeof(char) * size) < 0)
		return -ENOMEM;
	if(fsize == 0) //file initially empty and ignore offset
	{	
		node->data = malloc(sizeof(char) * size);		
		node->attrs->st_size = size;
		mem_avail -= size;
		memcpy(node->data,buffer,size);
	}
	else if((offset + size) > fsize)
	{
		node->data = realloc(node->data, sizeof(char) * (offset + size));
		memcpy(node->data + offset, buffer, size);      
		node->attrs->st_size = offset + size;
		mem_avail -= (sizeof(char) * (offset + size - fsize));
	}
	//need a lil size changes here
	else if((offset + size) < fsize)	
	{
		node->data = realloc(node->data, sizeof(char) * (offset + size));
		memcpy(node->data + offset, buffer, size);      
		node->attrs->st_size = offset + size;
		mem_avail -= (sizeof(char) * (offset + size - fsize));
	}*/
}


//Function to create a New file
static int fsys_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
	
	Node *parent = NULL;
	int valid = check_path(path, &parent);

	if(valid) {
		return -EEXIST;
	}

	char * ptr = strrchr(path, '/');
	char tmp[MAX_NAME];
	strncpy(tmp, path, ptr - path);
	tmp[ptr - path] = '\0';

	valid = check_path(tmp, &parent);
	if (!valid) {
		return -ENOENT;
	}
	Node * newchild = NULL;
	int ret = allocate_node(&newchild);

	if(ret != 0) {
		return ret;
	}

	ptr++;
	init_for_file(newchild, ptr);
	
	parent->data.st.st_nlink = parent->data.st.st_nlink + 1;

	newchild->parent = parent;
	newchild->next = parent->firstchild;
	parent->firstchild = newchild;

	change_timestamps_dir(parent);

	return 0;
}

//Function to remove file/directory from tree Hierarchy
void remove_from_ds (Node * child) {
	Node * parent = child->parent;
	if (parent == NULL) { return;}

	if (parent->firstchild == child) {
		parent->firstchild = child->next;
	} else {
		Node * tmp = parent->firstchild;
		while (tmp != NULL) {
			if (tmp->next == child) {
				tmp->next = child->next;
				break;
			}
			tmp = tmp->next;
		}
	}
	parent->data.st.st_nlink--;
	change_timestamps_dir(parent);
}

//Function to remove directory
static int fsys_rmdir(const char *path) {
	Node * node = NULL;
	int valid = check_path(path, &node);
	if (!valid) {
		return -ENOENT;
	}
	if (!node->data.isdir) { return -ENOTDIR;   }
	if (node->firstchild)  { return -ENOTEMPTY; }
	remove_from_ds(node);
	free(node);
	freememory = freememory + sizeof(Node);
	return 0;
}

//Function to remove the file from Tree Structure
static int fsys_unlink(const char* path) {
	
	Node * node = NULL;
	int valid = check_path(path, &node);
	if (!valid) {
		return -ENOENT;
	}
	if (node->data.isdir) { return -EISDIR;}
	remove_from_ds(node);
	long freed_mem = sizeof(Node);
	if (node->filedata != NULL) {
		freed_mem = freed_mem + node->data.st.st_size;
		free(node->filedata);
		node->filedata = NULL;
	}
	free(node);
	freememory = freememory + freed_mem;
	return 0;
}

//Function to perform Rename of file 
static int fsys_rename(const char * from, const char * to) {
	Node * node1 = NULL;
	Node * node2 = NULL;

	int valid = check_path(from, &node1);
	if (!valid) {
		return -ENOENT;
	}

	valid = check_path(to, &node2);

	char * ptr = strrchr(to, '/');
	char tmp[MAX_NAME];
	strncpy(tmp, to, ptr - to);
	tmp[ptr - to] = '\0';
	ptr++;

	// check if to path exists nor not.
	if (!valid) {
		// check if parent of to path is valid
		valid = check_path(tmp, &node2);

		if (!valid) {
			return -ENOENT;
		}

	} else {  // To path exists
		if (node2->data.isdir) {		
			if (node2->firstchild) {
				return -ENOTEMPTY;
			}
			node2 = node2->parent;
			fsys_rmdir(to);
		} else {
			node2 = node2->parent;
			fsys_unlink(to);
		}	
	}
	remove_from_ds(node1);
	node1->parent = node2;
	node1->next = node2->firstchild;
	node2->firstchild = node1;
	node2->data.st.st_nlink++;
	strncpy(node1->data.name, ptr, MAX_NAME);
	
	time_t T;
	time(&T);
	node1->data.st.st_ctime = T;
	
	change_timestamps_dir(node2);
	return 0;
}

FILE * diskfile;

//Function to Store Tree Structure recurusively in Persistent Storage
void serialize(Node * parent) {

	//fprintf(stderr, "%s\n",parent->data.name);
	int num_child = parent->data.st.st_nlink - 2;
	int i = 0;
	Node * temp = parent->firstchild;
	for (;i<num_child; i++) {
		fwrite(&temp->data, sizeof(Ndata), 1, diskfile);
		if (temp->data.isdir) {
			serialize(temp);
		} else {
			int filelen = temp->data.st.st_size;
			fwrite(temp->filedata, sizeof(char), filelen, diskfile);
		}
		temp = temp->next;
	} 
}
//Funtion to Store file contents when File system in unmounted
void fsys_destroy(void* private_data) {
	if  (filedump[0] == '\0') {
		return;
	} 
	diskfile = fopen(filedump, "w+b");
	if (diskfile) {
		//write DS to disk
		fwrite(&Root->data, sizeof(Ndata), 1, diskfile);
		serialize(Root);
		fclose(diskfile);
	}
}

//Function to load the file system from Persistenr storage by Recreating the tree Structure
void deserialize(Node * parent) {
	//fprintf(stderr, "deserial%s\n",parent->data.name);
	int num_child = parent->data.st.st_nlink -2;	
	//fprintf(stderr, "deserial=%d\n",num_child);
	int i;
	Node * x;
	Node * cur;
	if (num_child == 0) {
		return;
	}
	allocate_node(&x);
	parent->firstchild = x;
	x->parent = parent;
	cur = x;

	for (i=1; i< num_child; i++) {
		Node * y;
		int ret = allocate_node(&y);
		if (ret != 0) {
			fprintf(stderr,"deserialize: No space left on device\n");
			return;
		}
		cur->next = y;
		y->parent = parent;
		cur = y;
	}

	Node * temp = parent->firstchild;
	for (i=0; i<num_child; i++) {
		fread(&temp->data, sizeof(Ndata), 1, diskfile);
		if (temp->data.isdir) {
			deserialize(temp);
		} else {
			int filelen = temp->data.st.st_size;
			if (filelen > freememory) {
				fprintf(stderr,"deserialize: No space left on device\n");
				return;
			}
			temp->filedata = calloc(filelen, sizeof(char));
			if (temp->filedata == NULL) {
				fprintf(stderr,"deserialize: Not enough memory\n");
				return;
			}
			freememory -= filelen;
			fread(temp->filedata, sizeof(char), filelen, diskfile);
		}
		temp = temp->next;
	} 
}

static int fsys_opendir(const char *path, struct fuse_file_info *fi) {
	Node *p= NULL;
	int valid = check_path(path, &p);
	if (!valid) {
		return -ENOENT;
	}
	if (!p->data.isdir) {
		return -ENOTDIR;
	}
	return 0;
}

static struct fuse_operations hello_oper = {
	.getattr	= fsys_getattr,
	.readdir	= fsys_readdir,
	.open		= fsys_open,
	.read		= fsys_read,
	.utimens        = fsys_utime,
	.rmdir		= fsys_rmdir,
	.mkdir		= fsys_mkdir,
	.create         = fsys_create,
	.write          = fsys_write,
	.truncate	= fsys_truncate,
	.unlink 	= fsys_unlink,
	.rename     	= fsys_rename,
	.destroy	= fsys_destroy,
	.opendir	= fsys_opendir,    
};

int main(int argc, char *argv[])
{
	

	freememory = 256 * 1024;//50 * 1024 * 1024;  //provide max memory size available
	if (freememory <= 0) {
		fprintf(stderr, "Invalid Memory Size\n");
		return -1;
	}
	int init_done = 0;

	
		strncpy(filedump, "/home/prithvi/Desktop/IOS_Lab/persist", MAX_NAME);
		diskfile = fopen(filedump,"rb");
		if (diskfile) {
        	// Read from disk into ds
        	allocate_node(&Root);
        	fread(&Root->data, sizeof(Ndata), 1, diskfile);
        	deserialize(Root);
			init_done = 1;
			fclose(diskfile);
		}
		//argc--;
	
	//initialize the root
	if (!init_done) {
		
		Root = (Node *)calloc(1, sizeof(Node));
		strcpy(Root->data.name, "/");
		Root->data.isdir = 1;
		Root->data.st.st_nlink = 2;   // . and ..
		Root->data.st.st_uid = 0;
		Root->data.st.st_gid = 0;
		Root->data.st.st_mode = S_IFDIR |  0755; //755 is default directory permissions

		time_t T;
		time(&T);

		Root->data.st.st_size = 4096;
		Root->data.st.st_blocks = 8;
		Root->data.st.st_atime = T;
		Root->data.st.st_mtime = T;
		Root->data.st.st_ctime = T;
		freememory = freememory - sizeof(Node);
	}
	return fuse_main(argc, argv, &hello_oper, NULL);
}
