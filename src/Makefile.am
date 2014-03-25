include Makefile.include

AM_CPPFLAGS += -I$(builddir)

noinst_LIBRARIES = \
  libbitcredit_server.a \
  libbitcredit_common.a \
  libbitcredit_cli.a
if ENABLE_WALLET
noinst_LIBRARIES += libbitcredit_wallet.a
endif

bin_PROGRAMS =

if BUILD_BITCOIND
  bin_PROGRAMS += bitcreditd
endif

if BUILD_BITCOIN_CLI
  bin_PROGRAMS += bitcredit-cli
endif

SUBDIRS = . $(BUILD_QT) $(BUILD_TEST)
DIST_SUBDIRS = . qt test
.PHONY: FORCE
# bitcoin core #
BITCOIN_CORE_H = \
  addrman.h \
  alert.h \
  allocators.h \
  base58.h \
  bloom.h \
  chainparams.h \
  checkpoints.h \
  bitcoin_checkpoints.h \
  checkqueue.h \
  clientversion.h \
  coincontrol.h \
  coins.h \
  bitcoin_coins.h \
  bitcoin_claimcoins.h \
  compat.h \
  core.h \
  bitcoin_core.h \
  crypter.h \
  db.h \
  bitcoin_db.h \
  hash.h \
  init.h \
  key.h \
  keystore.h \
  leveldbwrapper.h \
  limitedmap.h \
  main_common.h \
  main.h \
  bitcoin_main.h \
  miner.h \
  mruset.h \
  netbase.h \
  net.h \
  noui.h \
  protocol.h \
  rpcclient.h \
  rpcprotocol.h \
  rpcserver.h \
  script.h \
  bitcoin_script.h \
  serialize.h \
  sync.h \
  subsidylevels.h \
  threadsafety.h \
  tinyformat.h \
  txdb.h \
  bitcoin_txdb.h \
  bitcoin_claimtxdb.h \
  txmempool.h \
  bitcoin_txmempool.h \
  ui_interface.h \
  uint256.h \
  util.h \
  version.h \
  walletdb.h \
  bitcoin_walletdb.h \
  wallet.h \
  bitcoin_wallet.h

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

libbitcredit_server_a_SOURCES = \
  addrman.cpp \
  alert.cpp \
  bloom.cpp \
  checkpoints.cpp \
  bitcoin_checkpoints.cpp \
  coins.cpp \
  bitcoin_coins.cpp \
  bitcoin_claimcoins.cpp \
  init.cpp \
  keystore.cpp \
  leveldbwrapper.cpp \
  main_common.cpp \
  main.cpp \
  bitcoin_main.cpp \
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
  bitcoin_txdb.cpp \
  bitcoin_claimtxdb.cpp \
  txmempool.cpp \
  bitcoin_txmempool.cpp \
  $(JSON_H) \
  $(BITCOIN_CORE_H)

libbitcredit_wallet_a_SOURCES = \
  db.cpp \
  bitcoin_db.cpp \
  crypter.cpp \
  rpcdump.cpp \
  rpcwallet.cpp \
  wallet.cpp \
  bitcoin_wallet.cpp \
  walletdb.cpp \
  bitcoin_walletdb.cpp \
  $(BITCOIN_CORE_H)

libbitcredit_common_a_SOURCES = \
  base58.cpp \
  allocators.cpp \
  chainparams.cpp \
  core.cpp \
  bitcoin_core.cpp \
  hash.cpp \
  key.cpp \
  netbase.cpp \
  protocol.cpp \
  rpcprotocol.cpp \
  script.cpp \
  bitcoin_script.cpp \
  sync.cpp \
  util.cpp \
  version.cpp \
  $(BITCOIN_CORE_H)

if GLIBC_BACK_COMPAT
libbitcredit_common_a_SOURCES += compat/glibc_compat.cpp
libbitcredit_common_a_SOURCES += compat/glibcxx_compat.cpp
endif

libbitcredit_cli_a_SOURCES = \
  rpcclient.cpp \
  $(BITCOIN_CORE_H)

nodist_libbitcredit_common_a_SOURCES = $(top_srcdir)/src/obj/build.h
#

# bitcreditd binary #
bitcreditd_LDADD = \
  libbitcredit_server.a \
  libbitcredit_cli.a \
  libbitcredit_common.a \
  $(LIBLEVELDB) \
  $(LIBMEMENV)
if ENABLE_WALLET
bitcreditd_LDADD += libbitcredit_wallet.a
endif
bitcreditd_SOURCES = bitcreditd.cpp
#

if TARGET_WINDOWS
bitcreditd_SOURCES += bitcreditd-res.rc
endif

AM_CPPFLAGS += $(BDB_CPPFLAGS)
bitcreditd_LDADD += $(BOOST_LIBS) $(BDB_LIBS)

# bitcredit-cli binary #
bitcredit_cli_LDADD = \
  libbitcredit_cli.a \
  libbitcredit_common.a \
  $(BOOST_LIBS)
bitcredit_cli_SOURCES = bitcredit-cli.cpp
#

if TARGET_WINDOWS
bitcredit_cli_SOURCES += bitcredit-cli-res.rc
endif

# NOTE: This dependency is not strictly necessary, but without it make may try to build both in parallel, which breaks the LevelDB build system in a race
leveldb/libleveldb.a: leveldb/libmemenv.a

leveldb/%.a:
	@echo "Building LevelDB ..." && $(MAKE) -C $(@D) $(@F) CXX="$(CXX)" \
	  CC="$(CC)" PLATFORM=$(TARGET_OS) AR="$(AR)" $(LEVELDB_TARGET_FLAGS) \
	  OPT="$(CXXFLAGS) $(CPPFLAGS)"

qt/bitcoinstrings.cpp: $(libbitcredit_server_a_SOURCES) $(libbitcredit_common_a_SOURCES) $(libbitcredit_cli_a_SOURCES)
	@test -n $(XGETTEXT) || echo "xgettext is required for updating translations"
	@cd $(top_srcdir); XGETTEXT=$(XGETTEXT) share/qt/extract_strings_qt.py

CLEANFILES = leveldb/libleveldb.a leveldb/libmemenv.a *.gcda *.gcno

DISTCLEANFILES = obj/build.h

EXTRA_DIST = leveldb Makefile.include

clean-local:
	-$(MAKE) -C leveldb clean
	rm -f leveldb/*/*.gcno leveldb/helpers/memenv/*.gcno
