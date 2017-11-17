
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "../my_pthread_t.h"

// Here, we test our ability to perform a basic allocation for
// one single thread.
int main(int argc, char **argv){
	
	int val = 9001;
	int * pointer = (int*)malloc(sizeof(int));
	*pointer = val; 
	printf("pointer's value should be %d; value is actually: %d\n", val, *pointer);

	free(pointer);

	printf("Finished freeing pointer!\n");
	printf("Finished test 1 successfully!\n");
	
	return 0;
}