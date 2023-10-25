CC = gcc
CFLAGS = -Wall
LDFLAGS = -pthread

helper_functions.o: helper_functions.c helper_functions.h
	$(CC) $(CFLAGS) -c helper_functions.c

cardreader: cardreader.o helper_functions.o
	$(CC) -o $@ $^ $(LDFLAGS)

cardreader.o: cardreader.c
	$(CC) -c $< $(CFLAGS)

clean:
	rm -f *.o