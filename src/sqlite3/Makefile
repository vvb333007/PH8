CC = gcc
CFLAGS = -O2 -Wall -DPH7_ENABLE_SQLITE3=1

sqlite3.o: sqlite3.c
	$(CC) $(CFLAGS) -c sqlite3.c -o sqlite3.o

clean:
	rm -f sqlite3.o