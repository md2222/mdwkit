#!/bin/make

SRCS := src/configfile.c src/main.c

CFLAGS  = `pkg-config --cflags gtk+-3.0` -Isrc -I/usr/include/webkitgtk-4.0 -I/usr/include/libsoup-2.4

LIBS    = `pkg-config --libs gtk+-3.0 webkit2gtk-4.0`

OUT := bin/mdwkit

all:
	mkdir -p bin
#	gcc -Os -s -DDEBUG $(SRCS) -o $(OUT) $(CFLAGS) $(LIBS)
	gcc -Os -s $(SRCS) -o $(OUT) $(CFLAGS) $(LIBS)
	chmod 755 $(OUT)
	make -C ext

clean:
	rm -f $(OUT)
