CC = gcc
CFLAGS = -g -Wall
OBJECTS = *.c
SERVER = bbserver
CLIENT = bbpeer
TODELETE = *.o

$SERVER : $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o $(SERVER)

$CLIENT : $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o $(CLIENT)

.PHONY: clean
clean:
	rm -f $(TODELETE)