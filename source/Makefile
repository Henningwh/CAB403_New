CC = gcc
CFLAGS = -Wall
LDFLAGS = -pthread
# All comps minus overseer. Yet to be implemented
ALL_COMPONENTS = simulator firealarm cardreader door callpoint tempsensor elevator destselect

# Compile helper functions
helper_functions.o: helper_functions.c helper_functions.h
	$(CC) $(CFLAGS) -c helper_functions.c

# Compile simulator
simulator: simulator.o helper_functions.o
	$(CC) -o $@ $^ $(LDFLAGS)
simulator.o: simulator.c 
	$(CC) -c $< $(CFLAGS)

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

# Compile tempsensor
tempsensor.o: tempsensor.c
	$(CC) -c $< $(CFLAGS)
tempsensor: tempsensor.o helper_functions.o
	$(CC) -o $@ $^ $(LDFLAGS)

# Compile elevator
elevator.o: elevator.c
	$(CC) -c $< $(CFLAGS)
elevator: elevator.o
	$(CC) -o $@ $^ $(LDFLAGS)

all: $(ALL_COMPONENTS)

clean:
	rm -f *.o $(ALL_COMPONENTS)