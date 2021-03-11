#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define TRUE 1
#define FALSE 0

int main(int argc,char *argv[]) {
	int i;
	int temp;
	int length;
	length = atoi(argv[1]);
	char key[length+1];

	//seed rand()
	srand(time(NULL));

	//randomly generate keygen
	for (i = 0; i < length; i++) {
		temp = (rand()%27)+65;
		if (temp == 91) {
			key[i] = ' ';
		}
		else {
			key[i] = (char)temp;
		}
	} 

	//add null terminator
	key[length] = '\0';

	//print to stdout
	printf("%s\n", key);

	return 0;
}
