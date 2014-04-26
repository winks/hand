CC=gcc
CFLAGS=-Wall

all: clean hand

run:
	./hand

hand:
	${CC} ${CFLAGS} -o hand hand.c

static:
	${CC} ${CFLAGS} -o hand.static -static hand.c

clean:
	rm -f hand hand.static

.PHONY: clean
