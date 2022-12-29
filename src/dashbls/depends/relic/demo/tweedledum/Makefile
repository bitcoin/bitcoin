CFLAGS=-O3 -march=native -mtune=native -ggdb
RELIC_ROOT = ../..

all: lib 
	gcc ${CFLAGS} -c main.c -o main.o -I ${RELIC_ROOT}/include -I target/include
	gcc ${CFLAGS} -o main main.o target/lib/librelic_s.a -lgmp

lib:
	mkdir -p target
	cd target; ${RELIC_ROOT}/../preset/gmp-ecc-tweedledum.sh ${RELIC_ROOT}/../; cmake -DEP_METHD='JACOB;LWNAF;COMBS;INTER' .; make

clean:
	rm -rf target *.o main
