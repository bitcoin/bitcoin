noinst_LTLIBRARIES += libmongoc.la

headerdir = $(prefix)/include/libmongoc-@MONGOC_API_VERSION@
header_DATA = \
	$(INST_H_FILES) \
	$(MONGOC_DEF_FILES) \
	src/mongoc/mongoc-config.h

MONGOC_DEF_FILES = \
	src/mongoc/op-delete.def \
	src/mongoc/op-get-more.def \
	src/mongoc/op-header.def \
	src/mongoc/op-insert.def \
	src/mongoc/op-kill-cursors.def \
	src/mongoc/op-msg.def \
	src/mongoc/op-query.def \
	src/mongoc/op-reply.def \
	src/mongoc/op-reply-header.def \
	src/mongoc/op-update.def \
	src/mongoc/op-compressed.def \
	src/mongoc/mongoc-counters.defs

INST_H_FILES = \
	src/mongoc/mongoc-apm.h \
	src/mongoc/mongoc-bulk-operation.h \
	src/mongoc/mongoc-client.h \
	src/mongoc/mongoc-client-pool.h \
	src/mongoc/mongoc-collection.h \
	src/mongoc/mongoc-cursor.h \
	src/mongoc/mongoc-database.h \
	src/mongoc/mongoc-error.h \
	src/mongoc/mongoc-find-and-modify.h \
	src/mongoc/mongoc-flags.h \
	src/mongoc/mongoc-gridfs-file.h \
	src/mongoc/mongoc-gridfs-file-list.h \
	src/mongoc/mongoc-gridfs-file-page.h \
	src/mongoc/mongoc-gridfs.h \
	src/mongoc/mongoc.h \
	src/mongoc/mongoc-handshake.h \
	src/mongoc/mongoc-host-list.h \
	src/mongoc/mongoc-index.h \
	src/mongoc/mongoc-init.h \
	src/mongoc/mongoc-iovec.h \
	src/mongoc/mongoc-log.h \
	src/mongoc/mongoc-macros.h \
	src/mongoc/mongoc-matcher.h \
	src/mongoc/mongoc-opcode.h \
	src/mongoc/mongoc-rand.h \
	src/mongoc/mongoc-read-concern.h \
	src/mongoc/mongoc-read-prefs.h \
	src/mongoc/mongoc-server-description.h \
	src/mongoc/mongoc-socket.h \
	src/mongoc/mongoc-ssl.h \
	src/mongoc/mongoc-stream-buffered.h \
	src/mongoc/mongoc-stream-file.h \
	src/mongoc/mongoc-stream-gridfs.h \
	src/mongoc/mongoc-stream.h \
	src/mongoc/mongoc-stream-socket.h \
	src/mongoc/mongoc-stream-tls.h \
	src/mongoc/mongoc-stream-tls-libressl.h \
	src/mongoc/mongoc-stream-tls-openssl.h \
	src/mongoc/mongoc-stream-tls-secure-channel.h \
	src/mongoc/mongoc-stream-tls-secure-transport.h \
	src/mongoc/mongoc-topology-description.h \
	src/mongoc/mongoc-uri.h \
	src/mongoc/mongoc-version-functions.h \
	src/mongoc/mongoc-version.h \
	src/mongoc/mongoc-write-concern.h \
	src/mongoc/utlist.h

NOINST_H_FILES = \
	src/mongoc/mongoc-apm-private.h \
	src/mongoc/mongoc-array-private.h \
	src/mongoc/mongoc-async-cmd-private.h \
	src/mongoc/mongoc-async-private.h \
	src/mongoc/mongoc-b64-private.h \
	src/mongoc/mongoc-buffer-private.h \
	src/mongoc/mongoc-bulk-operation-private.h \
	src/mongoc/mongoc-client-pool-private.h \
	src/mongoc/mongoc-client-private.h \
	src/mongoc/mongoc-cluster-cyrus-private.h \
	src/mongoc/mongoc-cluster-private.h \
	src/mongoc/mongoc-cluster-sasl-private.h \
	src/mongoc/mongoc-cluster-sspi-private.h \
	src/mongoc/mongoc-cluster-sspi-private.h \
	src/mongoc/mongoc-cmd-private.h \
	src/mongoc/mongoc-collection-private.h \
	src/mongoc/mongoc-compression-private.h \
	src/mongoc/mongoc-counters-private.h \
	src/mongoc/mongoc-crypto-cng-private.h \
	src/mongoc/mongoc-crypto-common-crypto-private.h \
	src/mongoc/mongoc-crypto-openssl-private.h \
	src/mongoc/mongoc-crypto-private.h \
	src/mongoc/mongoc-cursor-array-private.h \
	src/mongoc/mongoc-cursor-cursorid-private.h \
	src/mongoc/mongoc-cursor-private.h \
	src/mongoc/mongoc-cursor-transform-private.h \
	src/mongoc/mongoc-cyrus-private.h \
	src/mongoc/mongoc-database-private.h \
	src/mongoc/mongoc-errno-private.h \
	src/mongoc/mongoc-find-and-modify-private.h \
	src/mongoc/mongoc-gridfs-file-list-private.h \
	src/mongoc/mongoc-gridfs-file-page-private.h \
	src/mongoc/mongoc-gridfs-file-private.h \
	src/mongoc/mongoc-gridfs-private.h \
	src/mongoc/mongoc-handshake-compiler-private.h \
	src/mongoc/mongoc-handshake-os-private.h \
	src/mongoc/mongoc-handshake-private.h \
	src/mongoc/mongoc-host-list-private.h \
	src/mongoc/mongoc-libressl-private.h \
	src/mongoc/mongoc-linux-distro-scanner-private.h \
	src/mongoc/mongoc-list-private.h \
	src/mongoc/mongoc-log-private.h \
	src/mongoc/mongoc-matcher-op-private.h \
	src/mongoc/mongoc-matcher-private.h \
	src/mongoc/mongoc-memcmp-private.h \
	src/mongoc/mongoc-openssl-private.h \
	src/mongoc/mongoc-queue-private.h \
	src/mongoc/mongoc-rand-private.h \
	src/mongoc/mongoc-read-concern-private.h \
	src/mongoc/mongoc-read-prefs-private.h \
	src/mongoc/mongoc-rpc-private.h \
	src/mongoc/mongoc-sasl-private.h \
	src/mongoc/mongoc-scram-private.h \
	src/mongoc/mongoc-secure-channel-private.h \
	src/mongoc/mongoc-secure-transport-private.h \
	src/mongoc/mongoc-server-description-private.h \
	src/mongoc/mongoc-server-stream-private.h \
	src/mongoc/mongoc-set-private.h \
	src/mongoc/mongoc-socket-private.h \
	src/mongoc/mongoc-ssl-private.h \
	src/mongoc/mongoc-sspi-private.h \
	src/mongoc/mongoc-stream-private.h \
	src/mongoc/mongoc-stream-tls-libressl-private.h \
	src/mongoc/mongoc-stream-tls-openssl-bio-private.h \
	src/mongoc/mongoc-stream-tls-openssl-private.h \
	src/mongoc/mongoc-stream-tls-private.h \
	src/mongoc/mongoc-stream-tls-secure-channel-private.h \
	src/mongoc/mongoc-stream-tls-secure-transport-private.h \
	src/mongoc/mongoc-thread-private.h \
	src/mongoc/mongoc-topology-description-apm-private.h \
	src/mongoc/mongoc-topology-description-private.h \
	src/mongoc/mongoc-topology-private.h \
	src/mongoc/mongoc-topology-scanner-private.h \
	src/mongoc/mongoc-trace-private.h \
	src/mongoc/mongoc-uri-private.h \
	src/mongoc/mongoc-util-private.h \
	src/mongoc/mongoc-write-command-private.h \
	src/mongoc/mongoc-write-concern-private.h

libmongoc_la_SOURCES = \
	$(INST_H_FILES) \
	src/mongoc/mongoc-apm.c \
	src/mongoc/mongoc-array.c \
	src/mongoc/mongoc-async.c \
	src/mongoc/mongoc-async-cmd.c \
	src/mongoc/mongoc-buffer.c \
	src/mongoc/mongoc-bulk-operation.c \
	src/mongoc/mongoc-b64.c \
	src/mongoc/mongoc-client.c \
	src/mongoc/mongoc-client-pool.c \
	src/mongoc/mongoc-cluster.c \
	src/mongoc/mongoc-collection.c \
	src/mongoc/mongoc-compression.c \
	src/mongoc/mongoc-counters.c \
	src/mongoc/mongoc-cursor.c \
	src/mongoc/mongoc-cursor-array.c \
	src/mongoc/mongoc-cursor-cursorid.c \
	src/mongoc/mongoc-cursor-transform.c \
	src/mongoc/mongoc-database.c \
	src/mongoc/mongoc-find-and-modify.c \
	src/mongoc/mongoc-host-list.c \
	src/mongoc/mongoc-init.c \
	src/mongoc/mongoc-gridfs.c \
	src/mongoc/mongoc-gridfs-file.c \
	src/mongoc/mongoc-gridfs-file-page.c \
	src/mongoc/mongoc-gridfs-file-list.c \
	src/mongoc/mongoc-handshake.c \
	src/mongoc/mongoc-index.c \
	src/mongoc/mongoc-linux-distro-scanner.c \
	src/mongoc/mongoc-list.c \
	src/mongoc/mongoc-log.c \
	src/mongoc/mongoc-matcher-op.c \
	src/mongoc/mongoc-matcher.c \
	src/mongoc/mongoc-memcmp.c \
	src/mongoc/mongoc-cmd.c \
	src/mongoc/mongoc-queue.c \
	src/mongoc/mongoc-read-concern.c \
	src/mongoc/mongoc-read-prefs.c \
	src/mongoc/mongoc-rpc.c \
	src/mongoc/mongoc-server-description.c \
	src/mongoc/mongoc-server-stream.c \
	src/mongoc/mongoc-set.c \
	src/mongoc/mongoc-socket.c \
	src/mongoc/mongoc-stream.c \
	src/mongoc/mongoc-stream-buffered.c \
	src/mongoc/mongoc-stream-file.c \
	src/mongoc/mongoc-stream-gridfs.c \
	src/mongoc/mongoc-stream-socket.c \
	src/mongoc/mongoc-topology.c \
	src/mongoc/mongoc-topology-description.c \
	src/mongoc/mongoc-topology-description-apm.c \
	src/mongoc/mongoc-topology-scanner.c \
	src/mongoc/mongoc-uri.c \
	src/mongoc/mongoc-util.c \
	src/mongoc/mongoc-version-functions.c \
	src/mongoc/mongoc-write-command.c \
	src/mongoc/mongoc-write-concern.c

if ENABLE_CRYPTO
libmongoc_la_SOURCES += \
	src/mongoc/mongoc-crypto.c \
	src/mongoc/mongoc-scram.c

if ENABLE_CRYPTO_LIBCRYPTO
libmongoc_la_SOURCES += \
	src/mongoc/mongoc-crypto-openssl.c \
	src/mongoc/mongoc-rand-openssl.c
endif
if ENABLE_CRYPTO_COMMON_CRYPTO
libmongoc_la_SOURCES += \
	src/mongoc/mongoc-crypto-common-crypto.c \
	src/mongoc/mongoc-rand-common-crypto.c
endif
if ENABLE_CRYPTO_CNG
libmongoc_la_SOURCES += \
	src/mongoc/mongoc-crypto-cng.c \
	src/mongoc/mongoc-rand-cng.c
endif
endif

if ENABLE_SSL
libmongoc_la_SOURCES += \
	src/mongoc/mongoc-stream-tls.c \
	src/mongoc/mongoc-ssl.c

if ENABLE_SSL_LIBRESSL
libmongoc_la_SOURCES += \
	src/mongoc/mongoc-libressl.c \
	src/mongoc/mongoc-stream-tls-libressl.c
endif
if ENABLE_SSL_OPENSSL
libmongoc_la_SOURCES += \
	src/mongoc/mongoc-openssl.c \
	src/mongoc/mongoc-stream-tls-openssl.c \
	src/mongoc/mongoc-stream-tls-openssl-bio.c
endif
if ENABLE_SSL_SECURE_TRANSPORT
libmongoc_la_SOURCES += \
	src/mongoc/mongoc-secure-transport.c \
	src/mongoc/mongoc-stream-tls-secure-transport.c
endif
if ENABLE_SSL_SECURE_CHANNEL
libmongoc_la_SOURCES += \
	src/mongoc/mongoc-secure-channel.c \
	src/mongoc/mongoc-stream-tls-secure-channel.c
endif
endif

if ENABLE_SASL
libmongoc_la_SOURCES += src/mongoc/mongoc-cluster-sasl.c
libmongoc_la_SOURCES += src/mongoc/mongoc-sasl.c
endif
if ENABLE_SASL_CYRUS
libmongoc_la_SOURCES += src/mongoc/mongoc-cluster-cyrus.c
libmongoc_la_SOURCES += src/mongoc/mongoc-cyrus.c
endif

if ENABLE_SASL_SSPI
libmongoc_la_SOURCES += src/mongoc/mongoc-cluster-sspi.c
libmongoc_la_SOURCES += src/mongoc/mongoc-sspi.c
endif

if ENABLE_SASL_GSSAPI
libmongoc_la_SOURCES += src/mongoc/mongoc-cluster-gssapi.c
libmongoc_la_SOURCES += src/mongoc/mongoc-gssapi.c
endif

libmongoc_la_CPPFLAGS = \
	-DMONGOC_COMPILATION \
	$(LIBC_FEATURES) \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/mongoc \
	-I$(top_builddir)/src/mongoc \
	$(BSON_CFLAGS)

libmongoc_la_CFLAGS = \
	$(MAINTAINER_CFLAGS) \
	$(OPTIMIZE_CFLAGS) \
	$(COVERAGE_CFLAGS) \
	$(PTHREAD_CFLAGS) \
	$(SSL_CFLAGS) \
	$(SNAPPY_CFLAGS) \
	$(ZLIB_CFLAGS) \
	$(SASL_CFLAGS)

if OS_SOLARIS
libmongoc_la_CFLAGS += -D_REENTRANT
endif

libmongoc_la_LIBADD = \
	$(BSON_LIBS) \
	$(PTHREAD_LIBS) \
	$(SHM_LIB) \
	$(SSL_LIBS) \
	$(SNAPPY_LIBS) \
	$(ZLIB_LIBS) \
	$(SASL_LIBS)

if OS_WIN32
libmongoc_la_LIBADD += -lws2_32
endif

if WITH_SNAPPY
libmongoc_la_LIBADD += libsnappy.la
lib_LTLIBRARIES += libsnappy.la
libsnappy_la_CPPFLAGS = $(BSON_CFLAGS)
libsnappy_la_SOURCES = \
                        src/snappy-1.1.3/snappy.cc \
                        src/snappy-1.1.3/snappy-sinksource.cc \
                        src/snappy-1.1.3/snappy-stubs-internal.cc \
                        src/snappy-1.1.3/snappy-c.cc

noinst_HEADERS += \
                  src/snappy-1.1.3/snappy-internal.h \
                  src/snappy-1.1.3/snappy-stubs-internal.h \
                  src/snappy-1.1.3/snappy-test.h \
                  src/snappy-1.1.3/snappy.h \
                  src/snappy-1.1.3/snappy-sinksource.h \
                  src/snappy-1.1.3/snappy-stubs-public.h \
                  src/snappy-1.1.3/snappy-c.h

dist_doc_DATA = src/snappy-1.1.3/COPYING
endif

if WITH_ZLIB
libmongoc_la_LIBADD += libzlib.la
noinst_LTLIBRARIES += libzlib.la
libzlib_la_CFLAGS  =
libzlib_la_SOURCES = \
                        src/zlib-1.2.11/adler32.c \
                        src/zlib-1.2.11/crc32.c \
                        src/zlib-1.2.11/deflate.c \
                        src/zlib-1.2.11/infback.c \
                        src/zlib-1.2.11/inffast.c \
                        src/zlib-1.2.11/inflate.c \
                        src/zlib-1.2.11/inftrees.c \
                        src/zlib-1.2.11/trees.c \
                        src/zlib-1.2.11/zutil.c \
                        src/zlib-1.2.11/compress.c \
                        src/zlib-1.2.11/uncompr.c \
                        src/zlib-1.2.11/gzclose.c \
                        src/zlib-1.2.11/gzlib.c \
                        src/zlib-1.2.11/gzread.c \
                        src/zlib-1.2.11/gzwrite.c

noinst_HEADERS += \
                  src/zlib-1.2.11/crc32.h \
                  src/zlib-1.2.11/deflate.h \
                  src/zlib-1.2.11/gzguts.h \
                  src/zlib-1.2.11/inffast.h \
                  src/zlib-1.2.11/inffixed.h \
                  src/zlib-1.2.11/inflate.h \
                  src/zlib-1.2.11/inftrees.h \
                  src/zlib-1.2.11/trees.h \
                  src/zlib-1.2.11/zconf.h \
                  src/zlib-1.2.11/zlib.h \
                  src/zlib-1.2.11/zutil.h
endif

EXTRA_DIST += $(MONGOC_DEF_FILES) $(NOINST_H_FILES)

