.POSIX:
.PHONY: all check clean

CFLAGS+=-Wall -Wpedantic

all: b3sum

blake3.o blake3-test.o b3sum.o: blake3.h

b3sum: b3sum.o blake3.o

blake3-test: blake3-test.o blake3.o

check: blake3-test
	./blake3-test

clean:
	rm -f blake3.o blake3-test blake3-test.o b3sum b3sum.o
