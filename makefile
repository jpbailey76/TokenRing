CC = gcc
CFLAGS = -g -Wall
OBJECTS = *.c
OUT = bbserver
TODELETE = $(OUT) *.o

$OUT : $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o $(OUT)

.PHONY: clean
clean:
	rm -f $(TODELETE)