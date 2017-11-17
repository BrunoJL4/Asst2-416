CC = gcc
CFLAGS = -g -w

all:: parallelCal vectorMultiply externalCal test1 test2 test3 test4 test5

test1:
	$(CC) $(CFLAGS) -pthread -o test1 test1.c -L ../ -lmy_pthread

test2:
	$(CC) $(CFLAGS) -pthread -o test2 test2.c -L ../ -lmy_pthread

test3:
	$(CC) $(CFLAGS) -pthread -o test3 test3.c -L ../ -lmy_pthread

test4:
	$(CC) $(CFLAGS) -pthread -o test4 test4.c -L ../ -lmy_pthread 	

test5:
	$(CC) $(CFLAGS) -pthread -o test5 test5.c -L ../ -lmy_pthread 

parallelCal: 
	$(CC) $(CFLAGS) -pthread -o parallelCal parallelCal.c -L ../ -lmy_pthread

vectorMultiply: 
	$(CC) $(CFLAGS) -pthread -o vectorMultiply vectorMultiply.c -L ../ -lmy_pthread

externalCal: 
	$(CC) $(CFLAGS) -pthread -o externalCal externalCal.c -L ../ -lmy_pthread

clean:
	rm -rf parallelCal vectorMultiply externalCal test1 test2 test3 test4 test5 *.o ./record/
