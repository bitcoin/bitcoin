EXAMPLE_LDADD = libmongoc-1.0.la
if EXPLICIT_LIBS
EXAMPLE_LDADD += $(BSON_LIBS)
endif
EXAMPLE_CFLAGS = \
	-I$(top_srcdir)/src/mongoc \
	-I$(top_builddir)/src/mongoc \
	$(BSON_CFLAGS)

noinst_PROGRAMS += example-gridfs
example_gridfs_SOURCES = examples/example-gridfs.c
example_gridfs_CFLAGS = $(EXAMPLE_CFLAGS)
example_gridfs_LDADD = $(EXAMPLE_LDADD)

noinst_PROGRAMS += mongoc-dump
mongoc_dump_SOURCES = examples/mongoc-dump.c
mongoc_dump_CFLAGS = $(EXAMPLE_CFLAGS)
mongoc_dump_LDADD = $(EXAMPLE_LDADD)

noinst_PROGRAMS += filter-bsondump
filter_bsondump_SOURCES = examples/filter-bsondump.c
filter_bsondump_CFLAGS = -Wno-deprecated-declarations $(EXAMPLE_CFLAGS)
filter_bsondump_LDADD = $(EXAMPLE_LDADD)

noinst_PROGRAMS += example-command-monitoring
example_command_monitoring_SOURCES = examples/example-command-monitoring.c
example_command_monitoring_CFLAGS = $(EXAMPLE_CFLAGS)
example_command_monitoring_LDADD = $(EXAMPLE_LDADD)

noinst_PROGRAMS += example-sdam-monitoring
example_sdam_monitoring_SOURCES = examples/example-sdam-monitoring.c
example_sdam_monitoring_CFLAGS = $(EXAMPLE_CFLAGS)
example_sdam_monitoring_LDADD = $(EXAMPLE_LDADD)

noinst_PROGRAMS += example-client
example_client_SOURCES = examples/example-client.c
example_client_CFLAGS = $(EXAMPLE_CFLAGS)
example_client_LDADD = $(EXAMPLE_LDADD)

noinst_PROGRAMS += example-create-indexes
example_create_indexes_SOURCES = examples/example-create-indexes.c
example_create_indexes_CFLAGS = $(EXAMPLE_CFLAGS)
example_create_indexes_LDADD = $(EXAMPLE_LDADD)

EXAMPLE_POOL_LDADD =
EXAMPLE_POOL_CFLAGS =
if EXPLICIT_LIBS
EXAMPLE_POOL_LDADD += $(PTHREAD_LIBS)
EXAMPLE_POOL_CFLAGS += $(PTHREAD_CFLAGS)
endif

noinst_PROGRAMS += example-pool
example_pool_SOURCES = examples/example-pool.c
example_pool_CFLAGS = $(EXAMPLE_CFLAGS) $(EXAMPLE_POOL_CFLAGS)
example_pool_LDADD = $(EXAMPLE_LDADD) $(EXAMPLE_POOL_LDADD)

noinst_PROGRAMS += example-command-with-opts
example_command_with_opts_SOURCES = examples/example-command-with-opts.c
example_command_with_opts_CFLAGS = $(EXAMPLE_CFLAGS)
example_command_with_opts_LDADD = $(EXAMPLE_LDADD)

noinst_PROGRAMS += example-scram
example_scram_SOURCES = examples/example-scram.c
example_scram_CFLAGS = $(EXAMPLE_CFLAGS)
example_scram_LDADD = $(EXAMPLE_LDADD)

noinst_PROGRAMS += mongoc-ping
mongoc_ping_SOURCES = examples/mongoc-ping.c
mongoc_ping_CFLAGS = $(EXAMPLE_CFLAGS)
mongoc_ping_LDADD = $(EXAMPLE_LDADD)

noinst_PROGRAMS += mongoc-tail
mongoc_tail_SOURCES = examples/mongoc-tail.c
mongoc_tail_CFLAGS = $(EXAMPLE_CFLAGS)
mongoc_tail_LDADD = $(EXAMPLE_LDADD)

noinst_PROGRAMS += find-and-modify
find_and_modify_SOURCES = examples/find-and-modify.c
find_and_modify_CFLAGS = $(EXAMPLE_CFLAGS)
find_and_modify_LDADD = $(EXAMPLE_LDADD)

noinst_PROGRAMS += example-matcher
example_matcher_SOURCES = examples/example-matcher.c
example_matcher_CFLAGS = -Wno-deprecated-declarations $(EXAMPLE_CFLAGS)
example_matcher_LDADD = $(EXAMPLE_LDADD)

noinst_PROGRAMS += bulk1
bulk1_SOURCES = examples/bulk/bulk1.c
bulk1_CFLAGS = $(EXAMPLE_CFLAGS)
bulk1_LDADD = $(EXAMPLE_LDADD)

noinst_PROGRAMS += bulk2
bulk2_SOURCES = examples/bulk/bulk2.c
bulk2_CFLAGS = $(EXAMPLE_CFLAGS)
bulk2_LDADD = $(EXAMPLE_LDADD)

noinst_PROGRAMS += bulk3
bulk3_SOURCES = examples/bulk/bulk3.c
bulk3_CFLAGS = $(EXAMPLE_CFLAGS)
bulk3_LDADD = $(EXAMPLE_LDADD)

noinst_PROGRAMS += bulk4
bulk4_SOURCES = examples/bulk/bulk4.c
bulk4_CFLAGS = $(EXAMPLE_CFLAGS)
bulk4_LDADD = $(EXAMPLE_LDADD)

noinst_PROGRAMS += bulk5
bulk5_SOURCES = examples/bulk/bulk5.c
bulk5_CFLAGS = $(EXAMPLE_CFLAGS)
bulk5_LDADD = $(EXAMPLE_LDADD)

noinst_PROGRAMS += bulk6
bulk6_SOURCES = examples/bulk/bulk6.c
bulk6_CFLAGS = $(EXAMPLE_CFLAGS)
bulk6_LDADD = $(EXAMPLE_LDADD)

noinst_PROGRAMS += bulk-collation
bulk_collation_SOURCES = examples/bulk/bulk-collation.c
bulk_collation_CFLAGS = $(EXAMPLE_CFLAGS)
bulk_collation_LDADD = $(EXAMPLE_LDADD)

noinst_PROGRAMS += aggregation1
aggregation1_SOURCES = examples/aggregation/aggregation1.c
aggregation1_CFLAGS = $(EXAMPLE_CFLAGS)
aggregation1_LDADD = $(EXAMPLE_LDADD)

noinst_PROGRAMS += fam
fam_SOURCES = examples/find_and_modify_with_opts/fam.c
fam_CFLAGS = $(EXAMPLE_CFLAGS)
fam_LDADD = $(EXAMPLE_LDADD)

EXTRA_DIST += \
	examples/doc-common-insert.c \
	examples/basic_aggregation/map-reduce-basic.c \
	examples/basic_aggregation/map-reduce-advanced.c \
	examples/basic_aggregation/distinct.c \
	examples/basic_aggregation/basic-aggregation.c \
	examples/basic_aggregation/constants.c \
	examples/common_operations/explain.c \
	examples/common_operations/copydb.c \
	examples/common_operations/common-operations.c \
	examples/common_operations/clone-collection.c \
	examples/find_and_modify_with_opts/bypass.c \
	examples/find_and_modify_with_opts/fields.c \
	examples/find_and_modify_with_opts/flags.c \
	examples/find_and_modify_with_opts/opts.c \
	examples/find_and_modify_with_opts/sort.c \
	examples/find_and_modify_with_opts/update.c

# These files are included when we generate docs from tutorial.rst
EXTRA_DIST += \
	examples/hello_mongoc.c \
	examples/compile-with-pkg-config.sh \
	examples/compile-with-pkg-config-static.sh \
	examples/cmake/find_package/CMakeLists.txt \
	examples/cmake/find_package_static/CMakeLists.txt
