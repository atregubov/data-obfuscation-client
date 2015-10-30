CXX=g++
CXXFLAGS=-O3 -std=c++0x -Wall -I../libfmsr/include -I../Jerasure/include
LDFLAGS=-O3 -Wall -L../libfmsr/lib -lfmsr -L../Jerasure/lib -lJerasure -lpthread

SRCS=nccloud.cc config.cc fileop.cc coding.cc storage.cc \
     codings/fmsr.cc codings/replication.cc codings/rs.cc \
     storages/local.cc storages/swift.cc
OBJS=$(SRCS:.cc=.o)

LS_SRCS=list_repo.cc config.cc storage.cc storages/local.cc storages/swift.cc
LS_OBJS=$(LS_SRCS:.cc=.o)

all: libfmsr jerasure bin/nccloud bin/list_repo

clean:
	rm -f $(OBJS) $(LS_OBJS) bin/nccloud bin/list_repo

docs:
	doxygen Doxyfile

libfmsr:
	make -C ../libfmsr

jerasure:
	make -C ../Jerasure

bin/nccloud: $(OBJS)
	mkdir -p bin
	$(CXX) -o $@ $(OBJS) $(LDFLAGS)

bin/list_repo: $(LS_OBJS)
	mkdir -p bin
	$(CXX) -o $@ $(LS_OBJS) $(LDFLAGS)

.PHONY : all clean docs libfmsr jerasure

