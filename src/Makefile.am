include Makefile.include

AM_CPPFLAGS += -I$(builddir)

noinst_LIBRARIES = \
  libcrowncoin_server.a \
  libcrowncoin_common.a \
  libcrowncoin_cli.a
if ENABLE_WALLET
noinst_LIBRARIES += libcrowncoin_wallet.a
endif

bin_PROGRAMS =

if BUILD_CROWNCOIND
  bin_PROGRAMS += crowncoind
endif

if BUILD_CROWNCOIN_CLI
  bin_PROGRAMS += crowncoin-cli
endif

SUBDIRS = . $(BUILD_QT) $(BUILD_TEST)
DIST_SUBDIRS = . qt test
.PHONY: FORCE
# Crowncoin #
BITCOIN_CORE_H = \
  activethrone.h \
  addrman.h \
  alert.h \
  allocators.h \
  base58.h bignum.h \
  bloom.h \
  chainparams.h \
  checkpoints.h \
  checkqueue.h \
  clientversion.h \
  coincontrol.h \
  coins.h \
  compat.h \
  core.h \
  crypter.h \
  darksend.h \
  darksend-relay.h \
  db.h \
  hash.h \
  init.h \
  instantx.h \
  key.h \
  keepass.h \
  keystore.h \
  leveldbwrapper.h \
  limitedmap.h \
  main.h \
  throne.h \
  throne-pos.h \
  throneman.h \
  throneconfig.h \
  miner.h \
  mruset.h \
  names.h \
  netbase.h \
  net.h \
  noui.h \
  protocol.h \
  rpcclient.h \
  rpcprotocol.h \
  rpcserver.h \
  script.h \
  serialize.h \
  spork.h \
  sync.h \
  threadsafety.h \
  tinyformat.h \
  txdb.h \
  txmempool.h \
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

libcrowncoin_server_a_SOURCES = \
  activethrone.cpp \
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
  names.cpp \
  net.cpp \
  noui.cpp \
  rpcblockchain.cpp \
  rpcdarksend.cpp \
  rpcmining.cpp \
  rpcmisc.cpp \
  rpcnames.cpp \
  rpcnet.cpp \
  rpcrawtransaction.cpp \
  rpcserver.cpp \
  txdb.cpp \
  txmempool.cpp \
  $(JSON_H) \
  $(BITCOIN_CORE_H)

libcrowncoin_wallet_a_SOURCES = \
  activethrone.cpp \
  db.cpp \
  crypter.cpp \
  rpcdump.cpp \
  rpcwallet.cpp \
  wallet.cpp \
  walletdb.cpp \
  keepass.cpp \
  $(BITCOIN_CORE_H)

libcrowncoin_common_a_SOURCES = \
  activethrone.cpp \
  auxpow.cpp \
  base58.cpp \
  allocators.cpp \
  chainparams.cpp \
  core.cpp \
  darksend.cpp \
  darksend-relay.cpp \
  throne.cpp \
  throne-pos.cpp \
  throneman.cpp \
  throneconfig.cpp \
  instantx.cpp \
  hash.cpp \
  key.cpp \
  netbase.cpp \
  protocol.cpp \
  rpcprotocol.cpp \
  script.cpp \
  sync.cpp \
  util.cpp \
  version.cpp \
  spork.cpp \
  $(BITCOIN_CORE_H)

if GLIBC_BACK_COMPAT
libcrowncoin_common_a_SOURCES += compat/glibc_compat.cpp
libcrowncoin_common_a_SOURCES += compat/glibcxx_compat.cpp
endif

libcrowncoin_cli_a_SOURCES = \
  rpcclient.cpp \
  $(BITCOIN_CORE_H)

nodist_libcrowncoin_common_a_SOURCES = $(top_srcdir)/src/obj/build.h
#

# crowncoind binary #
crowncoind_LDADD = \
  libcrowncoin_server.a \
  libcrowncoin_cli.a \
  libcrowncoin_common.a \
  $(LIBLEVELDB) \
  $(LIBMEMENV)
if ENABLE_WALLET
crowncoind_LDADD += libcrowncoin_wallet.a
endif
crowncoind_SOURCES = crowncoind.cpp
#

if TARGET_WINDOWS
crowncoind_SOURCES += crowncoind-res.rc
endif

AM_CPPFLAGS += $(BDB_CPPFLAGS)
crowncoind_LDADD += $(BOOST_LIBS) $(BDB_LIBS)

# crowncoin-cli binary #
crowncoin_cli_LDADD = \
  libcrowncoin_cli.a \
  libcrowncoin_common.a \
  $(BOOST_LIBS)
crowncoin_cli_SOURCES = crowncoin-cli.cpp
#

if TARGET_WINDOWS
crowncoin_cli_SOURCES += crowncoin-cli-res.rc
endif

# NOTE: This dependency is not strictly necessary, but without it make may try to build both in parallel, which breaks the LevelDB build system in a race
leveldb/libleveldb.a: leveldb/libmemenv.a

leveldb/%.a:
	@echo "Building LevelDB ..." && $(MAKE) -C $(@D) $(@F) CXX="$(CXX)" \
	  CC="$(CC)" PLATFORM=$(TARGET_OS) AR="$(AR)" $(LEVELDB_TARGET_FLAGS) \
	  OPT="$(CXXFLAGS) $(CPPFLAGS)"

qt/crowncoinstrings.cpp: $(libcrowncoin_server_a_SOURCES) $(libcrowncoin_common_a_SOURCES) $(libcrowncoin_cli_a_SOURCES)
	@test -n $(XGETTEXT) || echo "xgettext is required for updating translations"
	@cd $(top_srcdir); XGETTEXT=$(XGETTEXT) share/qt/extract_strings_qt.py

CLEANFILES = leveldb/libleveldb.a leveldb/libmemenv.a *.gcda *.gcno

DISTCLEANFILES = obj/build.h

EXTRA_DIST = leveldb Makefile.include

clean-local:
	-$(MAKE) -C leveldb clean
	rm -f leveldb/*/*.gcno leveldb/helpers/memenv/*.gcno
