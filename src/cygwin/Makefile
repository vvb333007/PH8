CC = gcc
CFLAGS = -O2 -Wall -I. -I..

freertos_shim.o: freertos_shim.c
	$(CC) $(CFLAGS) -c freertos_shim.c -o freertos_shim.o

clean:
	rm -f *.[od]