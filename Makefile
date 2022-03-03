CC = gcc
CFLAGS = -std=c99 -Wpedantic -Wall
DEBUG_FLAGS = -g
SRC = words.c
PREFIX = /usr/local

all:
	${CC} ${CFLAGS} ${SRC} -o words

install: all
	mkdir -p ${PREFIX}/bin
	cp -f words ${PREFIX}/bin
	chmod 755 ${PREFIX}/bin/words

uninstall:
	rm -f ${PREFIX}/bin/words

debug:
	${CC} ${CFLAGS} ${DEBUG_FLAGS} ${SRC} -o debug

memdebug:
	${CC} ${CFLAGS} ${DEBUG_FLAGS} -fsanitize=address ${SRC} -o debug

clean :
	rm -f debug words vgcore*

.PHONY: all debug memdebug clean install uninstall
