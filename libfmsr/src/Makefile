CC=gcc
CFLAGS=-fPIC -O3 -Wall -std=c99 -lpthread

SRCS=fmsr.c fmsrutil.c gf.c matrix.c misc.c
HDRS=fmsr.h
OBJS=$(SRCS:.c=.o)

all: ../lib/libfmsr.so headers

clean:
	rm -f $(OBJS) ../lib/libfmsr.so*
	make -C test clean
	(cd ../include && rm -f $(HDRS))

docs:
	doxygen Doxyfile

headers:
	mkdir -p ../include
	cp $(HDRS) ../include

test:
	make -C test

../lib/libfmsr.so: ../lib/libfmsr.so.0
	ln -sf libfmsr.so.0 ../lib/libfmsr.so

../lib/libfmsr.so.0: $(OBJS)
	mkdir -p ../lib
	$(CC) -shared -Wl,-soname,libfmsr.so.0 -o ../lib/libfmsr.so.0 $(OBJS)

.PHONY : all clean docs headers test

