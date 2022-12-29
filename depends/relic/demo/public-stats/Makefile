CFLAGS=-O3 -march=native -mtune=native -ggdb
RELIC_ROOT = ../..

all: lib data.csv
	gcc ${CFLAGS} -c main.c -o main.o -I ${RELIC_ROOT}/include -I target/include
	gcc ${CFLAGS} -c csv.c -o csv.o -I ${RELIC_ROOT}/include -I target/include
	gcc ${CFLAGS} -o main csv.o main.o target/lib/librelic_s.a -lgmp

lib:
	mkdir -p target
	cd target; ${RELIC_ROOT}/../preset/x64-pbc-bls12-381.sh ${RELIC_ROOT}/../; cmake -DEP_METHD='JACOB;LWNAF;COMBS;INTER' .; make

data.csv: 
	wget -c https://raw.githubusercontent.com/TheEconomist/covid-19-excess-deaths-tracker/master/source-data/spain/archive/spain_total_source_2020_04_13.csv
	mv spain_total_source_2020_04_13.csv data_04_13.csv
	wget -c https://momo.isciii.es/public/momo/data
	mv data data.csv

clean:
	rm -rf target *.o main
