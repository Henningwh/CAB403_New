CC = gcc
CFLAGS = -Wall
LDFLAGS = -pthread
ALL_COMPONENTS = overseer firealarm cardreader door callpoint tempsensor elevator destselect

# Compile helper functions
helper_functions.o: helper_functions.c helper_functions.h
	$(CC) $(CFLAGS) -c helper_functions.c

# Compile cardreader
cardreader.o: cardreader.c
	$(CC) -c $< $(CFLAGS)
cardreader: cardreader.o helper_functions.o
	$(CC) -o $@ $^ $(LDFLAGS)

# Compile callpoint
callpoint.o: callpoint.c
	$(CC) -c $< $(CFLAGS)
callpoint: callpoint.o helper_functions.o
	$(CC) -o $@ $^ $(LDFLAGS)

all: $(ALL_COMPONENTS)

clean:
	rm -f *.o $(ALL_COMPONENTS)