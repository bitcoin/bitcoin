CFLAGS=-O3 -march=native -mtune=native -ggdb
RELIC_ROOT = ../..

all: lib
	gcc ${CFLAGS} -c test.c -o test.o -I ${RELIC_ROOT}/include -I target-gmp/include
	gcc ${CFLAGS} -o test test.o target-gmp/lib/librelic_s.a -lgmp

lib:
	mkdir -p target-gmp
	cd target-gmp; ${RELIC_ROOT}/../preset/gmp-paillier-4096.sh ${RELIC_ROOT}/../; make

clean:
	rm -rf target-gmp *.o test
