#include <stdio.h>
#include <string.h>
#include <ctype.h>

char decrypt(char key, char c){
	char ret = c - (key - 'a');// + 1);
	if (ret < 'a'){
		int shift = 'a' - ret ;
		return 'z' - shift + 1; 
	}
}

int main(int argc, const char* argv[]){

	if(argc != 3){
		printf("Incorrect number of arguements\n");
		return 1;
	}

	int size = strlen(argv[1]);
	char keys[size];
	strcpy(keys,argv[1]);
	
	size = strlen(argv[2]);
	char c[size];
	strcpy(c, argv[2]);

	int i,j;
	for(i=0, j = 0; i < strlen(c); i++, j++){
		if(j >= strlen(keys)){
			j=0;
		}
//		printf("j: %d\n", j);
		char key = keys[j];
		char ret = decrypt(key, c[i]);
		printf("returned: %c\n", ret);
	}
}

