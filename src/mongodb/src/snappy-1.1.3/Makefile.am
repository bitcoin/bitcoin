noinst_LTLIBRARIES += libsnappy.la

libsnappy_la_SOURCES = \
       src/snappy-1.1.3/snappy.cc \
       src/snappy-1.1.3/snappy-sinksource.cc \
       src/snappy-1.1.3/snappy-stubs-internal.cc \
       src/snappy-1.1.3/snappy-c.cc

libsnappy_la_CPPFLAGS = $(BSON_CFLAGS)

noinst_HEADERS += \
 src/snappy-1.1.3/snappy-internal.h \
 src/snappy-1.1.3/snappy-stubs-internal.h \
 src/snappy-1.1.3/snappy-test.h \
 src/snappy-1.1.3/snappy.h \
 src/snappy-1.1.3/snappy-sinksource.h \
 src/snappy-1.1.3/snappy-stubs-public.h \
 src/snappy-1.1.3/snappy-c.h

dist_doc_DATA = COPYING
