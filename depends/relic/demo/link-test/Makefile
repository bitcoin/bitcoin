RELIC_ROOT = ../../..

all: type1 type3
	gcc -c test1.c -o test1.o -I /usr/local/include/relic_type1
	gcc -c test2.c -o test2.o -I /usr/local/include/relic_type3
	gcc -o test test.c test1.o test2.o /usr/local/lib/librelic_s_*.a -lgmp

type1:
	mkdir -p target-type1
	cd target-type1; cmake -DLABEL=type1 ${RELIC_ROOT}; ${RELIC_ROOT}/preset/gmp-pbc-ss1536.sh ${RELIC_ROOT}; make && make install

type3:
	mkdir -p target-type3
	cd target-type3; cmake -DLABEL=type3 ${RELIC_ROOT}; ${RELIC_ROOT}/preset/x64-pbc-bls12-381.sh ${RELIC_ROOT}; make && make install

clean:
	rm -rf target-* *.o test
