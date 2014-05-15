GXX=gcc -g -lpthread -lrt

all: sieve

sieve: sieve.o mytimer.o
	$(GXX) sieve.o mytimer.o -o sieve

sieve.o:sieve.c
	$(GXX) -c sieve.c -o sieve.o

mytimer.o:mytimer.c
	$(GXX) -c mytimer.c -o mytimer.o

clean:
	rm -f *.o *~ sieve *.txt
	rm -rf *.o sieve
