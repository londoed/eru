eru: eru.o
	$(CC) eru.c -o eru -Wall -Wextra -pedantic -std=c99

eru.o: eru.c eru.h
