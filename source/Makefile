CC = gcc
CFLAGS = -Wall
LDFLAGS = -pthread
ALL_COMPONENTS = overseer simulator firealarm cardreader door callpoint tempsensor elevator destselect

# Compile helper functions
helper_functions.o: helper_functions.c helper_functions.h
	$(CC) $(CFLAGS) -c helper_functions.c

# COmpile ipLibTCP
ipLibTCP.o: ipLibTCP.c
	$(CC) -c $< $(CFLAGS)

# Compile overseer
overseer.o: overseer.c
	$(CC) -c $< $(CFLAGS)
ipLibTCP.o: ipLibTCP.c
	$(CC) -c $< $(CFLAGS)
ipLibUDP.o: ipLibUDP.c
	$(CC) -c $< $(CFLAGS)
sleepFunction.o: sleepFunction.c
	$(CC) -c $< $(CFLAGS)
overseer: overseer.o ipLibTCP.o ipLibUDP.o sleepFunction.o
	$(CC) -o $@ $^ $(LDFLAGS)


# Compile simulator
simulator.o: simulator.c 
	$(CC) -c $< $(CFLAGS)
simulator: simulator.o helper_functions.o
	$(CC) -o $@ $^ $(LDFLAGS)

# Compile cardreader
cardreader.o: cardreader.c
	$(CC) -c $< $(CFLAGS)
cardreader: cardreader.o helper_functions.o
	$(CC) -o $@ $^ $(LDFLAGS)

# Compile door
door.o: door.c
	$(CC) -c $< $(CFLAGS)
door: door.o ipLibTCP.o helper_functions.o
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