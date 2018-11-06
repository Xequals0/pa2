#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#define DEVICE "/dev/cryptctl"

int main() {
	int i; //number of bytes written/read
	int fd;
	char ch, write_buf[100], read_buf[100];

	fd = open(DEVICE, O_RDWR); //open fir reading and writing

	if(fd == -1){
		printf("file %s either does not exist or has been locked by another process\n", DEVICE);
		exit(-1);
	}

	printf("r = read from device\nw = write to device\netner command: ");
	scanf("%c", &ch);

	switch(ch){
		case 'w':
			printf("Enter data: ");
			scanf("%s", write_buf);
			write(fd, write_buf, sizeof(write_buf));
			break;
		case 'r':
			read(fd, read_buf, sizeof(read_buf));
			printf("device: %s\n", read_buf);
			break;
		default:
			printf("command not recognized\n");
			break;
	}

	close(fd);

	return 0;
}
