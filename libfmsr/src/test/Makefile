CC=gcc
CFLAGS=-O3 -Wall -std=c99 -lpthread

SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)
PROGS=$(patsubst %.c,%.test,$(SRCS))

all: libfmsr clean $(PROGS)
	$(foreach p,$(PROGS),./$(p) || exit 1;)

clean:
	rm -f $(OBJS) $(PROGS)

libfmsr:
	make -C ..

%.test: %.o
	$(CC) -o $@ $< ../*.o $(CFLAGS)

.PHONY : all clean libfmsr

