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
int main (int argc, char* argv[])
{
	char buf[BUF_SIZE];
	int i, dev_fd, file_fd;// the fd for the device and the fd for the input file
	size_t ret, file_size = 0, data_size = -1;
	char file_name[50];
	char method[20];
	char ip[20];
	struct timeval start;
	struct timeval end;
	double trans_time; //calulate the time between the device is opened and it is closed
	char *kernel_address, *file_address;

	int N = atoi(argv[1]);
    strcpy(method, argv[N+2]);
	strcpy(ip, argv[N+3]);
	if( (dev_fd = open("/dev/slave_device", O_RDWR)) < 0)//should be O_RDWR for PROT_WRITE when mmap()
	{
		perror("failed to open /dev/slave_device\n");
		return 1;
	}
	
	
	for(int i = 0; i < N; i++){
		
		gettimeofday(&start ,NULL);
		strcpy(file_name, argv[i+2]);
		if( (file_fd = open (file_name, O_RDWR | O_CREAT | O_TRUNC)) < 0)
		{
			perror("failed to open input file\n");
			return 1;
		}

		if(ioctl(dev_fd, 0x12345677, ip) == -1)	//0x12345677 : connect to master in the device
		{
			perror("ioclt create slave socket error\n");
			return 1;
		}

		write(1, "ioctl success\n", 14);

		switch(method[0])
		{
			case 'f'://fcntl : read()/write()
				size_t remain;
				read(dev_fd, buf, 32);
				sscanf(buf, "%zu", &remain);
				file_size = remain;
				do
				{
					if(remain > sizeof(buf))
						ret = read(dev_fd, buf, sizeof(buf)); // read from the the device
					else
						ret = read(dev_fd, buf, remain); // read from the the device
					write(file_fd, buf, ret); //write to the input file
					remain -= ret;
				}while(remain > 0);
				break;
				
			case 'm'://mmap
				size_t remain;
				read(dev_fd, buf, 32);
				sscanf(buf, "%zu", &remain);
				file_size = remain;
				ftruncate(file_fd, file_size);
				
				char *mfile;
				int cursor = 0;
				
				while(remain > 0){
					ret = read(dev_fd, buf, sizeof(buf));
					mfile = mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, file_fd, cursor);
					cursor += ret;
					memcpy(mfile, buf, ret);
					munmap(mfile, PAGE_SIZE);
					remain -= ret;
				}
				
				break;
		}

		gettimeofday(&end, NULL);
		trans_time = (end.tv_sec - start.tv_sec)*1000 + (end.tv_usec - start.tv_usec)*0.0001;
		printf("Transmission time: %lf ms, File size: %d bytes\n", trans_time, file_size / 8);


		close(file_fd);
		
	}
	
	if(ioctl(dev_fd, 0x12345679) == -1)// end receiving data, close the connection
	{
		perror("ioclt client exits error\n");
		return 1;
	}
	
	close(dev_fd);
	return 0;
}


