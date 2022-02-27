CC = gcc

CFLAGS = -std=c99 -Wpedantic -Wall

DEBUG_FLAGS = -g

SRC = words.c

all:
	${CC} ${CFLAGS} ${SRC} -o words

debug:
	${CC} ${CFLAGS} ${DEBUG_FLAGS} ${SRC} -o debug

memdebug:
	${CC} ${CFLAGS} ${DEBUG_FLAGS} -fsanitize=address ${SRC} -o debug

clean :
	rm -f debug words vgcore*

.PHONY: all debug memdebug clean # install uninstall
