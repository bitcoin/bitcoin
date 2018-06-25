# Copyright (c) 2013-2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

DIST_SUBDIRS = secp256k1 univalue

AM_LDFLAGS = $(PTHREAD_CFLAGS) $(LIBTOOL_LDFLAGS) $(HARDENED_LDFLAGS) $(GPROF_LDFLAGS) $(SANITIZER_LDFLAGS)
AM_CXXFLAGS = $(DEBUG_CXXFLAGS) $(HARDENED_CXXFLAGS) $(WARN_CXXFLAGS) $(NOWARN_CXXFLAGS) $(ERROR_CXXFLAGS) $(GPROF_CXXFLAGS) $(SANITIZER_CXXFLAGS)
AM_CPPFLAGS = $(DEBUG_CPPFLAGS) $(HARDENED_CPPFLAGS)
AM_LIBTOOLFLAGS = --preserve-dup-deps
EXTRA_LIBRARIES =

if EMBEDDED_UNIVALUE
LIBUNIVALUE = univalue/libunivalue.la

$(LIBUNIVALUE): $(wildcard univalue/lib/*) $(wildcard univalue/include/*)
	$(AM_V_at)$(MAKE) $(AM_MAKEFLAGS) -C $(@D) $(@F)
else
LIBUNIVALUE = $(UNIVALUE_LIBS)
endif

BITCOIN_INCLUDES=-I$(builddir) $(BDB_CPPFLAGS) $(BOOST_CPPFLAGS) $(LEVELDB_CPPFLAGS) $(CRYPTO_CFLAGS) $(SSL_CFLAGS)

BITCOIN_INCLUDES += -I$(srcdir)/secp256k1/include
BITCOIN_INCLUDES += $(UNIVALUE_CFLAGS)

LIBPARTICL_SERVER=libparticl_server.a
LIBPARTICL_COMMON=libparticl_common.a
LIBPARTICL_CONSENSUS=libparticl_consensus.a
LIBPARTICL_CLI=libparticl_cli.a
LIBPARTICL_UTIL=libparticl_util.a
LIBPARTICL_CRYPTO_BASE=crypto/libparticl_crypto_base.a
LIBPARTICLQT=qt/libparticlqt.a
LIBSECP256K1=secp256k1/libsecp256k1.la
LIBPARTICL_SMSG=libparticl_smsg.a

if ENABLE_ZMQ
LIBPARTICL_ZMQ=libparticl_zmq.a
endif
if BUILD_BITCOIN_LIBS
LIBPARTICLCONSENSUS=libparticlconsensus.la
endif
if ENABLE_WALLET
LIBPARTICL_WALLET=libparticl_wallet.a
endif
if ENABLE_USBDEVICE
LIBPARTICL_USBDEVICE=libparticl_usbdevice.a
endif

LIBPARTICL_CRYPTO= $(LIBPARTICL_CRYPTO_BASE)
if ENABLE_SSE41
LIBPARTICL_CRYPTO_SSE41 = crypto/libparticl_crypto_sse41.a
LIBPARTICL_CRYPTO += $(LIBPARTICL_CRYPTO_SSE41)
endif
if ENABLE_AVX2
LIBPARTICL_CRYPTO_AVX2 = crypto/libparticl_crypto_avx2.a
LIBPARTICL_CRYPTO += $(LIBPARTICL_CRYPTO_AVX2)
endif

$(LIBSECP256K1): $(wildcard secp256k1/src/*) $(wildcard secp256k1/include/*)
	$(AM_V_at)$(MAKE) $(AM_MAKEFLAGS) -C $(@D) $(@F)

# Make is not made aware of per-object dependencies to avoid limiting building parallelization
# But to build the less dependent modules first, we manually select their order here:
EXTRA_LIBRARIES += \
  $(LIBPARTICL_CRYPTO) \
  $(LIBPARTICL_UTIL) \
  $(LIBPARTICL_COMMON) \
  $(LIBPARTICL_CONSENSUS) \
  $(LIBPARTICL_SERVER) \
  $(LIBPARTICL_CLI) \
  $(LIBPARTICL_WALLET) \
  $(LIBPARTICL_ZMQ) \
  $(LIBPARTICL_USBDEVICE) \
  $(LIBPARTICL_SMSG)

lib_LTLIBRARIES = $(LIBPARTICLCONSENSUS)

bin_PROGRAMS =
noinst_PROGRAMS =
TESTS =
BENCHMARKS =

if BUILD_BITCOIND
  bin_PROGRAMS += particld
endif

if BUILD_BITCOIN_UTILS
  bin_PROGRAMS += particl-cli particl-tx
endif

.PHONY: FORCE check-symbols check-security
# bitcoin core #
BITCOIN_CORE_H = \
  addrdb.h \
  addressindex.h \
  spentindex.h \
  timestampindex.h \
  rctindex.h \
  addrman.h \
  base58.h \
  bech32.h \
  bloom.h \
  blockencodings.h \
  chain.h \
  chainparams.h \
  chainparamsbase.h \
  chainparamsseeds.h \
  chainparamsimport.h \
  checkpoints.h \
  checkqueue.h \
  clientversion.h \
  coins.h \
  compat.h \
  compat/byteswap.h \
  compat/endian.h \
  compat/sanity.h \
  compressor.h \
  consensus/consensus.h \
  consensus/tx_verify.h \
  core_io.h \
  core_memusage.h \
  cuckoocache.h \
  fs.h \
  httprpc.h \
  httpserver.h \
  index/base.h \
  index/txindex.h \
  indirectmap.h \
  init.h \
  anon.h \
  blind.h \
  interfaces/handler.h \
  interfaces/node.h \
  interfaces/wallet.h \
  key.h \
  key/stealth.h \
  key/extkey.h \
  key/mnemonic.h \
  unilib/uninorms.h \
  unilib/utf8.h \
  key/types.h \
  key/keyutil.h \
  key/wordlists/chinese_simplified.h \
  key/wordlists/chinese_traditional.h \
  key/wordlists/english.h \
  key/wordlists/french.h \
  key/wordlists/japanese.h \
  key/wordlists/spanish.h \
  key/wordlists/italian.h \
  key/wordlists/korean.h \
  key_io.h \
  keystore.h \
  dbwrapper.h \
  limitedmap.h \
  logging.h \
  memusage.h \
  merkleblock.h \
  miner.h \
  net.h \
  net_processing.h \
  netaddress.h \
  netbase.h \
  netmessagemaker.h \
  noui.h \
  policy/feerate.h \
  policy/fees.h \
  policy/policy.h \
  policy/rbf.h \
  pow.h \
  pos/kernel.h \
  pos/miner.h \
  protocol.h \
  random.h \
  reverse_iterator.h \
  reverselock.h \
  rpc/blockchain.h \
  rpc/client.h \
  rpc/mining.h \
  rpc/protocol.h \
  rpc/server.h \
  rpc/rpcutil.h \
  rpc/rawtransaction.h \
  rpc/register.h \
  rpc/util.h \
  scheduler.h \
  script/ismine.h \
  script/sigcache.h \
  script/sign.h \
  script/standard.h \
  streams.h \
  smsg/db.h \
  smsg/crypter.h \
  smsg/net.h \
  smsg/smessage.h \
  smsg/rpcsmessage.h \
  support/allocators/secure.h \
  support/allocators/zeroafterfree.h \
  support/cleanse.h \
  support/events.h \
  support/lockedpool.h \
  sync.h \
  threadsafety.h \
  threadinterrupt.h \
  timedata.h \
  torcontrol.h \
  txdb.h \
  txmempool.h \
  ui_interface.h \
  undo.h \
  util.h \
  utilmoneystr.h \
  utiltime.h \
  validation.h \
  validationinterface.h \
  versionbits.h \
  walletinitinterface.h \
  wallet/coincontrol.h \
  wallet/crypter.h \
  wallet/db.h \
  wallet/feebumper.h \
  wallet/fees.h \
  wallet/rpcwallet.h \
  wallet/wallet.h \
  wallet/walletdb.h \
  wallet/rpchdwallet.h \
  wallet/hdwalletdb.h \
  wallet/hdwallet.h \
  wallet/walletutil.h \
  wallet/coinselection.h \
  warnings.h \
  zmq/zmqabstractnotifier.h \
  zmq/zmqconfig.h\
  zmq/zmqnotificationinterface.h \
  zmq/zmqpublishnotifier.h \
  usbdevice/usbdevice.h \
  usbdevice/rpcusbdevice.h


obj/build.h: FORCE
	@$(MKDIR_P) $(builddir)/obj
	@$(top_srcdir)/share/genbuild.sh $(abs_top_builddir)/src/obj/build.h \
	  $(abs_top_srcdir)
libparticl_util_a-clientversion.$(OBJEXT): obj/build.h

# server: shared between bitcoind and bitcoin-qt
libparticl_server_a_CPPFLAGS = $(AM_CPPFLAGS) $(BITCOIN_INCLUDES) $(MINIUPNPC_CPPFLAGS) $(EVENT_CFLAGS) $(EVENT_PTHREADS_CFLAGS)
libparticl_server_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libparticl_server_a_SOURCES = \
  addrman.cpp \
  addrdb.cpp \
  addressindex.cpp \
  bloom.cpp \
  blockencodings.cpp \
  chain.cpp \
  checkpoints.cpp \
  consensus/tx_verify.cpp \
  httprpc.cpp \
  httpserver.cpp \
  index/base.cpp \
  index/txindex.cpp \
  init.cpp \
  dbwrapper.cpp \
  merkleblock.cpp \
  miner.cpp \
  net.cpp \
  net_processing.cpp \
  noui.cpp \
  policy/fees.cpp \
  policy/policy.cpp \
  policy/rbf.cpp \
  pow.cpp \
  pos/kernel.cpp \
  rest.cpp \
  rpc/anon.cpp \
  rpc/mnemonic.cpp \
  rpc/blockchain.cpp \
  rpc/mining.cpp \
  rpc/misc.cpp \
  rpc/net.cpp \
  rpc/rawtransaction.cpp \
  rpc/server.cpp \
  rpc/client.cpp \
  rpc/rpcutil.cpp \
  script/sigcache.cpp \
  timedata.cpp \
  torcontrol.cpp \
  txdb.cpp \
  txmempool.cpp \
  ui_interface.cpp \
  validation.cpp \
  validationinterface.cpp \
  versionbits.cpp \
  $(BITCOIN_CORE_H)

if ENABLE_ZMQ
libparticl_zmq_a_CPPFLAGS = $(BITCOIN_INCLUDES) $(ZMQ_CFLAGS)
libparticl_zmq_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libparticl_zmq_a_SOURCES = \
  zmq/zmqabstractnotifier.cpp \
  zmq/zmqnotificationinterface.cpp \
  zmq/zmqpublishnotifier.cpp
endif

if ENABLE_USBDEVICE
libparticl_usbdevice_a_CPPFLAGS = $(BITCOIN_INCLUDES) $(AM_CPPFLAGS)
libparticl_usbdevice_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libparticl_usbdevice_a_CFLAGS = -DHAVE_HIDAPI $(AM_CFLAGS)
libparticl_usbdevice_a_SOURCES = \
  usbdevice/ledger/btchipApdu.h \
  usbdevice/ledger/ledgerLayer.h \
  usbdevice/ledger/ledgerLayer.c \
  usbdevice/ledger/dongleCommHidHidapi.h \
  usbdevice/ledger/dongleCommHidHidapi.c \
  usbdevice/usbdevice.cpp \
  usbdevice/debugdevice.h \
  usbdevice/debugdevice.cpp \
  usbdevice/ledgerdevice.h \
  usbdevice/ledgerdevice.cpp \
  usbdevice/rpcusbdevice.cpp
endif

# wallet: shared between bitcoind and bitcoin-qt, but only linked
# when wallet enabled
libparticl_wallet_a_CPPFLAGS = $(AM_CPPFLAGS) $(BITCOIN_INCLUDES)
libparticl_wallet_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libparticl_wallet_a_SOURCES = \
  interfaces/wallet.cpp \
  wallet/crypter.cpp \
  wallet/db.cpp \
  wallet/feebumper.cpp \
  wallet/fees.cpp \
  wallet/init.cpp \
  wallet/rpcdump.cpp \
  wallet/rpcwallet.cpp \
  wallet/wallet.cpp \
  wallet/walletdb.cpp \
  wallet/hdwallet.cpp \
  wallet/hdwalletdb.cpp \
  wallet/rpchdwallet.cpp \
  blind.cpp \
  key/stealth.cpp \
  pos/miner.cpp \
  policy/rbf.cpp \
  wallet/walletutil.cpp \
  wallet/coinselection.cpp \
  $(BITCOIN_CORE_H)

# crypto primitives library
crypto_libparticl_crypto_base_a_CPPFLAGS = $(AM_CPPFLAGS)
crypto_libparticl_crypto_base_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
crypto_libparticl_crypto_base_a_SOURCES = \
  crypto/aes.cpp \
  crypto/aes.h \
  crypto/chacha20.h \
  crypto/chacha20.cpp \
  crypto/common.h \
  crypto/hmac_sha256.cpp \
  crypto/hmac_sha256.h \
  crypto/hmac_sha512.cpp \
  crypto/hmac_sha512.h \
  crypto/ripemd160.cpp \
  crypto/ripemd160.h \
  crypto/sha1.cpp \
  crypto/sha1.h \
  crypto/sha256.cpp \
  crypto/sha256.h \
  crypto/sha512.cpp \
  crypto/sha512.h

if USE_ASM
crypto_libparticl_crypto_base_a_SOURCES += crypto/sha256_sse4.cpp
endif

crypto_libparticl_crypto_sse41_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
crypto_libparticl_crypto_sse41_a_CPPFLAGS = $(AM_CPPFLAGS)
crypto_libparticl_crypto_sse41_a_CXXFLAGS += $(SSE41_CXXFLAGS)
crypto_libparticl_crypto_sse41_a_CPPFLAGS += -DENABLE_SSE41
crypto_libparticl_crypto_sse41_a_SOURCES = crypto/sha256_sse41.cpp

crypto_libparticl_crypto_avx2_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
crypto_libparticl_crypto_avx2_a_CPPFLAGS = $(AM_CPPFLAGS)
crypto_libparticl_crypto_avx2_a_CXXFLAGS += $(AVX2_CXXFLAGS)
crypto_libparticl_crypto_avx2_a_CPPFLAGS += -DENABLE_AVX2
crypto_libparticl_crypto_avx2_a_SOURCES = crypto/sha256_avx2.cpp

# consensus: shared between all executables that validate any consensus rules.
libparticl_consensus_a_CPPFLAGS = $(AM_CPPFLAGS) $(BITCOIN_INCLUDES)
libparticl_consensus_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libparticl_consensus_a_SOURCES = \
  amount.h \
  arith_uint256.cpp \
  arith_uint256.h \
  consensus/merkle.cpp \
  consensus/merkle.h \
  consensus/params.h \
  consensus/validation.h \
  hash.cpp \
  hash.h \
  prevector.h \
  primitives/block.cpp \
  primitives/block.h \
  primitives/transaction.cpp \
  primitives/transaction.h \
  pubkey.cpp \
  pubkey.h \
  script/bitcoinconsensus.cpp \
  script/interpreter.cpp \
  script/interpreter.h \
  script/script.cpp \
  script/script.h \
  script/script_error.cpp \
  script/script_error.h \
  serialize.h \
  span.h \
  tinyformat.h \
  uint256.cpp \
  uint256.h \
  utilstrencodings.cpp \
  utilstrencodings.h \
  version.h

# common: shared between bitcoind, and bitcoin-qt and non-server tools
libparticl_common_a_CPPFLAGS = $(AM_CPPFLAGS) $(BITCOIN_INCLUDES)
libparticl_common_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libparticl_common_a_SOURCES = \
  base58.cpp \
  bech32.cpp \
  chainparams.cpp \
  coins.cpp \
  compressor.cpp \
  core_read.cpp \
  core_write.cpp \
  anon.cpp \
  blind.cpp \
  key.cpp \
  key/keyutil.cpp \
  key/extkey.cpp \
  key/stealth.cpp \
  smsg/crypter.cpp \
  key/mnemonic.cpp \
  unilib/uninorms.cpp \
  unilib/utf8.cpp \
  key_io.cpp \
  keystore.cpp \
  netaddress.cpp \
  netbase.cpp \
  policy/feerate.cpp \
  protocol.cpp \
  scheduler.cpp \
  script/ismine.cpp \
  script/sign.cpp \
  script/standard.cpp \
  warnings.cpp \
  $(BITCOIN_CORE_H)

# util: shared between all executables.
# This library *must* be included to make sure that the glibc
# backward-compatibility objects and their sanity checks are linked.
libparticl_util_a_CPPFLAGS = $(AM_CPPFLAGS) $(BITCOIN_INCLUDES)
libparticl_util_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libparticl_util_a_SOURCES = \
  support/lockedpool.cpp \
  chainparamsbase.cpp \
  clientversion.cpp \
  compat/glibc_sanity.cpp \
  compat/glibcxx_sanity.cpp \
  compat/strnlen.cpp \
  fs.cpp \
  interfaces/handler.cpp \
  interfaces/node.cpp \
  logging.cpp \
  random.cpp \
  rpc/protocol.cpp \
  rpc/util.cpp \
  support/cleanse.cpp \
  sync.cpp \
  threadinterrupt.cpp \
  util.cpp \
  utilmoneystr.cpp \
  utilstrencodings.cpp \
  utiltime.cpp \
  $(BITCOIN_CORE_H)


libparticl_smsg_a_CPPFLAGS = $(AM_CPPFLAGS) $(BITCOIN_INCLUDES)
libparticl_smsg_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libparticl_smsg_a_SOURCES = \
  lz4/lz4.h \
  lz4/lz4.c \
  xxhash/xxhash.h \
  xxhash/xxhash.c \
  interfaces/handler.cpp \
  smsg/crypter.cpp \
  smsg/keystore.h \
  smsg/keystore.cpp \
  smsg/db.cpp \
  smsg/smessage.cpp \
  smsg/rpcsmessage.cpp


if GLIBC_BACK_COMPAT
libparticl_util_a_SOURCES += compat/glibc_compat.cpp
endif

# cli: shared between bitcoin-cli and bitcoin-qt
libparticl_cli_a_CPPFLAGS = $(AM_CPPFLAGS) $(BITCOIN_INCLUDES)
libparticl_cli_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libparticl_cli_a_SOURCES = \
  rpc/client.cpp \
  $(BITCOIN_CORE_H)


nodist_libparticl_util_a_SOURCES = $(srcdir)/obj/build.h
#

# bitcoind binary #
particld_SOURCES = bitcoind.cpp
particld_CPPFLAGS = $(AM_CPPFLAGS) $(BITCOIN_INCLUDES)
particld_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
particld_LDFLAGS = $(RELDFLAGS) $(AM_LDFLAGS) $(LIBTOOL_APP_LDFLAGS)

if TARGET_WINDOWS
particld_SOURCES += bitcoind-res.rc
endif

particld_LDADD = \
  $(LIBPARTICL_SERVER) \
  $(LIBPARTICL_WALLET) \
  $(LIBPARTICL_COMMON) \
  $(LIBUNIVALUE) \
  $(LIBPARTICL_UTIL) \
  $(LIBPARTICL_ZMQ) \
  $(LIBPARTICL_CONSENSUS) \
  $(LIBPARTICL_CRYPTO) \
  $(LIBLEVELDB) \
  $(LIBLEVELDB_SSE42) \
  $(LIBMEMENV) \
  $(LIBSECP256K1) \
  $(LIBPARTICL_SMSG) \
  $(LIBPARTICL_USBDEVICE)

particld_LDADD += $(BOOST_LIBS) $(BDB_LIBS) $(SSL_LIBS) $(CRYPTO_LIBS) $(MINIUPNPC_LIBS) $(EVENT_PTHREADS_LIBS) $(EVENT_LIBS) $(ZMQ_LIBS) $(USB_LIBS)

# bitcoin-cli binary #
#$(BITCOIN_CLI_NAME)_SOURCES = bitcoin-cli.cpp
particl_cli_SOURCES = bitcoin-cli.cpp
particl_cli_CPPFLAGS = $(AM_CPPFLAGS) $(BITCOIN_INCLUDES) $(EVENT_CFLAGS)
particl_cli_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
particl_cli_LDFLAGS = $(RELDFLAGS) $(AM_LDFLAGS) $(LIBTOOL_APP_LDFLAGS)

if TARGET_WINDOWS
particl_cli_SOURCES += bitcoin-cli-res.rc
endif

particl_cli_LDADD = \
  $(LIBPARTICL_CLI) \
  $(LIBUNIVALUE) \
  $(LIBPARTICL_UTIL) \
  $(LIBPARTICL_CRYPTO)

particl_cli_LDADD += $(BOOST_LIBS) $(SSL_LIBS) $(CRYPTO_LIBS) $(EVENT_LIBS)
#

# bitcoin-tx binary #
particl_tx_SOURCES = bitcoin-tx.cpp
particl_tx_CPPFLAGS = $(AM_CPPFLAGS) $(BITCOIN_INCLUDES)
particl_tx_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
particl_tx_LDFLAGS = $(RELDFLAGS) $(AM_LDFLAGS) $(LIBTOOL_APP_LDFLAGS)

if TARGET_WINDOWS
particl_tx_SOURCES += bitcoin-tx-res.rc
endif

particl_tx_LDADD = \
  $(LIBUNIVALUE) \
  $(LIBPARTICL_COMMON) \
  $(LIBPARTICL_UTIL) \
  $(LIBPARTICL_CONSENSUS) \
  $(LIBPARTICL_CRYPTO) \
  $(LIBSECP256K1)

particl_tx_LDADD += $(BOOST_LIBS) $(CRYPTO_LIBS)
#

# bitcoinconsensus library #
if BUILD_BITCOIN_LIBS
include_HEADERS = script/particlconsensus.h
libparticlconsensus_la_SOURCES = $(crypto_libparticl_crypto_base_a_SOURCES) $(libparticl_consensus_a_SOURCES)

if GLIBC_BACK_COMPAT
  libparticlconsensus_la_SOURCES += compat/glibc_compat.cpp
endif

libparticlconsensus_la_LDFLAGS = $(AM_LDFLAGS) -no-undefined $(RELDFLAGS)
libparticlconsensus_la_LIBADD = $(LIBSECP256K1) $(BOOST_LIBS)
libparticlconsensus_la_CPPFLAGS = $(AM_CPPFLAGS) -I$(builddir)/obj -I$(srcdir)/secp256k1/include -DBUILD_BITCOIN_INTERNAL
libparticlconsensus_la_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)

endif
#

CTAES_DIST =  crypto/ctaes/bench.c
CTAES_DIST += crypto/ctaes/ctaes.c
CTAES_DIST += crypto/ctaes/ctaes.h
CTAES_DIST += crypto/ctaes/README.md
CTAES_DIST += crypto/ctaes/test.c

CLEANFILES = $(EXTRA_LIBRARIES)

CLEANFILES += *.gcda *.gcno
CLEANFILES += compat/*.gcda compat/*.gcno
CLEANFILES += consensus/*.gcda consensus/*.gcno
CLEANFILES += crypto/*.gcda crypto/*.gcno
CLEANFILES += policy/*.gcda policy/*.gcno
CLEANFILES += primitives/*.gcda primitives/*.gcno
CLEANFILES += script/*.gcda script/*.gcno
CLEANFILES += support/*.gcda support/*.gcno
CLEANFILES += univalue/*.gcda univalue/*.gcno
CLEANFILES += wallet/*.gcda wallet/*.gcno
CLEANFILES += wallet/test/*.gcda wallet/test/*.gcno
CLEANFILES += zmq/*.gcda zmq/*.gcno
CLEANFILES += obj/build.h

EXTRA_DIST = $(CTAES_DIST)


config/bitcoin-config.h: config/stamp-h1
	@$(MAKE) -C $(top_builddir) $(subdir)/$(@)
config/stamp-h1: $(top_srcdir)/$(subdir)/config/bitcoin-config.h.in $(top_builddir)/config.status
	$(AM_V_at)$(MAKE) -C $(top_builddir) $(subdir)/$(@)
$(top_srcdir)/$(subdir)/config/bitcoin-config.h.in:  $(am__configure_deps)
	$(AM_V_at)$(MAKE) -C $(top_srcdir) $(subdir)/config/bitcoin-config.h.in

clean-local:
	-$(MAKE) -C secp256k1 clean
	-$(MAKE) -C univalue clean
	-rm -f leveldb/*/*.gcda leveldb/*/*.gcno leveldb/helpers/memenv/*.gcda leveldb/helpers/memenv/*.gcno
	-rm -f config.h
	-rm -rf test/__pycache__

.rc.o:
	@test -f $(WINDRES)
	## FIXME: How to get the appropriate modulename_CPPFLAGS in here?
	$(AM_V_GEN) $(WINDRES) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(CPPFLAGS) -DWINDRES_PREPROC -i $< -o $@

check-symbols: $(bin_PROGRAMS)
if GLIBC_BACK_COMPAT
	@echo "Checking glibc back compat..."
	$(AM_V_at) READELF=$(READELF) CPPFILT=$(CPPFILT) $(top_srcdir)/contrib/devtools/symbol-check.py < $(bin_PROGRAMS)
endif

check-security: $(bin_PROGRAMS)
if HARDEN
	@echo "Checking binary security..."
	$(AM_V_at) READELF=$(READELF) OBJDUMP=$(OBJDUMP) $(top_srcdir)/contrib/devtools/security-check.py < $(bin_PROGRAMS)
endif

%.pb.cc %.pb.h: %.proto
	@test -f $(PROTOC)
	$(AM_V_GEN) $(PROTOC) --cpp_out=$(@D) --proto_path=$(<D) $<

if EMBEDDED_LEVELDB
include Makefile.leveldb.include
endif

if ENABLE_TESTS
include Makefile.test.include
endif

if ENABLE_BENCH
include Makefile.bench.include
endif

if ENABLE_QT
include Makefile.qt.include
endif

if ENABLE_QT_TESTS
include Makefile.qttest.include
endif
