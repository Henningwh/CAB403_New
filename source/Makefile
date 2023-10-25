CC = gcc
CFLAGS = -Wall
LDFLAGS = -pthread

helper_functions.o: helper_functions.c helper_functions.h
	$(CC) $(CFLAGS) -c helper_functions.c

cardreader.o: cardreader.c
	$(CC) -c $< $(CFLAGS)

cardreader: cardreader.o helper_functions.o
	$(CC) -o $@ $^ $(LDFLAGS)

clean:
	rm -f *.o