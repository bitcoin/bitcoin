include Makefile.include
AM_CPPFLAGS =  $(INCLUDES) -I$(top_builddir)/src/obj \
  -I$(top_srcdir)/src/leveldb/include -I$(top_srcdir)/src/leveldb/helpers \
  -I$(builddir) $(BOOST_INCLUDES) $(BDB_CPPFLAGS)
AM_LDFLAGS = $(PTHREAD_CFLAGS)

noinst_LIBRARIES = libbitcoin.a

bin_PROGRAMS = bitcoind bitcoin-cli

SUBDIRS = . $(BUILD_QT) $(BUILD_TEST)
DIST_SUBDIRS = . qt test
.PHONY: FORCE
# bitcoin core #
BITCOIN_CORE_H = addrman.h alert.h allocators.h base58.h bignum.h \
  bitcoinrpc.h bloom.h chainparams.h checkpoints.h checkqueue.h \
  clientversion.h compat.h core.h crypter.h db.h hash.h init.h \
  key.h keystore.h leveldbwrapper.h limitedmap.h main.h miner.h mruset.h \
  netbase.h net.h protocol.h script.h serialize.h sync.h threadsafety.h \
  txdb.h txmempool.h ui_interface.h uint256.h util.h version.h walletdb.h wallet.h

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

libbitcoin_a_SOURCES = addrman.cpp alert.cpp allocators.cpp bitcoinrpc.cpp bloom.cpp \
  chainparams.cpp checkpoints.cpp core.cpp crypter.cpp db.cpp hash.cpp \
  init.cpp key.cpp keystore.cpp leveldbwrapper.cpp main.cpp miner.cpp \
  netbase.cpp net.cpp noui.cpp protocol.cpp rpcblockchain.cpp rpcdump.cpp \
  rpcmining.cpp rpcnet.cpp rpcrawtransaction.cpp rpcwallet.cpp script.cpp \
  sync.cpp txdb.cpp txmempool.cpp util.cpp version.cpp wallet.cpp walletdb.cpp $(JSON_H) \
  $(BITCOIN_CORE_H)

nodist_libbitcoin_a_SOURCES = $(top_srcdir)/src/obj/build.h
#

# bitcoind binary #
bitcoind_LDADD = libbitcoin.a leveldb/libleveldb.a leveldb/libmemenv.a \
  $(BOOST_LIBS)
bitcoind_SOURCES = bitcoind.cpp
#

if TARGET_WINDOWS
bitcoind_SOURCES += bitcoind-res.rc
endif

AM_CPPFLAGS += $(BDB_CPPFLAGS)
bitcoind_LDADD += $(BDB_LIBS)

# bitcoin-cli binary #
bitcoin_cli_LDADD = libbitcoin.a leveldb/libleveldb.a leveldb/libmemenv.a \
  $(BOOST_LIBS)
bitcoin_cli_SOURCES = bitcoin-cli.cpp
#

if TARGET_WINDOWS
bitcoin_cli_SOURCES += bitcoin-cli-res.rc
endif
bitcoin_cli_LDADD += $(BDB_LIBS)

leveldb/libleveldb.a: leveldb/libmemenv.a

leveldb/%.a:
	@echo "Building LevelDB ..." && $(MAKE) -C $(@D) $(@F) CXX="$(CXX)" \
	  CC="$(CC)" PLATFORM=$(TARGET_OS) AR="$(AR)" $(LEVELDB_TARGET_FLAGS) \
	  OPT="$(CXXFLAGS) $(CPPFLAGS)"

qt/bitcoinstrings.cpp: $(libbitcoin_a_SOURCES)
	@test -n $(XGETTEXT) || echo "xgettext is required for updating translations"
	@cd $(top_srcdir); XGETTEXT=$(XGETTEXT) share/qt/extract_strings_qt.py

CLEANFILES = leveldb/libleveldb.a leveldb/libmemenv.a *.gcda *.gcno

DISTCLEANFILES = obj/build.h

EXTRA_DIST = leveldb Makefile.include

clean-local:
	-$(MAKE) -C leveldb clean
	rm -f leveldb/port/*.gcno leveldb/db/*.gcno leveldb/table/*.gcno leveldb/helpers/*.gcno
	rm -f leveldb/util/*.gcno leveldb/helpers/memenv/*.gcno
