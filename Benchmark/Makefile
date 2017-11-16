CC = gcc
CFLAGS = -g -w

all:: parallelCal vectorMultiply externalCal test1 test2

test1:
	$(CC) $(CFLAGS) -pthread -o test1 test1.c -L ../ -lmy_pthread -lm

test2:
	$(CC) $(CFLAGS) -pthread -o test2 test2.c -L ../ -lmy_pthread -lm

parallelCal: 
	$(CC) $(CFLAGS) -pthread -o parallelCal parallelCal.c -L ../ -lmy_pthread -lm

vectorMultiply: 
	$(CC) $(CFLAGS) -pthread -o vectorMultiply vectorMultiply.c -L ../ -lmy_pthread -lm

externalCal: 
	$(CC) $(CFLAGS) -pthread -o externalCal externalCal.c -L ../ -lmy_pthread -lm

clean:
	rm -rf parallelCal vectorMultiply externalCal test1 test2 *.o ./record/
