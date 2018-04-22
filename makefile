CC = gcc
CFLAGS = -g -Wall -lpthread
OBJECTS = bbserver bbpeer
all: $(OBJECTS)
TODELETE = *.o

$(objects): %: %.c
	$(CC) $(CFLAGS) -o $@ $<

.PHONY: clean
clean:
	rm -f $(TODELETE)