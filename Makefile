all: jerasure libfmsr nccloud

clean:
	make -C Jerasure clean
	make -C libfmsr clean
	make -C nccloud clean

docs:
	make -C libfmsr docs
	make -C nccloud docs

jerasure:
	make -C Jerasure

libfmsr:
	make -C libfmsr

nccloud:
	make -C nccloud

test: libfmsr
	make -C libfmsr test

.PHONY : all clean docs jerasure libfmsr nccloud test

