CC = gcc
CFLAGS = -g -Wall
OBJECTS = bbserver bbpeer
all: $(OBJECTS)
TODELETE = *.o

$(objects): %: %.c
        $(CC) $(CFLAGS) -o $@ $<

.PHONY: clean
clean:
	rm -f $(TODELETE)