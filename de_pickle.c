
void pickle(){

	int fd = open("", O_CREAT | O_WRONLY | O_TRUNC, 0755);
	printf("file descriptor: %d\n", fd);
	int i;
	for(i=0; i<MAX_DIR_NR; i++){
		write(fd, used_dir_indices+i, sizeof(int));
		if(used_dir_indices[i]){ //used_dir_indices is a bit-vector : 1 means field is occupied in dirs[]
			write(fd, dirs+i, sizeof(dir));
		}
	}

	for(i=0; i<MAX_FILE_NR; i++){
		write(fd, used_file_indices+i, sizeof(int));
		if(used_file_indices[i]){  //used_file_indices is a bit-vector : 1 means field is occupied in files[]
			write(fd, files+i, sizeof(fcb));
			// write(fd, "\n", 1);
		}
	}

	close(fd);
}

int depickle(){
	int fd = open("", O_RDONLY);

	if(fd == -1){
		return 0;
	}
	printf("Saved state found\n");

	printf("file descriptor: %d\n", fd);
	int i;
	printf("Populating directory index\n");
	for(i=0; i<MAX_DIR_NR; i++){
		read(fd, used_dir_indices+i, sizeof(int));
		if(used_dir_indices[i]){
			read(fd, dirs+i, sizeof(dir));
		}
	}

	printf("Populating file index\n");
	for(i=0; i<MAX_FILE_NR; i++){
		read(fd, used_file_indices+i, sizeof(int));
		if(used_file_indices[i]){
			read(fd, files+i, sizeof(fcb));
			// read(fd, "\n", 1);
		}
	}

	printf("Depickling completed\n");
	close(fd);

	return 1;
}
