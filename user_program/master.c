#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>


#define PAGE_SIZE 4096
#define BUF_SIZE 512

size_t get_filesize(const char* filename);//get the size of the input file


int main (int argc, char* argv[])
{
	char buf[BUF_SIZE];
	int i, dev_fd, file_fd;// the fd for the device and the fd for the input file
	size_t ret, file_size, offset = 0, tmp;
	char file_name[50], method[20];
	char *kernel_address = NULL, *file_address = NULL;
	struct timeval start;
	struct timeval end;
	double trans_time; // calulate the time between the device is opened and it is closed
	int N = atoi(argv[1]); // number of files to send
	if( (dev_fd = open("/dev/master_device", O_RDWR)) < 0){
		perror("failed to open /dev/master_device\n");
		return 1;
	}
	
	gettimeofday(&start ,NULL); // start time

	if(ioctl(dev_fd, 0x12345677) == -1){ //0x12345677 : create socket and accept the connection from the slave
		perror("ioclt server create socket error\n");
		return 1;
	}

	strncpy(method, argv[argc - 1], sizeof(method)); // policy

	for(i = 0 ; i < N ; i++){
		strncpy(file_name, argv[2+i], sizeof(file_name)); // filepath
		if( (file_fd = open(file_name, O_RDWR)) < 0 ){
			perror("failed to open input file\n");
			return 1;
		}

		if( (file_size = get_filesize(file_name)) < 0){
			perror("failed to get filesize\n");
			return 1;
		}
		char size[512];
		sprintf(size, "%zu", file_size);
		write(dev_fd, size, 512); // tell the reciever how large is the file
		//offset += 32

		switch(method[0]){
			case 'f': //fcntl : read()/write()
				do{					
					ret = read(file_fd, buf, sizeof(buf)); // read from the input file
					write(dev_fd, buf, ret);//write to the the device
				}	while(ret > 0);
			break;
			case 'm':
				for(ret = 0 ; ret < file_size ; ret += PAGE_SIZE){
					/*
					if( (kernel_address = mmap(NULL, PAGE_SIZE, PROT_WRITE, MAP_SHARED, dev_fd, offset+ret)) == (void *) -1){
						perror("failed to map /dev/master_device\n");
						return 1;
					}
					*/
					if( (file_address = mmap(NULL, PAGE_SIZE, PROT_READ, MAP_SHARED, file_fd, ret)) == (void *) -1){
						perror("failed to map input file\n");
						return 1;
					}

					write(dev_fd, file_address, PAGE_SIZE < (file_size - ret) ? PAGE_SIZE : (file_size - ret));
write(1, file_address, PAGE_SIZE < (file_size - ret) ? PAGE_SIZE : (file_size - ret));
					munmap(file_address, PAGE_SIZE);
				}
				//offset += file_size;
			break;
		}
		gettimeofday(&end, NULL);
		trans_time = (end.tv_sec - start.tv_sec)*1000 + (end.tv_usec - start.tv_usec)*0.0001;
		printf("Transmission time: %lf ms, File size: %d bytes\n", trans_time, file_size / 8);
	}
	if(ioctl(dev_fd, 0x12345679) == -1){ // end sending data, close the connection
		perror("ioclt server exits error\n");
		return 1;
	}
	
	close(file_fd);
	close(dev_fd);
	
	return 0;
}

size_t get_filesize(const char* filename){
    struct stat st;
    stat(filename, &st);
    return st.st_size;
}
