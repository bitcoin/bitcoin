AM_CPPFLAGS =  $(INCLUDES) -I$(top_builddir)/src/obj \
  -I$(top_srcdir)/src/leveldb/include -I$(top_srcdir)/src/leveldb/helpers \
  $(BOOST_INCLUDES)
AM_LDFLAGS = $(PTHREAD_CFLAGS)

noinst_LIBRARIES = libppcoin_server.a libppcoin_common.a libppcoin_cli.a

bin_PROGRAMS = ppcoind peercoin-cli

SUBDIRS = . $(BUILD_QT) $(BUILD_TEST)
DIST_SUBDIRS = . qt test
.PHONY: FORCE
# bitcoin core #
BITCOIN_CORE_H = addrman.h alert.h allocators.h base58.h bignum.h \
  bitcoinrpc.h bloom.h checkpoints.h checkpointsync.h checkqueue.h \
  clientversion.h compat.h crypter.h db.h hash.h init.h \
  key.h kernel.h keystore.h leveldb.h limitedmap.h main.h mruset.h \
  netbase.h net.h protocol.h script.h serialize.h sync.h threadsafety.h \
  txdb.h ui_interface.h uint256.h util.h version.h walletdb.h wallet.h

JSON_H = json/json_spirit.h json/json_spirit_error_position.h \
  json/json_spirit_reader.h json/json_spirit_reader_template.h \
  json/json_spirit_stream_reader.h json/json_spirit_utils.h \
  json/json_spirit_value.h json/json_spirit_writer.h \
  json/json_spirit_writer_template.h

obj/build.h: FORCE
	@$(MKDIR_P) $(abs_top_builddir)/src/obj
	@$(top_srcdir)/share/genbuild.sh $(abs_top_builddir)/src/obj/build.h \
	  $(abs_top_srcdir)
version.o: obj/build.h

libppcoin_server_a_SOURCES = addrman.cpp alert.cpp bloom.cpp \
  checkpoints.cpp checkpointsync.cpp crypter.cpp db.cpp \
  init.cpp kernel.cpp keystore.cpp leveldb.cpp main.cpp \
  net.cpp noui.cpp rpcblockchain.cpp rpcdump.cpp \
  rpcmining.cpp rpcnet.cpp rpcrawtransaction.cpp rpcwallet.cpp \
  txdb.cpp wallet.cpp walletdb.cpp $(JSON_H) \
  $(BITCOIN_CORE_H)

libppcoin_common_a_SOURCES = \
  hash.cpp \
  key.cpp \
  netbase.cpp \
  protocol.cpp \
  script.cpp \
  sync.cpp \
  util.cpp \
  version.cpp \
  $(BITCOIN_CORE_H)

libppcoin_cli_a_SOURCES = \
  rpcclient.cpp \
  $(BITCOIN_CORE_H)

nodist_libppcoin_common_a_SOURCES = $(top_srcdir)/src/obj/build.h
#

# ppcoind binary #
ppcoind_LDADD = libppcoin_server.a libppcoin_cli.a libppcoin_common.a leveldb/libleveldb.a leveldb/libmemenv.a \
  $(BOOST_LIBS)
ppcoind_SOURCES = bitcoind.cpp
#

if TARGET_WINDOWS
ppcoind_SOURCES += bitcoind-res.rc
endif

AM_CPPFLAGS += $(BDB_CPPFLAGS)
ppcoind_LDADD += $(BDB_LIBS)

# bitcoin-cli binary #
peercoin_cli_LDADD = libppcoin_cli.a libppcoin_common.a $(BOOST_LIBS)
peercoin_cli_SOURCES = bitcoin-cli.cpp
#

if TARGET_WINDOWS
peercoin_cli_SOURCES += bitcoin-cli-res.rc
endif

leveldb/libleveldb.a: leveldb/libmemenv.a

leveldb/%.a:
	@echo "Building LevelDB ..." && $(MAKE) -C $(@D) $(@F) CXX="$(CXX)" \
	  CC="$(CC)" PLATFORM=$(TARGET_OS) AR="$(AR)" $(LEVELDB_TARGET_FLAGS) \
	  OPT="$(CXXFLAGS) $(CPPFLAGS)"

qt/bitcoinstrings.cpp: $(libppcoin_server_a_SOURCES) $(libppcoin_common_a_SOURCES) $(libppcoin_cli_a_SOURCES)
	@test -n $(XGETTEXT) || echo "xgettext is required for updating translations"
	@cd $(top_srcdir); XGETTEXT=$(XGETTEXT) share/qt/extract_strings_qt.py

CLEANFILES = leveldb/libleveldb.a leveldb/libmemenv.a *.gcda *.gcno

DISTCLEANFILES = obj/build.h

EXTRA_DIST = leveldb Makefile.include

clean-local:
	-$(MAKE) -C leveldb clean
	rm -f leveldb/port/*.gcno leveldb/db/*.gcno leveldb/table/*.gcno leveldb/helpers/*.gcno
	rm -f leveldb/util/*.gcno leveldb/helpers/memenv/*.gcno
