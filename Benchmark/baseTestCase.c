
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "../my_pthread_t.h"

int main(int argc, char **argv){

	
	
	int * pointer = (int*)malloc(sizeof(int));
	*pointer = 69; 

	free(pointer);
	
	return 0;
}