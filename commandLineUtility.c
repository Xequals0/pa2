#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#define DEVICE "/dev/cryptctl"

int main() {
	int i; //number of bytes written/read
	int fd;
	char ch, write_buf[10000], read_buf[10000];
    int counter = 0;
    int pairs[100] = {0};
    char key[100];
    int index = 0;
    /*
	fd = open(DEVICE, O_RDWR); //open fir reading and writing

	if(fd == -1){
		printf("%s either does not exist or has been locked by another process\n", DEVICE);
		exit(-1);
	}*/
    
    printf("Enter a command: \n\t c = create an encrypt/decrypt pair \n\t r = delete an encrypt/decrypt pair \n\t e = encrypt<plaintext> \n\t d = decrypt<ciphertext> \n\t k = change key\n\t q = quit\n\t$ ");
    scanf("%c", &ch);
    
    while(1){
        printf("\n");
        switch(ch){
            case 'c':
                printf("Please specify a key for the pair (100 byte limit): ");
                scanf("%s",key);
                if(key[0] == '\0'){
                    printf("Invalid or empty key");
                    break;
                }else{
                    //Call the create ioctl() call here; return the index of the created pair
                    printf("\n /dev/cryptEncrypt%d\n /dev/cryptDecrypt%d \n have been created...",counter,counter);
                    pairs[counter] = 1;
                    counter++;
                }
                break;
            case 'r':
                printf("\nSpecify the index of the device to be deleted: ");
                scanf("%d", &index);
                if (pairs[index] == 0) {
                    printf("The following crypt device pair has not been created yet or has already been deleted.\n");
                }
                else
                {
                    pairs[index] = 0;
                    //Call the delete ioctl() call here
                    printf("\n /dev/cryptEncrypt%d\n /dev/cryptDecrypt%d \n have been deleted...",index,index);
                }
                break;
            case 'e':
                printf("\nPlease specify the index of the device pair that you would like to use: ");
                scanf("%d", &index);
                if (pairs[index] == 0) {
                    printf("The following crypt device pair does not exist.\n");
                }
                else
                {
                    printf("Please provide the plaintext to be encrypted (10000 byte limit): \n");
                    scanf(" %[^\n]", write_buf);
                    write(fd, write_buf, sizeof(write_buf));
                    //make the encrypt call here
                    read(fd, read_buf, sizeof(read_buf));
                    printf("The ciphertext is: %s\n", read_buf);
                }
                /*read(fd, read_buf, sizeof(read_buf));
                printf("device: %s\n", read_buf);*/
                break;
            case 'd':
                printf("\nPlease specify the index of the device pair that you would like to use: ");
                scanf("%d", &index);
                if (pairs[index] == 0) {
                    printf("The following crypt device pair does not exist.\n");
                }
                else
                {
                    printf("Please provide the ciphertext to be decrypted (10000 byte limit): \n");
                    scanf(" %[^\n]", write_buf);
                    write(fd, write_buf, sizeof(write_buf));
                    //make the decrypt call here
                    read(fd, read_buf, sizeof(read_buf));
                    printf("The plaintext is: %s\n", read_buf);
                }
                break;
            case 'k':
                printf("\nPlease specify the index of the device pair that you would like to modify: ");
                scanf("%d", &index);
                if (pairs[index] == 0) {
                    printf("The following crypt device pair does not exist.\n");
                }
                else
                {
                    printf("\nPlease provide the new key (100 byte limit): ");
                    scanf("%s",key);
                    if(key[0] == '\0'){
                        printf("Invalid or empty key");
                        break;
                    }else{
                        //Call change key here
                        printf("\n /dev/cryptEncrypt%d\n /dev/cryptDecrypt%d \n have been updated with the new key...",index,index);
                    }
                }
                break;
            case 'q':
                break;
            default:
                //printf("command not recognized\n");
                break;
        }
        if(ch == 'q'){
            break;
        }
        printf("\nEnter a command:\n\t$ ");
        scanf("%c", &ch);
    }
    
    //Cleanup + exit calls:
	close(fd);
	return 0;
}
