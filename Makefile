CFLAGS = -O3
CC = gcc
BUNDLE = Makefile tclled.h tclled.c tcltest.c tetris.c hashtable.h hashtable.c 
VERSION = 0.5
ARCHIVE = blinky_tetris

all: tcltest hashtable tetris

archive: $(BUNDLE)
	mkdir $(ARCHIVE)-$(VERSION)
	cp $(BUNDLE) $(ARCHIVE)-$(VERSION)/
	tar cvfz $(ARCHIVE)-$(VERSION).tar.gz $(ARCHIVE)-$(VERSION)
	rm -rf $(ARCHIVE)-$(VERSION)

clean:
	$(RM) *.o
	$(RM) $(ARCHIVE)-$(VERSION).tar.gz

tcltest: tcltest.o tclled.o
	$(CC) $(CFLAGS) -o tcltest $^

tetris: tetris.o tclled.o hashtable.o
	$(CC) $(CFLAGS) -o tetris $^

hashtest: hashtest.o hashtable.o 
	$(CC) $(CFLAGS) -o $@ $^

tcltest.o: tclled.h tcltest.c

tclled.o: tclled.h tclled.c

tetris.o: tclled.h tetris.c

hashtable.o: hashtable.h hashtable.c
