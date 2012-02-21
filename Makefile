CC = gcc
CCFLAGS = -Wall -I. -lm -g -ggdb
CFLAGS = -Wall -I. -g -ggdb
TARGETS = ntsc-test-data

.PHONY: all clean wipe

all: $(TARGETS) $(MODULES) $(SCRIPTS)

ntsc-test-data: ntsc-test-data.o shared.o
	$(CC) -o $@ $^ $(CCFLAGS)

clean:
	rm -f *.o *~

wipe: clean
	rm -f $(TARGETS)
