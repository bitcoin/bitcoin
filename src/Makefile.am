AM_CPPFLAGS =  $(INCLUDES)
AM_LDFLAGS = $(PTHREAD_CFLAGS)


if EMBEDDED_LEVELDB
LEVELDB_CPPFLAGS += -I$(srcdir)/leveldb/include
LEVELDB_CPPFLAGS += -I$(srcdir)/leveldb/helpers/memenv
LIBLEVELDB += $(builddir)/leveldb/libleveldb.a
LIBMEMENV  += $(builddir)/leveldb/libmemenv.a

# NOTE: This dependency is not strictly necessary, but without it make may try to build both in parallel, which breaks the LevelDB build system in a race
$(LIBLEVELDB): $(LIBMEMENV)

$(LIBLEVELDB) $(LIBMEMENV):
	@echo "Building LevelDB ..." && $(MAKE) -C $(@D) $(@F) CXX="$(CXX)" \
	  CC="$(CC)" PLATFORM=$(TARGET_OS) AR="$(AR)" $(LEVELDB_TARGET_FLAGS) \
          OPT="$(CXXFLAGS) $(CPPFLAGS)"
endif

BITCOIN_CONFIG_INCLUDES=-I$(builddir)/config
BITCOIN_INCLUDES=-I$(builddir) -I$(builddir)/obj $(BOOST_CPPFLAGS) $(LEVELDB_CPPFLAGS)

LIBBITCOIN_SERVER=libbitcoin_server.a
LIBBITCOIN_WALLET=libbitcoin_wallet.a
LIBBITCOIN_COMMON=libbitcoin_common.a
LIBBITCOIN_CLI=libbitcoin_cli.a
LIBBITCOIN_UTIL=libbitcoin_util.a
LIBBITCOIN_CRYPTO=crypto/libbitcoin_crypto.a
LIBBITCOINQT=qt/libbitcoinqt.a

noinst_LIBRARIES = \
  libbitcoin_server.a \
  libbitcoin_common.a \
  libbitcoin_cli.a \
  libbitcoin_util.a \
  crypto/libbitcoin_crypto.a
if ENABLE_WALLET
BITCOIN_INCLUDES += $(BDB_CPPFLAGS)
noinst_LIBRARIES += libbitcoin_wallet.a
endif

bin_PROGRAMS =
TESTS =

if BUILD_BITCOIND
  bin_PROGRAMS += bitcoind
endif

if BUILD_BITCOIN_CLI
  bin_PROGRAMS += bitcoin-cli
endif

.PHONY: FORCE
# bitcoin core #
BITCOIN_CORE_H = \
  addrman.h \
  alert.h \
  allocators.h \
  base58.h \
  bloom.h \
  chainparams.h \
  chainparamsbase.h \
  checkpoints.h \
  checkqueue.h \
  clientversion.h \
  coincontrol.h \
  coins.h \
  compat.h \
  core.h \
  crypter.h \
  db.h \
  hash.h \
  init.h \
  key.h \
  keystore.h \
  leveldbwrapper.h \
  limitedmap.h \
  main.h \
  miner.h \
  mruset.h \
  netbase.h \
  net.h \
  noui.h \
  pow.h \
  protocol.h \
  rpcclient.h \
  rpcprotocol.h \
  rpcserver.h \
  script.h \
  serialize.h \
  sync.h \
  threadsafety.h \
  timedata.h \
  tinyformat.h \
  txdb.h \
  txmempool.h \
  ui_interface.h \
  uint256.h \
  util.h \
  version.h \
  walletdb.h \
  wallet.h \
  compat/sanity.h

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
	@$(MKDIR_P) $(builddir)/obj
	@$(top_srcdir)/share/genbuild.sh $(abs_top_builddir)/src/obj/build.h \
	  $(abs_top_srcdir)
libbitcoin_util_a-version.$(OBJEXT): obj/build.h

# server: shared between bitcoind and bitcoin-qt
libbitcoin_server_a_CPPFLAGS = $(BITCOIN_INCLUDES)
libbitcoin_server_a_SOURCES = \
  addrman.cpp \
  alert.cpp \
  bloom.cpp \
  checkpoints.cpp \
  init.cpp \
  leveldbwrapper.cpp \
  main.cpp \
  miner.cpp \
  net.cpp \
  noui.cpp \
  pow.cpp \
  rpcblockchain.cpp \
  rpcmining.cpp \
  rpcmisc.cpp \
  rpcnet.cpp \
  rpcrawtransaction.cpp \
  rpcserver.cpp \
  timedata.cpp \
  txdb.cpp \
  txmempool.cpp \
  $(JSON_H) \
  $(BITCOIN_CORE_H)

# wallet: shared between bitcoind and bitcoin-qt, but only linked
# when wallet enabled
libbitcoin_wallet_a_CPPFLAGS = $(BITCOIN_INCLUDES)
libbitcoin_wallet_a_SOURCES = \
  db.cpp \
  crypter.cpp \
  rpcdump.cpp \
  rpcwallet.cpp \
  wallet.cpp \
  walletdb.cpp \
  $(BITCOIN_CORE_H)

# crypto primitives library
crypto_libbitcoin_crypto_a_CPPFLAGS = $(BITCOIN_CONFIG_INCLUDES)
crypto_libbitcoin_crypto_a_SOURCES = \
  crypto/sha1.cpp \
  crypto/sha2.cpp \
  crypto/ripemd160.cpp \
  crypto/common.h \
  crypto/sha2.h \
  crypto/sha1.h \
  crypto/ripemd160.h

# common: shared between bitcoind, and bitcoin-qt and non-server tools
libbitcoin_common_a_CPPFLAGS = $(BITCOIN_INCLUDES)
libbitcoin_common_a_SOURCES = \
  allocators.cpp \
  base58.cpp \
  chainparams.cpp \
  coins.cpp \
  core.cpp \
  hash.cpp \
  key.cpp \
  keystore.cpp \
  netbase.cpp \
  protocol.cpp \
  script.cpp \
  $(BITCOIN_CORE_H)

# util: shared between all executables.
# This library *must* be included to make sure that the glibc
# backward-compatibility objects and their sanity checks are linked.
libbitcoin_util_a_CPPFLAGS = $(BITCOIN_INCLUDES)
libbitcoin_util_a_SOURCES = \
  chainparamsbase.cpp \
  rpcprotocol.cpp \
  sync.cpp \
  util.cpp \
  version.cpp \
  compat/glibc_sanity.cpp \
  compat/glibcxx_sanity.cpp \
  $(BITCOIN_CORE_H)

if GLIBC_BACK_COMPAT
libbitcoin_util_a_SOURCES += compat/glibc_compat.cpp
libbitcoin_util_a_SOURCES += compat/glibcxx_compat.cpp
endif

# cli: shared between bitcoin-cli and bitcoin-qt
libbitcoin_cli_a_SOURCES = \
  rpcclient.cpp \
  $(BITCOIN_CORE_H)

nodist_libbitcoin_util_a_SOURCES = $(srcdir)/obj/build.h
#

# bitcoind binary #
bitcoind_LDADD = \
  $(LIBBITCOIN_SERVER) \
  $(LIBBITCOIN_COMMON) \
  $(LIBBITCOIN_UTIL) \
  $(LIBBITCOIN_CRYPTO) \
  $(LIBLEVELDB) \
  $(LIBMEMENV)
if ENABLE_WALLET
bitcoind_LDADD += libbitcoin_wallet.a
endif
bitcoind_SOURCES = bitcoind.cpp
#

if TARGET_WINDOWS
bitcoind_SOURCES += bitcoind-res.rc
endif

bitcoind_LDADD += $(BOOST_LIBS) $(BDB_LIBS)
bitcoind_CPPFLAGS = $(BITCOIN_INCLUDES)

# bitcoin-cli binary #
bitcoin_cli_LDADD = \
  $(LIBBITCOIN_CLI) \
  $(LIBBITCOIN_COMMON) \
  $(LIBBITCOIN_UTIL) \
  $(LIBBITCOIN_CRYPTO) \
  $(BOOST_LIBS)
bitcoin_cli_SOURCES = \
  bitcoin-cli.cpp
bitcoin_cli_CPPFLAGS = $(BITCOIN_INCLUDES)
#

if TARGET_WINDOWS
bitcoin_cli_SOURCES += bitcoin-cli-res.rc
endif

CLEANFILES = leveldb/libleveldb.a leveldb/libmemenv.a *.gcda *.gcno

DISTCLEANFILES = obj/build.h

EXTRA_DIST = leveldb

clean-local:
	-$(MAKE) -C leveldb clean
	rm -f leveldb/*/*.gcno leveldb/helpers/memenv/*.gcno
	-rm -f config.h

.rc.o:
	@test -f $(WINDRES)
	$(AM_V_GEN) $(WINDRES) -i $< -o $@

.mm.o:
	$(AM_V_CXX) $(OBJCXX) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) \
	  $(CPPFLAGS) $(AM_CXXFLAGS) $(QT_INCLUDES) $(CXXFLAGS)  -c -o $@ $<

%.pb.cc %.pb.h: %.proto
	@test -f $(PROTOC)
	$(AM_V_GEN) $(PROTOC) --cpp_out=$(@D) --proto_path=$(abspath $(<D) $<)

if ENABLE_TESTS
include Makefile.test.include
endif

if ENABLE_QT
include Makefile.qt.include
endif

if ENABLE_QT_TESTS
include Makefile.qttest.include
endif
