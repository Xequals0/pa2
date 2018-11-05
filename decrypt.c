#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

//Encryption: Ei = (Pi + Ki) % 26
char* encrypt(char* key, char c[]){
/*    int value = (c + key) % 26;
    value += 'a';
    return value; */

	int i;
	char currKey = *key;
	for(i = 0; c[i] != '\0'; i++){
		currKey = *key + (i % strlen(key));
		char newChar = c[i] + (currKey - 'a');
		if(newChar > 'z'){
			int shift = newChar - 'z';
			c[i] = 'a' + shift - 1;
		}
		else
			c[i] = newChar;
	}
	return c;
}

//Decryption: Di = (Ei - Ki + 26) % 26
char* decrypt(char* key, char c[]){
	
	int i;
	char currKey = *key;
	for(i =0; c[i] != '\0'; i++){
		currKey = *(key) + (i % strlen(key));
		char newChar = c[i] - (currKey - 'a');
		if(newChar < 'a'){
			int shift = 'a' - newChar;
			c[i] = 'z' - shift +1;
		}
		else{
			c[i] = newChar;	
		}
	}
	
    return c;
}

int main(int argc, const char* argv[]){
	char s[] = "abcdz";
	char* key = "b";
	char* s2 = decrypt(key, s);
	printf("s: %s\n", s2);
	
	return 0;
}

