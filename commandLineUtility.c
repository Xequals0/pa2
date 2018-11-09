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

int fd;
int cryptfd;
char ch;
int devindex;
char nameBuf[10];
char write_buf[1000];
char read_buf[1000];        

typedef struct{
    int pair;
    char key[256];
    int changed;
} keyStruct;

typedef struct{
    int pair;
    int deleted;
} delStruct;
    
delStruct deleteDev;
keyStruct changeKey;
keyStruct createDev;  

int main() {
    devindex = 0;      
	fd = open(DEVICE, O_RDWR); //open fir reading and writing

	if(fd == -1){
		printf("%s either does not exist or has been locked by another process\n", DEVICE);
		exit(-1);
	}
    
    printf("Enter a command: \n\t c = create an encrypt/decrypt pair \n\t r = delete an encrypt/decrypt pair \n\t e = encrypt<plaintext> \n\t d = decrypt<ciphertext> \n\t k = change key\n\t q = quit\n\t$ ");
    scanf(" %c", &ch);
    
    while(1){
        //printf("\n");
        switch(ch){
            case 'c':
                printf("Please specify a key for the pair (256 byte limit): ");
                scanf("%s", createDev.key);
                if(createDev.key[0] == '\0'){
                    printf("Invalid or empty key");
                    break;
                }else{
                    ioctl(fd, CREATE_IOCTL, &createDev);
                    printf("\n /dev/cryptEncrypt%d\n /dev/cryptDecrypt%d \n have been created...",createDev.pair,createDev.pair);
                }
                break;
            case 'r':
                printf("\nSpecify the index of the device to be deleted: ");
                scanf("%d", &deleteDev.pair);
                ioctl(fd, DELETE_IOCTL, &deleteDev);

                if(deleteDev.deleted == 0) printf("\n /dev/cryptEncrypt%d\n /dev/cryptDecrypt%d \n have been deleted...",devindex,devindex);
                else printf("\n Unable to delete specified device - either hasn't been created or has already been deleted");
                break;
            case 'e':
                printf("\nPlease specify the index of the device pair that you would like to use: ");
                scanf("%d", &devindex);
                sprintf(nameBuf, ENCDEV"%d", devindex);                
                cryptfd = open(nameBuf, O_RDWR);
                if (cryptfd == -1) printf("The specified crypt device pair does not exist.\n");
                else
                {
                    printf("Please provide the plaintext to be encrypted (10000 byte limit): \n");
                    scanf(" %[^\n]", write_buf);
                    write(cryptfd, write_buf, sizeof(write_buf));
                    read(cryptfd, read_buf, sizeof(read_buf));
                    printf("The ciphertext is: %s\n", read_buf);
                }
                break;
            case 'd':
                printf("\nPlease specify the index of the device pair that you would like to use: ");
                scanf("%d", &devindex);
                sprintf(nameBuf, DECDEV"%d", devindex);                
                cryptfd = open(nameBuf, O_RDWR);
                if (cryptfd == -1) printf("The specifed crypt device pair does not exist.\n");
                else
                {
                    printf("Please provide the ciphertext to be decrypted (10000 byte limit): \n");
                    scanf(" %[^\n]", write_buf);
                    write(cryptfd, write_buf, sizeof(write_buf));
                    read(cryptfd, read_buf, sizeof(read_buf));
                    printf("The plaintext is: %s\n", read_buf);
                }
                break;
            case 'k':
                printf("\nPlease specify the index of the device pair that you would like to modify: ");
                scanf("%d", &(changeKey.pair));
                printf("\nPlease provide the new key (256 byte limit): ");
                scanf("%s",changeKey.key);
                if(changeKey.key[0] == '\0'){
                    printf("Invalid or empty key");
                    break;
                }else{
                    ioctl(fd, CHANGE_IOCTL, &changeKey);
                    if(changeKey.changed == 0)printf("\n /dev/cryptEncrypt%d\n /dev/cryptDecrypt%d \n have been updated with the new key...",changeKey.pair,changeKey.pair);
                    else printf("\n The specified device pair does not exist \n");
                }
                break;
            case 'q':
                break;
            default:
                printf("command not recognized: %c\n", ch);
                break;
        }
        if(ch == 'q'){
            break;
        }
        printf("\nEnter a command:\n\t$ ");
        scanf(" %c", &ch);
    }
    
    //Cleanup + exit calls:
	close(fd);
	return 0;
}
