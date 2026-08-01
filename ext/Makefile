#!/bin/make

SRCS := src/ext.cpp src/cgi.cpp

CFLAGS  = `pkg-config --cflags gtk+-3.0` -Isrc -I/usr/include/webkitgtk-4.0 -I/usr/include/libsoup-2.4

LIBS    = `pkg-config --libs gtk+-3.0 webkit2gtk-4.0`

OUT=lib/mdwkitext.so

all:
	mkdir -p lib
	g++ -std=c++11 -Os -s -fPIC -shared -DDEBUG $(SRCS) -o $(OUT) $(CFLAGS) $(LIBS)
	chmod 755 $(OUT)

clean:
	rm -f $(OUT)
