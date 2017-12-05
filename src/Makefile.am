AM_CPPFLAGS =  $(INCLUDES) \
  -I$(top_builddir)/src/obj \
  $(BDB_CPPFLAGS) \
  $(BOOST_CPPFLAGS) $(BOOST_INCLUDES)
AM_CPPFLAGS += $(LEVELDB_CPPFLAGS)
AM_LDFLAGS = $(PTHREAD_CFLAGS)

AM_CPPFLAGS += -I$(top_srcdir)/src/leveldb/helpers/memenv \
  -I$(builddir)

AM_CPPFLAGS += -I$(builddir)

if EMBEDDED_LEVELDB
LEVELDB_CPPFLAGS += -I$(top_srcdir)/src/leveldb/include
LEVELDB_CPPFLAGS += -I$(top_srcdir)/src/leveldb/helpers/memenv
LIBLEVELDB += $(top_builddir)/src/leveldb/libleveldb.a
LIBMEMENV  += $(top_builddir)/src/leveldb/libmemenv.a

# NOTE: This dependency is not strictly necessary, but without it make may try to build both in parallel, which breaks the LevelDB build system in a race
$(LIBLEVELDB): $(LIBMEMENV)

$(LIBLEVELDB) $(LIBMEMENV):
	@echo "Building LevelDB ..." && $(MAKE) -C $(@D) $(@F) CXX="$(CXX)" \
	  CC="$(CC)" PLATFORM=$(TARGET_OS) AR="$(AR)" $(LEVELDB_TARGET_FLAGS) \
          OPT="$(CXXFLAGS) $(CPPFLAGS)"
endif

noinst_LIBRARIES = \
  libpeercoin_server.a \
  libpeercoin_common.a \
  libpeercoin_cli.a
if ENABLE_WALLET
noinst_LIBRARIES += libpeercoin_wallet.a
endif

bin_PROGRAMS =
TESTS =

if BUILD_BITCOIND
  bin_PROGRAMS += peercoind
endif

if BUILD_BITCOIN_CLI
  bin_PROGRAMS += peercoin-cli
endif

.PHONY: FORCE
# bitcoin core #
BITCOIN_CORE_H = \
  addrman.h \
  alert.h \
  allocators.h \
  base58.h \
  bignum.h \
  rpcclient.h \
  rpcprotocol.h \
  rpcserver.h \
  bloom.h \
  checkpoints.h \
  checkpointsync.h \
  checkqueue.h \
  clientversion.h \
  compat.h \
  crypter.h \
  db.h \
  hash.h \
  init.h \
  key.h \
  kernel.h \
  keystore.h \
  leveldbwrapper.h \
  limitedmap.h \
  main.h \
  mruset.h \
  netbase.h \
  net.h \
  protocol.h \
  script.h \
  serialize.h \
  sync.h \
  threadsafety.h \
  txdb.h \
  ui_interface.h \
  uint256.h \
  util.h \
  version.h \
  walletdb.h \
  wallet.h

JSON_H = \
  json/json_spirit.h \
  json/json_spirit_error_position.h \
  json/json_spirit_reader.h \
  json/json_spirit_reader_template.h \
  json/json_spirit_stream_reader.h \
  json/json_spirit_utils.h \
  json/json_spirit_value.h \
  json/json_spirit_writer.h \
  json/json_spirit_writer_template.h 

obj/build.h: FORCE
	@$(MKDIR_P) $(abs_top_builddir)/src/obj
	@$(top_srcdir)/share/genbuild.sh $(abs_top_builddir)/src/obj/build.h \
	  $(abs_top_srcdir)
version.o: obj/build.h

libpeercoin_server_a_SOURCES = \
  addrman.cpp \
  alert.cpp \
  rpcserver.cpp \
  bloom.cpp \
  checkpoints.cpp \
  checkpointsync.cpp \
  crypter.cpp \
  init.cpp \
  kernel.cpp \
  keystore.cpp \
  leveldbwrapper.cpp \
  main.cpp \
  net.cpp \
  noui.cpp \
  rpcblockchain.cpp \
  rpcnet.cpp \
  rpcrawtransaction.cpp \
  rpcserver.cpp \
  txdb.cpp \
  $(JSON_H) \
  $(BITCOIN_CORE_H)

libpeercoin_wallet_a_SOURCES = \
  db.cpp \
  rpcdump.cpp \
  rpcmining.cpp \
  rpcminting.cpp \
  script.cpp \
  kernelrecord.cpp \
  wallet.cpp \
  walletdb.cpp \
  rpcwallet.cpp \
  $(BITCOIN_CORE_H)

libpeercoin_common_a_SOURCES = \
  hash.cpp \
  key.cpp \
  netbase.cpp \
  protocol.cpp \
  rpcprotocol.cpp \
  sync.cpp \
  util.cpp \
  version.cpp \
  $(BITCOIN_CORE_H)

libpeercoin_cli_a_SOURCES = \
  rpcclient.cpp \
  $(BITCOIN_CORE_H)

nodist_libpeercoin_common_a_SOURCES = $(top_srcdir)/src/obj/build.h
#

# peercoind binary #
peercoind_LDADD = \
  libpeercoin_server.a \
  libpeercoin_cli.a \
  libpeercoin_common.a \
  $(LIBLEVELDB) \
  $(LIBMEMENV) \
  $(BOOST_LIBS)
if ENABLE_WALLET
peercoind_LDADD += libpeercoin_wallet.a
endif
peercoind_SOURCES = bitcoind.cpp
peercoind_LDFLAGS = $(AM_LDFLAGS)

if TARGET_WINDOWS
peercoind_SOURCES += bitcoind-res.rc
endif

AM_CPPFLAGS += $(BDB_CPPFLAGS)
peercoind_LDADD += $(BDB_LIBS)

# bitcoin-cli binary #
peercoin_cli_LDADD = libpeercoin_cli.a libpeercoin_common.a $(BOOST_LIBS)
peercoin_cli_SOURCES = bitcoin-cli.cpp
peercoin_cli_LDFLAGS = $(AM_LDFLAGS)

if TARGET_WINDOWS
peercoin_cli_SOURCES += bitcoin-cli-res.rc
endif

CLEANFILES = leveldb/libleveldb.a leveldb/libmemenv.a *.gcda *.gcno

DISTCLEANFILES = obj/build.h

EXTRA_DIST = leveldb

clean-local:
	-$(MAKE) -C leveldb clean
	rm -f leveldb/*/*.gcno leveldb/helpers/memenv/*.gcno

.rc.o:
	@test -f $(WINDRES) && $(WINDRES) -i $< -o $@ || \
	  echo error: could not build $@

.mm.o:
	$(OBJC) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) \
	  $(CPPFLAGS) $(AM_CXXFLAGS) $(QT_INCLUDES) $(CXXFLAGS)  -c -o $@ $<

LIBPEERCOIN_SERVER=libpeercoin_server.a
LIBPEERCOIN_WALLET=libpeercoin_wallet.a
LIBPEERCOIN_COMMON=libpeercoin_common.a
LIBPEERCOIN_CLI=libpeercoin_cli.a
LIBPEERCOINQT=qt/libpeercoinqt.a

if ENABLE_TESTS
include Makefile.test.include
endif

if ENABLE_QT
include Makefile.qt.include
endif

if ENABLE_QT_TESTS
include Makefile.qttest.include
endif
