bin_PROGRAMS += mongoc-stat

mongoc_stat_SOURCES = src/tools/mongoc-stat.c
mongoc_stat_CFLAGS = \
	$(LIBC_FEATURES) \
	$(MAINTAINER_CFLAGS) \
	$(OPTIMIZE_CFLAGS) \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/mongoc \
	-I$(top_builddir)/src/mongoc \
	$(BSON_CFLAGS)
mongoc_stat_LDFLAGS = \
	$(OPTIMIZE_LDFLAGS)
mongoc_stat_LDADD = \
	$(BSON_LIBS) \
	$(SHM_LIB)
