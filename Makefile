DEPS=freetype2  xcb  x11-xcb xft
CFLAGS=-g -Wall -Wextra
VERSION=1.3

CFILES:=$(shell find src -type f -name '*.c')
DEPS:=$(shell pkg-config --libs --cflags $(DEPS))
$(if $(DEPS),,$(error "pkg-config failed"))
CFLAGS+=$(DEPS)
CFLAGS+=-DVERSION=\"$(VERSION)\"

all:
	-mkdir bin 2> /dev/null
	$(CC) $(CFLAGS) $(CFILES) -o bin/xmenu
install:
	install bin/xmenu /usr/bin
clean:
	-rm -rf bin
