all: libfmsr

clean:
	make -C src clean

docs:
	make -C src docs

libfmsr:
	make -C src

test: libfmsr
	make -C src test

.PHONY : all clean docs libfmsr test

