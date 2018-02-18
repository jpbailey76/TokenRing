CC = gcc
CFLAGS = -g -Wall
OBJECTS = *.c
HOST = bbserver
CLIENT = bbpeer
TODELETE = *.o

$HOST : $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o $(HOST)

$CLIENT : $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o $(CLIENT)

.PHONY: clean
clean:
	rm -f $(TODELETE)