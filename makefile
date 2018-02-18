CC = gcc
CFLAGS = -g -Wall
OBJECTS = bbserver bbpeer
TODELETE = *.o

$(objects): %: %.c
        $(CC) $(CFLAGS) -o $@ $<

.PHONY: clean
clean:
	rm -f $(TODELETE)