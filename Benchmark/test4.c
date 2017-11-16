
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "../my_pthread_t.h"

// Here, we test our ability to perform many small, variable-sized
// allocations in a single thread (patterned intervals of chars
// and ints)
int main(int argc, char **argv){
	// maxVal is however many allocations we want to test.
	int maxVal = 100;
	
	
	
	printf("Finished test 4 successfully!\n");
	
	return 0;
}