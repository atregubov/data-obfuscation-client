CXX=g++
CXXFLAGS=-O3 -std=c++0x -Wall
LDFLAGS=-O3 -Wall

SRCS=src/sclient.cpp
OBJS=$(SRCS:.ccp=.o)

all: bin/sclient

clean:
	rm -f bin/sclient

bin/sclient: $(OBJS)
	mkdir -p bin
	$(CXX) -o $@ $(OBJS) $(LDFLAGS)


