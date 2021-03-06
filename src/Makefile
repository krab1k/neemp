CC=icc
CFLAGS=-Wall -Wcheck -Wremarks -std=c99 -g -Ofast -ipo -Xhost -wd981 -qopenmp
# -wd981 disables warning about operands evaluated in an unspecified order
EXTRA_DEFINE=-DUSE_MKL

ICC_DISABLE_WARNS=-wd11021

sources=$(wildcard *.c)
headers=$(wildcard *.h)
objects=$(sources:.c=.o) ../externals/newuoa/*.o ../externals/lhs/*.o
libraries=-mkl -lm -lz -qopenmp -lifcore
binaries=neemp
manpage=neemp.1

all: $(sources) $(headers) neemp

depend: .depend

.depend: $(sources)
	rm -f ./.depend
	$(CC) $(CFLAGS) -MM `xml2-config --cflags` $^ >> ./.depend;

include .depend

neemp-gnu: EXTRA_DEFINE=
neemp-gnu: CC=gcc
neemp-gnu: ICC_DISABLE_WARNS=
neemp-gnu: CFLAGS=-Wall -Wextra -std=c99 -pedantic -O3 -march=native -g -gdwarf-3 -fopenmp
neemp-gnu: libraries=-lm -lz -fopenmp -lgfortran -llapack
neemp-gnu: $(objects) neemp

neemp: $(objects)
	$(CC) $(objects) $(libraries) `xml2-config --cflags --libs` $(ICC_DISABLE_WARNS) -o ../neemp

%.o: %.c
	$(CC) `xml2-config --cflags` $(CFLAGS) $(EXTRA_DEFINE) -c $<

clean:
	rm -f $(objects) $(manpage) $(binaries)
