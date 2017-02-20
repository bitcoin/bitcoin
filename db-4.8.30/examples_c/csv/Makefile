# $Id$

# Berkeley DB installation.
DB_INCLUDE=../../build_unix
LIBS=	-L../../build_unix -L../../build_unix/.libs/ -ldb

INC=	-I. -I$(DB_INCLUDE)
CFLAGS=	$(INC) -g -W -Wall -Wpointer-arith -Wmissing-prototypes

PROGS=	csv_code csv_load csv_query
SRCS=	DbRecord.c code.c csv_local.c db.c load.c load_main.c query.c \
	query_main.c util.c

all: csv_load csv_query

csv_code: code.o
	$(CC) -o $@ $? $(LIBS)

LOAD_OBJS=DbRecord.o csv_local.o db.o load.o load_main.o util.o
csv_load: $(LOAD_OBJS)
	$(CC) -o $@ $(LOAD_OBJS) $(LIBS)

QUERY_OBJS=DbRecord.o csv_local.o db.o query.o query_main.o util.o
csv_query: $(QUERY_OBJS)
	$(CC) -o $@ $(QUERY_OBJS) $(LIBS)

clean distclean realclean:
	rm -rf $(PROGS) TESTDIR eBay tags *.o *.core csv_local.[ch]

tags:
	rm -f tags
	ctags $(SRCS) code.c

DbRecord.o csv_local.o db.o load.o load_main.o query.o: csv_local.h csv.h
query_main.o util.o: csv_local.h csv.h

csv_local.c csv_local.h: csv_code
	./csv_code -c csv_local.c -h csv_local.h -f sample.desc

lint_code:
	flexelint +fll \
	    "-e801" \
	    "-e818" \
	    "-esym(534,fprintf)" \
	    "-esym(534,memcpy)" \
	    "-esym(534,memmove)" \
	    "-esym(534,memset)" \
	    "-esym(534,printf)" \
	    "-wlib(1)" \
	    -i$(DB_INCLUDE) "-i/usr/include" \
	    code.c
lint_load:
	flexelint +fll \
	    "-e801" \
	    "-e818" \
	    "-esym(534,fprintf)" \
	    "-esym(534,memcpy)" \
	    "-esym(534,memmove)" \
	    "-esym(534,memset)" \
	    "-esym(534,printf)" \
	    "-wlib(1)" \
	    -i$(DB_INCLUDE)  "-i/usr/include" \
	    DbRecord.c csv_local.c db.c load.c load_main.c util.c
lint_query:
	flexelint +fll \
	    "-e801" \
	    "-e818" \
	    "-esym(534,fprintf)" \
	    "-esym(534,memcpy)" \
	    "-esym(534,memmove)" \
	    "-esym(534,memset)" \
	    "-esym(534,printf)" \
	    "-wlib(1)" \
	    -i$(DB_INCLUDE)  "-i/usr/include" \
	    DbRecord.c csv_local.c db.c query.c query_main.c util.c
