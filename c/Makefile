CC?=gcc
SCC?=/usr/local/musl/bin/musl-gcc
CFLAGS?=-Wall -Werror -Wextra -Wfloat-equal -Wundef -Wshadow -Wpointer-arith -ftrapv -Wcast-align -Wstrict-prototypes -Wstrict-overflow=5 -Waggregate-return -Wcast-qual -Wswitch-default -Wswitch-enum
OFLAGS?=-O3

all: clean hand shand static sstatic

run:
	./bin/hand -l

hand:
	${CC} ${CFLAGS} ${OFLAGS} -o bin/hand hand.c

shand:
	${SCC} ${CFLAGS} ${OFLAGS} -o bin/shand hand.c
	strip bin/shand

static:
	${CC} ${CFLAGS} ${OFLAGS} -o bin/hand.static -static hand.c

sstatic:
	${SCC} ${CFLAGS} ${OFLAGS} -o bin/shand.static -static hand.c

clean:
	rm -f bin/*

.PHONY: clean
