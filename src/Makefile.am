include Makefile.include

AM_CPPFLAGS += -I$(builddir)

lib_LTLIBRARIES = \
  libbitcoincore_common.la \
  libbitcoincore_server.la \
  libbitcoincore_cli.la
if ENABLE_WALLET
lib_LTLIBRARIES += libbitcoincore_wallet.la
endif

bin_PROGRAMS =

if BUILD_BITCOIND
  bin_PROGRAMS += bitcoind
endif

if BUILD_BITCOIN_CLI
  bin_PROGRAMS += bitcoin-cli
endif

SUBDIRS = . $(BUILD_QT) $(BUILD_TEST)
DIST_SUBDIRS = . qt test
.PHONY: FORCE

libbitcoincore_json_includedir = $(includedir)/$(PACKAGE)/json
libbitcoincore_json_include_HEADERS = \
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
version.lo: obj/build.h

# common utilities library #
libbitcoincore_common_includedir = $(includedir)/$(PACKAGE)
libbitcoincore_common_include_HEADERS = \
  allocators.h \
  base58.h \
  bignum.h \
  chainparams.h \
  clientversion.h \
  compat.h \
  core.h \
  hash.h \
  key.h \
  limitedmap.h \
  netbase.h \
  protocol.h \
  rpcprotocol.h \
  script.h \
  serialize.h \
  sync.h \
  threadsafety.h \
  tinyformat.h \
  uint256.h \
  util.h \
  version.h
libbitcoincore_common_la_SOURCES = \
  allocators.cpp \
  chainparams.cpp \
  core.cpp \
  hash.cpp \
  key.cpp \
  netbase.cpp \
  protocol.cpp \
  rpcprotocol.cpp \
  script.cpp \
  sync.cpp \
  util.cpp \
  version.cpp \
  $(libbitcoincore_json_include_HEADERS) \
  $(libbitcoincore_common_include_HEADERS)
libbitcoincore_common_la_LDFLAGS = -version-info $(CLIENT_VERSION_DYNLIB)
libbitcoincore_common_la_LIBADD = $(BOOST_LIBS)
nodist_libbitcoincore_common_la_SOURCES = $(top_srcdir)/src/obj/build.h

# server library #
libbitcoincore_server_includedir = $(includedir)/$(PACKAGE)
libbitcoincore_server_include_HEADERS = \
  addrman.h \
  alert.h \
  bloom.h \
  checkpoints.h \
  checkqueue.h \
  coincontrol.h \
  coins.h \
  init.h \
  keystore.h \
  leveldbwrapper.h \
  main.h \
  miner.h \
  net.h \
  noui.h \
  rpcclient.h \
  rpcserver.h \
  txdb.h \
  ui_interface.h
libbitcoincore_server_la_SOURCES = \
  addrman.cpp \
  alert.cpp \
  bloom.cpp \
  checkpoints.cpp \
  coins.cpp \
  init.cpp \
  keystore.cpp \
  leveldbwrapper.cpp \
  main.cpp \
  miner.cpp \
  net.cpp \
  noui.cpp \
  rpcblockchain.cpp \
  rpcmining.cpp \
  rpcmisc.cpp \
  rpcnet.cpp \
  rpcrawtransaction.cpp \
  rpcserver.cpp \
  txdb.cpp \
  txmempool.cpp \
  $(libbitcoincore_json_include_HEADERS) \
  $(libbitcoincore_common_include_HEADERS) \
  $(libbitcoincore_server_include_HEADERS)
libbitcoincore_server_la_LDFLAGS = -version-info $(CLIENT_VERSION_DYNLIB)
libbitcoincore_server_la_LIBADD = \
  libbitcoincore_common.la \
  $(BOOST_LIBS) $(LIBLEVELDB) $(LIBMEMENV)

# wallet library #
libbitcoincore_wallet_includedir = $(includedir)/$(PACKAGE)
libbitcoincore_wallet_include_HEADERS = \
  crypter.h \
  db.h \
  mruset.h \
  txmempool.h \
  walletdb.h \
  wallet.h
libbitcoincore_wallet_la_SOURCES = \
  db.cpp \
  crypter.cpp \
  rpcdump.cpp \
  rpcwallet.cpp \
  wallet.cpp \
  walletdb.cpp \
  $(libbitcoincore_json_include_HEADERS) \
  $(libbitcoincore_common_include_HEADERS) \
  $(libbitcoincore_server_include_HEADERS) \
  $(libbitcoincore_wallet_include_HEADERS)
libbitcoincore_wallet_la_LDFLAGS = -version-info $(CLIENT_VERSION_DYNLIB)
libbitcoincore_wallet_la_LIBADD = \
  libbitcoincore_server.la \
  libbitcoincore_common.la \
  $(BOOST_LIBS) $(BDB_LIBS)

# RPC client library #
libbitcoincore_cli_includedir = $(includedir)/$(PACKAGE)
libbitcoincore_cli_include_HEADERS =
libbitcoincore_cli_la_SOURCES = \
  rpcclient.cpp \
  $(libbitcoincore_json_include_HEADERS) \
  $(libbitcoincore_common_include_HEADERS)
libbitcoincore_cli_la_LDFLAGS = -version-info $(CLIENT_VERSION_DYNLIB)
libbitcoincore_cli_la_LIBADD = \
  libbitcoincore_common.la \
  $(BOOST_LIBS)

# bitcoind binary #
bitcoind_LDADD = \
  libbitcoincore_server.la \
  libbitcoincore_cli.la \
  libbitcoincore_common.la \
  $(LIBLEVELDB) $(LIBMEMENV) \
  $(BOOST_LIBS)
if ENABLE_WALLET
bitcoind_LDADD += libbitcoincore_wallet.la
endif
bitcoind_SOURCES = bitcoind.cpp
#

if TARGET_WINDOWS
bitcoind_SOURCES += bitcoind-res.rc
endif

AM_CPPFLAGS += $(BDB_CPPFLAGS)

# bitcoin-cli binary #
bitcoin_cli_LDADD = \
  libbitcoincore_cli.la \
  libbitcoincore_common.la \
  $(BOOST_LIBS)
bitcoin_cli_SOURCES = bitcoin-cli.cpp
#

if TARGET_WINDOWS
bitcoin_cli_SOURCES += bitcoin-cli-res.rc
endif

# NOTE: This dependency is not strictly necessary, but without it make may try to build both in parallel, which breaks the LevelDB build system in a race
leveldb/libleveldb.a: leveldb/libmemenv.a

leveldb/%.a:
	@echo "Building LevelDB ..." && $(MAKE) -C $(@D) $(@F) CXX="$(CXX)" \
	  CC="$(CC)" PLATFORM=$(TARGET_OS) AR="$(AR)" $(LEVELDB_TARGET_FLAGS) \
	  OPT="$(CXXFLAGS) $(CPPFLAGS) $(LEVELDB_EXTRA_CXXFLAGS)"

qt/bitcoinstrings.cpp: $(libbitcoincore_server_la_SOURCES) $(libbitcoincore_common_la_SOURCES) $(libbitcoincore_cli_la_SOURCES)
	@test -n $(XGETTEXT) || echo "xgettext is required for updating translations"
	@cd $(top_srcdir); XGETTEXT=$(XGETTEXT) share/qt/extract_strings_qt.py

CLEANFILES = leveldb/libleveldb.a leveldb/libmemenv.a *.gcda *.gcno

DISTCLEANFILES = obj/build.h

EXTRA_DIST = leveldb Makefile.include

clean-local:
	-$(MAKE) -C leveldb clean
	rm -f leveldb/*/*.gcno leveldb/helpers/memenv/*.gcno
