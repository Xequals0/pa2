#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#define DEVICE "/dev/cryptctl"
#define ENCDEV "/dev/encrypt"
#define DECDEV "/dev/decrypt"
#define IOCTLNUM 'k'
#define CREATE_IOCTL _IOW(IOCTLNUM, 1, int)
#define DELETE_IOCTL _IOW(IOCTLNUM, 2, int)
#define CHANGE_IOCTL _IOW(IOCTLNUM, 3, int)

int main() {
	int i; //number of bytes written/read
	int fd;
	int del;
	
	typedef struct{
		int pair;
		char key[256];
	} keyStruct;

	typedef struct{
		int pair;
		int deleted;
	} delStruct;

	char ch, write_buf[256], read_buf[100];

	keyStruct changeKey;
	keyStruct createDev;
	delStruct deleteDev;

	fd = open(DEVICE, O_RDWR); //open fir reading and writing

	if(fd == -1){
		printf("file %s either does not exist or has been locked by another process\n", DEVICE);
		exit(-1);
	}

	printf("r = read from device\nw = write to device\nc = create\nd = delete\nk = change key\netner command: ");
	scanf("%c", &ch);

	switch(ch){
		case 'w':
			printf("Enter data: ");
			scanf(" %[^\n]", write_buf);
			write(fd, write_buf, sizeof(write_buf));
			break;
		case 'r':
			read(fd, read_buf, sizeof(read_buf));
			printf("device: %s\n", read_buf);
			break;
		case 'c':
			printf("Enter key: ");
			scanf(" %[^\n]", createDev.key);
			ioctl(fd, CREATE_IOCTL, &createDev);
			printf("Pair created: %d\n",createDev.pair);
			break;
		case 'd':
			printf("Enter pair to delete: ");
			scanf("%d", &del);			
			ioctl(fd, DELETE_IOCTL, del);
			break;
		case 'k':
			printf("Enter pair to change: ");
			scanf("%d", &(changeKey.pair));
			printf("Enter new key: ");
			scanf(" %[^\n]", changeKey.key);			
			ioctl(fd, CHANGE_IOCTL, &changeKey);
			break;
		default:
			printf("command not recognized\n");
			break;
	}

	close(fd);

	return 0;
}
