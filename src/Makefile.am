# Copyright (c) 2013-2016 The Bitcoin Core developers
# Copyright (c) 2014-2018 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# Pattern rule to print variables, e.g. make print-top_srcdir
print-%: FORCE
	@echo '$*'='$($*)'

DIST_SUBDIRS = secp256k1

AM_LDFLAGS = $(LIBTOOL_LDFLAGS) $(HARDENED_LDFLAGS) $(GPROF_LDFLAGS) $(SANITIZER_LDFLAGS) $(CORE_LDFLAGS)
AM_CFLAGS = $(DEBUG_CFLAGS)
AM_CXXFLAGS = $(DEBUG_CXXFLAGS) $(HARDENED_CXXFLAGS) $(WARN_CXXFLAGS) $(NOWARN_CXXFLAGS) $(ERROR_CXXFLAGS) $(GPROF_CXXFLAGS) $(SANITIZER_CXXFLAGS) $(CORE_CXXFLAGS)
AM_OBJCXXFLAGS = $(AM_CXXFLAGS)
AM_CPPFLAGS = $(DEBUG_CPPFLAGS) $(HARDENED_CPPFLAGS) $(CORE_CPPFLAGS)
AM_LIBTOOLFLAGS = --preserve-dup-deps
PTHREAD_FLAGS = $(PTHREAD_CFLAGS) $(PTHREAD_LIBS)
EXTRA_LIBRARIES =

lib_LTLIBRARIES = $(LIBBITCOINCONSENSUS)
noinst_LTLIBRARIES =

bin_PROGRAMS =
noinst_PROGRAMS =
check_PROGRAMS =
TESTS =
BENCHMARKS =

if ENABLE_STACKTRACES
if ENABLE_CRASH_HOOKS
if CRASH_HOOKS_WRAPPED_CXX_ABI
# Wrap internal C++ ABI's so that we can attach stacktraces to exceptions
LDFLAGS_WRAP_EXCEPTIONS = -Wl,-wrap,__cxa_allocate_exception -Wl,-wrap,__cxa_free_exception
if TARGET_WINDOWS
LDFLAGS_WRAP_EXCEPTIONS += -Wl,-wrap,_assert -Wl,-wrap,_wassert
else
LDFLAGS_WRAP_EXCEPTIONS += -Wl,-wrap,__assert_fail
endif
endif
endif

if TARGET_WINDOWS
BACKTRACE_LIB = -ldbghelp -lbacktrace
else
BACKTRACE_LIB = -lbacktrace
endif
endif #ENABLE_STACKTRACES

BITCOIN_INCLUDES=-I$(builddir) -I$(srcdir)/$(MINISKETCH_INCLUDE_DIR_INT) -I$(srcdir)/secp256k1/include -I$(srcdir)/$(UNIVALUE_INCLUDE_DIR_INT) $(LEVELDB_CPPFLAGS)
BITCOIN_INCLUDES+=-isystem$(srcdir)/dashbls/include -isystem$(srcdir)/dashbls/depends/relic/include -isystem$(srcdir)/dashbls/depends/minialloc/include
BITCOIN_INCLUDES+=-isystem$(srcdir)/immer

LIBBITCOIN_NODE=libbitcoin_node.a
LIBBITCOIN_COMMON=libbitcoin_common.a
LIBBITCOIN_CONSENSUS=libbitcoin_consensus.a
LIBBITCOIN_CLI=libbitcoin_cli.a
LIBBITCOIN_UTIL=libbitcoin_util.a
LIBBITCOIN_CRYPTO_BASE=crypto/libbitcoin_crypto_base.la
LIBBITCOINQT=qt/libbitcoinqt.a
LIBDASHBLS=dashbls/libdashbls.la
LIBSECP256K1=secp256k1/libsecp256k1.la

if ENABLE_ZMQ
LIBBITCOIN_ZMQ=libbitcoin_zmq.a
endif
if BUILD_BITCOIN_LIBS
LIBBITCOINCONSENSUS=libdashconsensus.la
endif
if ENABLE_WALLET
LIBBITCOIN_WALLET=libbitcoin_wallet.a
LIBBITCOIN_WALLET_TOOL=libbitcoin_wallet_tool.a
endif

LIBBITCOIN_CRYPTO = $(LIBBITCOIN_CRYPTO_BASE)
LIBBITCOIN_CRYPTO_SPH = crypto/libbitcoin_crypto_sph.la
LIBBITCOIN_CRYPTO += $(LIBBITCOIN_CRYPTO_SPH)
if ENABLE_SSE41
LIBBITCOIN_CRYPTO_SSE41 = crypto/libbitcoin_crypto_sse41.la
LIBBITCOIN_CRYPTO += $(LIBBITCOIN_CRYPTO_SSE41)
if ENABLE_X86_SHANI
LIBBITCOIN_CRYPTO_X86_SHANI = crypto/libbitcoin_crypto_x86_shani.la
LIBBITCOIN_CRYPTO += $(LIBBITCOIN_CRYPTO_X86_SHANI)
endif
endif
if ENABLE_AVX2
LIBBITCOIN_CRYPTO_AVX2 = crypto/libbitcoin_crypto_avx2.la
LIBBITCOIN_CRYPTO += $(LIBBITCOIN_CRYPTO_AVX2)
endif
if ENABLE_ARM_SHANI
LIBBITCOIN_CRYPTO_ARM_SHANI = crypto/libbitcoin_crypto_arm_shani.la
LIBBITCOIN_CRYPTO += $(LIBBITCOIN_CRYPTO_ARM_SHANI)
endif
noinst_LTLIBRARIES += $(LIBBITCOIN_CRYPTO)

$(LIBDASHBLS):
	$(AM_V_at)$(MAKE) $(AM_MAKEFLAGS) -C $(@D)

$(LIBSECP256K1): $(wildcard secp256k1/src/*.h) $(wildcard secp256k1/src/*.c) $(wildcard secp256k1/include/*)
	$(AM_V_at)$(MAKE) $(AM_MAKEFLAGS) -C $(@D) $(@F)

# Make is not made aware of per-object dependencies to avoid limiting building parallelization
# But to build the less dependent modules first, we manually select their order here:
EXTRA_LIBRARIES += \
  $(LIBBITCOIN_UTIL) \
  $(LIBBITCOIN_COMMON) \
  $(LIBBITCOIN_CONSENSUS) \
  $(LIBBITCOIN_NODE) \
  $(LIBBITCOIN_CLI) \
  $(LIBBITCOIN_IPC) \
  $(LIBBITCOIN_WALLET) \
  $(LIBBITCOIN_WALLET_TOOL) \
  $(LIBBITCOIN_ZMQ)

if BUILD_BITCOIND
  bin_PROGRAMS += dashd
endif

if BUILD_BITCOIN_NODE
  bin_PROGRAMS += dash-node
endif

if BUILD_BITCOIN_CLI
  bin_PROGRAMS += dash-cli
endif
if BUILD_BITCOIN_TX
  bin_PROGRAMS += dash-tx
endif
if ENABLE_WALLET
if BUILD_BITCOIN_WALLET
  bin_PROGRAMS += dash-wallet
endif
endif

.PHONY: FORCE check-symbols check-security
# dash core #
BITCOIN_CORE_H = \
  addrdb.h \
  addressindex.h \
  spentindex.h \
  addrman.h \
  addrman_impl.h \
  attributes.h \
  banman.h \
  base58.h \
  batchedlogger.h \
  bech32.h \
  bip324.h \
  blockencodings.h \
  common/bloom.h \
  cachemap.h \
  cachemultimap.h \
  blockfilter.h \
  chain.h \
  chainlock/chainlock.h \
  chainlock/clsig.h \
  chainlock/signing.h \
  chainparams.h \
  chainparamsbase.h \
  chainparamsseeds.h \
  checkqueue.h \
  clientversion.h \
  coinjoin/coinjoin.h \
  coinjoin/client.h \
  coinjoin/common.h \
  coinjoin/context.h \
  coinjoin/options.h \
  coinjoin/server.h \
  coinjoin/util.h \
  coins.h \
  common/bloom.h \
  compat/assumptions.h \
  compat/byteswap.h \
  compat/compat.h \
  compat/cpuid.h \
  compat/endian.h \
  compressor.h \
  context.h \
  consensus/consensus.h \
  consensus/tx_check.h \
  consensus/tx_verify.h \
  core_io.h \
  core_memusage.h \
  cuckoocache.h \
  ctpl_stl.h \
  cxxtimer.hpp \
  dbwrapper.h \
  deploymentinfo.h \
  deploymentstatus.h \
  evo/assetlocktx.h \
  evo/dmn_types.h \
  evo/cbtx.h \
  evo/chainhelper.h \
  evo/creditpool.h \
  evo/deterministicmns.h \
  evo/dmnstate.h \
  evo/evodb.h \
  evo/mnauth.h \
  evo/mnhftx.h \
  evo/netinfo.h \
  evo/providertx.h \
  evo/simplifiedmns.h \
  evo/smldiff.h \
  evo/specialtx.h \
  evo/specialtx_filter.h \
  evo/specialtxman.h \
  dsnotificationinterface.h \
  governance/governance.h \
  governance/classes.h \
  governance/common.h \
  governance/exceptions.h \
  governance/object.h \
  governance/validators.h \
  governance/vote.h \
  governance/votedb.h \
  gsl/assert.h \
  gsl/pointers.h \
  flat-database.h \
  flatfile.h \
  fs.h \
  httprpc.h \
  httpserver.h \
  i2p.h \
  index/base.h \
  index/blockfilterindex.h \
  index/coinstatsindex.h \
  index/disktxpos.h \
  index/txindex.h \
  indirectmap.h \
  init.h \
  init/common.h \
  interfaces/chain.h \
  interfaces/coinjoin.h \
  interfaces/echo.h \
  interfaces/handler.h \
  interfaces/init.h \
  interfaces/ipc.h \
  interfaces/node.h \
  interfaces/wallet.h \
  instantsend/db.h \
  instantsend/instantsend.h \
  instantsend/lock.h \
  instantsend/signing.h \
  key.h \
  key_io.h \
  limitedmap.h \
  llmq/blockprocessor.h \
  llmq/commitment.h \
  llmq/context.h \
  llmq/debug.h \
  llmq/dkgsession.h \
  llmq/dkgsessionhandler.h \
  llmq/dkgsessionmgr.h \
  llmq/ehf_signals.h \
  llmq/options.h \
  llmq/params.h \
  llmq/quorums.h \
  llmq/signhash.h \
  llmq/signing.h \
  llmq/signing_shares.h \
  llmq/snapshot.h \
  llmq/utils.h \
  logging.h \
  logging/timer.h \
  mapport.h \
  masternode/active/context.h \
  masternode/active/notificationinterface.h \
  masternode/node.h \
  masternode/meta.h \
  masternode/payments.h \
  masternode/sync.h \
  masternode/utils.h \
  memusage.h \
  merkleblock.h \
  messagesigner.h \
  net.h \
  net_permissions.h \
  net_processing.h \
  net_types.h \
  netaddress.h \
  netbase.h \
  netfulfilledman.h \
  netgroup.h \
  netmessagemaker.h \
  node/blockstorage.h \
  node/caches.h \
  node/chainstate.h \
  node/coin.h \
  node/coinstats.h \
  node/connection_types.h \
  node/context.h \
  node/eviction.h \
  node/miner.h \
  node/minisketchwrapper.h \
  node/psbt.h \
  node/transaction.h \
  node/txreconciliation.h \
  node/interface_ui.h \
  node/utxo_snapshot.h \
  noui.h \
  outputtype.h \
  policy/feerate.h \
  policy/fees.h \
  policy/packages.h \
  policy/policy.h \
  policy/settings.h \
  pow.h \
  protocol.h \
  psbt.h \
  random.h \
  randomenv.h \
  rest.h \
  rpc/blockchain.h \
  rpc/client.h \
  rpc/evo_util.h \
  rpc/index_util.h \
  rpc/mempool.h \
  rpc/mining.h \
  rpc/protocol.h \
  rpc/rawtransaction_util.h \
  rpc/register.h \
  rpc/request.h \
  rpc/server.h \
  rpc/server_util.h \
  rpc/util.h \
  saltedhasher.h \
  scheduler.h \
  script/descriptor.h \
  script/keyorigin.h \
  script/sigcache.h \
  script/sign.h \
  script/signingprovider.h \
  script/standard.h \
  shutdown.h \
  source_location.h \
  spork.h \
  stacktraces.h \
  streams.h \
  stats/client.h \
  stats/rawsender.h \
  support/allocators/mt_pooled_secure.h \
  support/allocators/pool.h \
  support/allocators/pooled_secure.h \
  support/allocators/secure.h \
  support/allocators/zeroafterfree.h \
  support/cleanse.h \
  support/events.h \
  support/lockedpool.h \
  sync.h \
  timestampindex.h \
  threadsafety.h \
  timedata.h \
  torcontrol.h \
  txdb.h \
  txmempool.h \
  txorphanage.h \
  undo.h \
  unordered_lru_cache.h \
  util/bip32.h \
  util/bytevectorhash.h \
  util/check.h \
  util/edge.h \
  util/enumerate.h \
  util/epochguard.h \
  util/error.h \
  util/fastrange.h \
  util/fees.h \
  util/golombrice.h \
  util/hasher.h \
  util/hash_type.h \
  util/irange.h \
  util/asmap.h \
  util/getuniquepath.h \
  util/macros.h \
  util/message.h \
  util/moneystr.h \
  util/overflow.h \
  util/overloaded.h \
  util/pointer.h \
  util/ranges.h \
  util/readwritefile.h \
  util/result.h \
  util/underlying.h \
  util/serfloat.h \
  util/settings.h \
  util/ranges_set.h \
  util/sock.h \
  util/string.h \
  util/spanparsing.h \
  util/syserror.h \
  util/system.h \
  util/time.h \
  util/thread.h \
  util/threadinterrupt.h \
  util/threadnames.h \
  util/tokenpipe.h \
  util/trace.h \
  util/translation.h \
  util/types.h \
  util/ui_change_type.h \
  util/url.h \
  util/vector.h \
  util/wpipe.h \
  validation.h \
  validationinterface.h \
  versionbits.h \
  wallet/bdb.h \
  wallet/bip39.h \
  wallet/bip39_english.h \
  wallet/coincontrol.h \
  wallet/coinjoin.h \
  wallet/coinselection.h \
  wallet/context.h \
  wallet/crypter.h \
  wallet/db.h \
  wallet/dump.h \
  wallet/fees.h \
  wallet/hdchain.h \
  wallet/ismine.h \
  wallet/load.h \
  wallet/receive.h \
  wallet/rpc/util.h \
  wallet/rpc/wallet.h \
  wallet/salvage.h \
  wallet/scriptpubkeyman.h \
  wallet/spend.h \
  wallet/sqlite.h \
  wallet/transaction.h \
  wallet/wallet.h \
  wallet/walletdb.h \
  wallet/wallettool.h \
  wallet/walletutil.h \
  walletinitinterface.h \
  warnings.h \
  zmq/zmqabstractnotifier.h \
  zmq/zmqnotificationinterface.h \
  zmq/zmqpublishnotifier.h \
  zmq/zmqrpc.h \
  zmq/zmqutil.h


obj/build.h: FORCE
	@$(MKDIR_P) $(builddir)/obj
	$(AM_V_GEN) $(top_srcdir)/share/genbuild.sh "$(abs_top_builddir)/src/obj/build.h" \
	  "$(abs_top_srcdir)"
libbitcoin_util_a-clientversion.$(OBJEXT): obj/build.h


# server: shared between dashd and dash-qt
# Contains code accessing mempool and chain state that is meant to be separated
# from wallet and gui code (see node/README.md). Shared code should go in
# libbitcoin_common or libbitcoin_util libraries, instead.
libbitcoin_node_a_CPPFLAGS = $(AM_CPPFLAGS) $(BITCOIN_INCLUDES) $(BOOST_CPPFLAGS) $(MINIUPNPC_CPPFLAGS) $(NATPMP_CPPFLAGS) $(EVENT_CFLAGS) $(EVENT_PTHREADS_CFLAGS)
libbitcoin_node_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libbitcoin_node_a_SOURCES = \
  addrdb.cpp \
  addressindex.cpp \
  addrman.cpp \
  banman.cpp \
  batchedlogger.cpp \
  bip324.cpp \
  blockencodings.cpp \
  blockfilter.cpp \
  chain.cpp \
  chainlock/chainlock.cpp \
  chainlock/clsig.cpp \
  chainlock/signing.cpp \
  coinjoin/coinjoin.cpp \
  coinjoin/context.cpp \
  coinjoin/server.cpp \
  consensus/tx_verify.cpp \
  dbwrapper.cpp \
  deploymentstatus.cpp \
  dsnotificationinterface.cpp \
  evo/assetlocktx.cpp \
  evo/cbtx.cpp \
  evo/creditpool.cpp \
  evo/chainhelper.cpp \
  evo/deterministicmns.cpp \
  evo/dmnstate.cpp \
  evo/evodb.cpp \
  evo/mnauth.cpp \
  evo/mnhftx.cpp \
  evo/providertx.cpp \
  evo/simplifiedmns.cpp \
  evo/smldiff.cpp \
  evo/specialtx.cpp \
  evo/specialtx_filter.cpp \
  evo/specialtxman.cpp \
  flatfile.cpp \
  governance/classes.cpp \
  governance/exceptions.cpp \
  governance/governance.cpp \
  governance/object.cpp \
  governance/validators.cpp \
  governance/vote.cpp \
  governance/votedb.cpp \
  gsl/assert.cpp \
  httprpc.cpp \
  httpserver.cpp \
  i2p.cpp \
  index/base.cpp \
  index/blockfilterindex.cpp \
  index/coinstatsindex.cpp \
  index/txindex.cpp \
  init.cpp \
  instantsend/db.cpp \
  instantsend/instantsend.cpp \
  instantsend/lock.cpp \
  instantsend/signing.cpp \
  llmq/blockprocessor.cpp \
  llmq/commitment.cpp \
  llmq/context.cpp \
  llmq/debug.cpp \
  llmq/dkgsession.cpp \
  llmq/dkgsessionhandler.cpp \
  llmq/dkgsessionmgr.cpp \
  llmq/ehf_signals.cpp \
  llmq/options.cpp \
  llmq/quorums.cpp \
  llmq/signhash.cpp \
  llmq/signing.cpp \
  llmq/signing_shares.cpp \
  llmq/snapshot.cpp \
  llmq/utils.cpp \
  mapport.cpp \
  masternode/active/context.cpp \
  masternode/active/notificationinterface.cpp \
  masternode/node.cpp \
  masternode/meta.cpp \
  masternode/payments.cpp \
  masternode/sync.cpp \
  masternode/utils.cpp \
  net.cpp \
  netfulfilledman.cpp \
  netgroup.cpp \
  net_processing.cpp \
  node/blockstorage.cpp \
  node/caches.cpp \
  node/chainstate.cpp \
  node/coin.cpp \
  node/coinstats.cpp \
  node/connection_types.cpp \
  node/context.cpp \
  node/eviction.cpp \
  node/interfaces.cpp \
  node/miner.cpp \
  node/minisketchwrapper.cpp \
  node/psbt.cpp \
  node/transaction.cpp \
  node/txreconciliation.cpp \
  node/interface_ui.cpp \
  noui.cpp \
  policy/fees.cpp \
  policy/packages.cpp \
  policy/policy.cpp \
  policy/settings.cpp \
  pow.cpp \
  rest.cpp \
  rpc/blockchain.cpp \
  rpc/coinjoin.cpp \
  rpc/evo.cpp \
  rpc/fees.cpp \
  rpc/index_util.cpp \
  rpc/masternode.cpp \
  rpc/mempool.cpp \
  rpc/governance.cpp \
  rpc/mining.cpp \
  rpc/node.cpp \
  rpc/net.cpp \
  rpc/output_script.cpp \
  rpc/quorums.cpp \
  rpc/rawtransaction.cpp \
  rpc/server.cpp \
  rpc/server_util.cpp \
  rpc/signmessage.cpp \
  rpc/txoutproof.cpp \
  script/sigcache.cpp \
  shutdown.cpp \
  spork.cpp \
  stats/client.cpp \
  stats/rawsender.cpp \
  timedata.cpp \
  torcontrol.cpp \
  txdb.cpp \
  txmempool.cpp \
  txorphanage.cpp \
  validation.cpp \
  validationinterface.cpp \
  versionbits.cpp \
  $(BITCOIN_CORE_H)

if ENABLE_WALLET
libbitcoin_node_a_SOURCES += wallet/init.cpp
libbitcoin_node_a_CPPFLAGS += $(BDB_CPPFLAGS)
endif
if !ENABLE_WALLET
libbitcoin_node_a_SOURCES += dummywallet.cpp
endif

if ENABLE_ZMQ
libbitcoin_zmq_a_CPPFLAGS = $(AM_CPPFLAGS) $(BITCOIN_INCLUDES) $(BOOST_CPPFLAGS) $(ZMQ_CFLAGS)
libbitcoin_zmq_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libbitcoin_zmq_a_SOURCES = \
  zmq/zmqabstractnotifier.cpp \
  zmq/zmqnotificationinterface.cpp \
  zmq/zmqpublishnotifier.cpp \
  zmq/zmqrpc.cpp \
  zmq/zmqutil.cpp
endif


# wallet: shared between dashd and dash-qt, but only linked
# when wallet enabled
libbitcoin_wallet_a_CPPFLAGS = $(AM_CPPFLAGS) $(BITCOIN_INCLUDES) $(BOOST_CPPFLAGS) $(BDB_CPPFLAGS) $(SQLITE_CFLAGS)
libbitcoin_wallet_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libbitcoin_wallet_a_SOURCES = \
  coinjoin/client.cpp \
  coinjoin/interfaces.cpp \
  coinjoin/util.cpp \
  wallet/bip39.cpp \
  wallet/coinjoin.cpp \
  wallet/coincontrol.cpp \
  wallet/context.cpp \
  wallet/crypter.cpp \
  wallet/db.cpp \
  wallet/dump.cpp \
  wallet/fees.cpp \
  wallet/hdchain.cpp \
  wallet/interfaces.cpp \
  wallet/load.cpp \
  wallet/receive.cpp \
  wallet/rpc/addresses.cpp \
  wallet/rpc/backup.cpp \
  wallet/rpc/coins.cpp \
  wallet/rpc/encrypt.cpp \
  wallet/rpc/spend.cpp \
  wallet/rpc/signmessage.cpp \
  wallet/rpc/transactions.cpp \
  wallet/rpc/util.cpp \
  wallet/rpc/wallet.cpp \
  wallet/scriptpubkeyman.cpp \
  wallet/spend.cpp \
  wallet/transaction.cpp \
  wallet/wallet.cpp \
  wallet/walletdb.cpp \
  wallet/walletutil.cpp \
  wallet/coinselection.cpp \
  $(BITCOIN_CORE_H)

if USE_SQLITE
libbitcoin_wallet_a_SOURCES += wallet/sqlite.cpp
endif
if USE_BDB
libbitcoin_wallet_a_SOURCES += wallet/bdb.cpp wallet/salvage.cpp
endif

libbitcoin_wallet_tool_a_CPPFLAGS = $(AM_CPPFLAGS) $(BITCOIN_INCLUDES) $(BOOST_CPPFLAGS)
libbitcoin_wallet_tool_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libbitcoin_wallet_tool_a_SOURCES = \
  wallet/wallettool.cpp \
  $(BITCOIN_CORE_H)

# crypto primitives library

# crypto_base contains the unspecialized (unoptimized) versions of our
# crypto functions. Functions that require custom compiler flags and/or
# runtime opt-in are omitted.
crypto_libbitcoin_crypto_base_la_CPPFLAGS = $(AM_CPPFLAGS) $(PIC_FLAGS)

# Specify -static in both CXXFLAGS and LDFLAGS so libtool will only build a
# static version of this library. We don't need a dynamic version, and a dynamic
# version can't be used on windows anyway because the library doesn't currently
# export DLL symbols.
crypto_libbitcoin_crypto_base_la_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS) $(PIC_FLAGS) -static
crypto_libbitcoin_crypto_base_la_LDFLAGS = $(AM_LDFLAGS) -static

crypto_libbitcoin_crypto_base_la_SOURCES = \
  crypto/aes.cpp \
  crypto/aes.h \
  crypto/chacha20.h \
  crypto/chacha20.cpp \
  crypto/chacha20poly1305.h \
  crypto/chacha20poly1305.cpp \
  crypto/common.h \
  crypto/hkdf_sha256_32.cpp \
  crypto/hkdf_sha256_32.h \
  crypto/hmac_sha256.cpp \
  crypto/hmac_sha256.h \
  crypto/hmac_sha512.cpp \
  crypto/hmac_sha512.h \
  crypto/muhash.h \
  crypto/muhash.cpp \
  crypto/poly1305.h \
  crypto/poly1305.cpp \
  crypto/pkcs5_pbkdf2_hmac_sha512.cpp \
  crypto/pkcs5_pbkdf2_hmac_sha512.h \
  crypto/ripemd160.cpp \
  crypto/ripemd160.h \
  crypto/sha1.cpp \
  crypto/sha1.h \
  crypto/sha256.cpp \
  crypto/sha256.h \
  crypto/sha256_sse4.cpp \
  crypto/sha3.cpp \
  crypto/sha3.h \
  crypto/sha512.cpp \
  crypto/sha512.h \
  crypto/siphash.cpp \
  crypto/siphash.h

# See explanation for -static in crypto_libbitcoin_crypto_base_la's LDFLAGS and
# CXXFLAGS above
crypto_libbitcoin_crypto_sse41_la_LDFLAGS = $(AM_LDFLAGS) -static
crypto_libbitcoin_crypto_sse41_la_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS) -static
crypto_libbitcoin_crypto_sse41_la_CPPFLAGS = $(AM_CPPFLAGS)
crypto_libbitcoin_crypto_sse41_la_CXXFLAGS += $(SSE41_CXXFLAGS)
crypto_libbitcoin_crypto_sse41_la_CPPFLAGS += -DENABLE_SSE41
crypto_libbitcoin_crypto_sse41_la_SOURCES = crypto/sha256_sse41.cpp

# See explanation for -static in crypto_libbitcoin_crypto_base_la's LDFLAGS and
# CXXFLAGS above
crypto_libbitcoin_crypto_avx2_la_LDFLAGS = $(AM_LDFLAGS) -static
crypto_libbitcoin_crypto_avx2_la_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS) -static
crypto_libbitcoin_crypto_avx2_la_CPPFLAGS = $(AM_CPPFLAGS)
crypto_libbitcoin_crypto_avx2_la_CXXFLAGS += $(AVX2_CXXFLAGS)
crypto_libbitcoin_crypto_avx2_la_CPPFLAGS += -DENABLE_AVX2
crypto_libbitcoin_crypto_avx2_la_SOURCES = crypto/sha256_avx2.cpp

# See explanation for -static in crypto_libbitcoin_crypto_base_la's LDFLAGS and
# CXXFLAGS above
crypto_libbitcoin_crypto_sph_la_LDFLAGS = $(AM_LDFLAGS) -static
crypto_libbitcoin_crypto_sph_la_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS) -static
crypto_libbitcoin_crypto_sph_la_CPPFLAGS = $(AM_CPPFLAGS)
crypto_libbitcoin_crypto_sph_la_CFLAGS = $(SPHLIB_CFLAGS)
crypto_libbitcoin_crypto_sph_la_CPPFLAGS += \
  -DSPH_SMALL_FOOTPRINT_CUBEHASH=1 \
  -DSPH_SMALL_FOOTPRINT_JH=1
crypto_libbitcoin_crypto_sph_la_SOURCES = \
  crypto/x11/aes_helper.c \
  crypto/x11/blake.c \
  crypto/x11/bmw.c \
  crypto/x11/cubehash.c \
  crypto/x11/echo.c \
  crypto/x11/groestl.c \
  crypto/x11/jh.c \
  crypto/x11/keccak.c \
  crypto/x11/luffa.c \
  crypto/x11/shavite.c \
  crypto/x11/simd.c \
  crypto/x11/skein.c \
  crypto/x11/sph_blake.h \
  crypto/x11/sph_bmw.h \
  crypto/x11/sph_cubehash.h \
  crypto/x11/sph_echo.h \
  crypto/x11/sph_groestl.h \
  crypto/x11/sph_jh.h \
  crypto/x11/sph_keccak.h \
  crypto/x11/sph_luffa.h \
  crypto/x11/sph_shavite.h \
  crypto/x11/sph_simd.h \
  crypto/x11/sph_skein.h \
  crypto/x11/sph_types.h

# See explanation for -static in crypto_libbitcoin_crypto_base_la's LDFLAGS and
# CXXFLAGS above
crypto_libbitcoin_crypto_x86_shani_la_LDFLAGS = $(AM_LDFLAGS) -static
crypto_libbitcoin_crypto_x86_shani_la_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS) -static
crypto_libbitcoin_crypto_x86_shani_la_CPPFLAGS = $(AM_CPPFLAGS)
crypto_libbitcoin_crypto_x86_shani_la_CXXFLAGS += $(X86_SHANI_CXXFLAGS)
crypto_libbitcoin_crypto_x86_shani_la_CPPFLAGS += -DENABLE_SSE41 -DENABLE_X86_SHANI
crypto_libbitcoin_crypto_x86_shani_la_SOURCES = crypto/sha256_x86_shani.cpp

# See explanation for -static in crypto_libbitcoin_crypto_base_la's LDFLAGS and
# CXXFLAGS above
crypto_libbitcoin_crypto_arm_shani_la_LDFLAGS = $(AM_LDFLAGS) -static
crypto_libbitcoin_crypto_arm_shani_la_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS) -static
crypto_libbitcoin_crypto_arm_shani_la_CPPFLAGS = $(AM_CPPFLAGS)
crypto_libbitcoin_crypto_arm_shani_la_CXXFLAGS += $(ARM_SHANI_CXXFLAGS)
crypto_libbitcoin_crypto_arm_shani_la_CPPFLAGS += -DENABLE_ARM_SHANI
crypto_libbitcoin_crypto_arm_shani_la_SOURCES = crypto/sha256_arm_shani.cpp

# consensus: shared between all executables that validate any consensus rules.
libbitcoin_consensus_a_CPPFLAGS = $(AM_CPPFLAGS) $(BITCOIN_INCLUDES) $(BOOST_CPPFLAGS)
libbitcoin_consensus_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libbitcoin_consensus_a_SOURCES = \
  arith_uint256.cpp \
  arith_uint256.h \
  bls/bls.cpp \
  bls/bls.h \
  consensus/amount.h \
  consensus/merkle.cpp \
  consensus/merkle.h \
  consensus/params.h \
  consensus/tx_check.cpp \
  consensus/validation.h \
  hash.cpp \
  hash.h \
  hash_x11.h \
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
  streams.h \
  tinyformat.h \
  uint256.cpp \
  uint256.h \
  util/strencodings.cpp \
  util/strencodings.h \
  util/string.cpp \
  version.h

# common: shared between dashd, and dash-qt and non-server tools
libbitcoin_common_a_CPPFLAGS = $(AM_CPPFLAGS) $(BITCOIN_INCLUDES)
libbitcoin_common_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libbitcoin_common_a_SOURCES = \
  base58.cpp \
  bech32.cpp \
  chainparams.cpp \
  coins.cpp \
  common/bloom.cpp \
  compressor.cpp \
  core_read.cpp \
  core_write.cpp \
  deploymentinfo.cpp \
  evo/core_write.cpp \
  evo/netinfo.cpp \
  governance/common.cpp \
  init/common.cpp \
  key.cpp \
  key_io.cpp \
  merkleblock.cpp \
  net_types.cpp \
  netaddress.cpp \
  netbase.cpp \
  net_permissions.cpp \
  outputtype.cpp \
  policy/feerate.cpp \
  policy/policy.cpp \
  protocol.cpp \
  psbt.cpp \
  rpc/evo_util.cpp \
  rpc/rawtransaction_util.cpp \
  rpc/util.cpp \
  saltedhasher.cpp \
  scheduler.cpp \
  script/descriptor.cpp \
  script/sign.cpp \
  script/signingprovider.cpp \
  script/standard.cpp \
  warnings.cpp \
  $(BITCOIN_CORE_H)

# util: shared between all executables.
libbitcoin_util_a_CPPFLAGS = $(AM_CPPFLAGS) $(BITCOIN_INCLUDES) $(BOOST_CPPFLAGS)
libbitcoin_util_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libbitcoin_util_a_SOURCES = \
  bls/bls_batchverifier.h \
  bls/bls_ies.cpp \
  bls/bls_ies.h \
  bls/bls_worker.cpp \
  bls/bls_worker.h \
  coinjoin/common.cpp \
  coinjoin/options.cpp \
  support/lockedpool.cpp \
  chainparamsbase.cpp \
  clientversion.cpp \
  fs.cpp \
  interfaces/echo.cpp \
  interfaces/handler.cpp \
  interfaces/init.cpp \
  logging.cpp \
  messagesigner.cpp \
  random.cpp \
  randomenv.cpp \
  rpc/request.cpp \
  stacktraces.cpp \
  support/cleanse.cpp \
  sync.cpp \
  util/asmap.cpp \
  util/bip32.cpp \
  util/bytevectorhash.cpp \
  util/check.cpp \
  util/edge.cpp \
  util/error.cpp \
  util/fees.cpp \
  util/hasher.cpp \
  util/getuniquepath.cpp \
  util/sock.cpp \
  util/syserror.cpp \
  util/system.cpp \
  util/message.cpp \
  util/moneystr.cpp \
  util/readwritefile.cpp \
  util/settings.cpp \
  util/ranges_set.cpp \
  util/spanparsing.cpp \
  util/strencodings.cpp \
  util/time.cpp \
  util/serfloat.cpp \
  util/string.cpp \
  util/thread.cpp \
  util/threadinterrupt.cpp \
  util/threadnames.cpp \
  util/tokenpipe.cpp \
  util/wpipe.cpp \
  $(BITCOIN_CORE_H)

if USE_LIBEVENT
libbitcoin_util_a_SOURCES += util/url.cpp
endif

# cli: shared between dash-cli and dash-qt
libbitcoin_cli_a_CPPFLAGS = $(AM_CPPFLAGS) $(BITCOIN_INCLUDES)
libbitcoin_cli_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libbitcoin_cli_a_SOURCES = \
  compat/stdin.h \
  compat/stdin.cpp \
  rpc/client.cpp \
  $(BITCOIN_CORE_H)

nodist_libbitcoin_util_a_SOURCES = $(srcdir)/obj/build.h
#

# dashd & dash-node binaries #
bitcoin_daemon_sources = bitcoind.cpp
bitcoin_bin_cppflags = $(AM_CPPFLAGS) $(BITCOIN_INCLUDES)
bitcoin_bin_cxxflags = $(AM_CXXFLAGS) $(PIE_FLAGS)
bitcoin_bin_ldflags = $(LDFLAGS_WRAP_EXCEPTIONS) $(RELDFLAGS) $(AM_LDFLAGS) $(LIBTOOL_APP_LDFLAGS) $(PTHREAD_FLAGS)

if TARGET_WINDOWS
bitcoin_daemon_sources += dashd-res.rc
endif

bitcoin_bin_ldadd = \
  $(LIBBITCOIN_WALLET) \
  $(LIBBITCOIN_COMMON) \
  $(LIBBITCOIN_UTIL) \
  $(LIBUNIVALUE) \
  $(LIBBITCOIN_ZMQ) \
  $(LIBBITCOIN_CONSENSUS) \
  $(LIBBITCOIN_CRYPTO) \
  $(LIBDASHBLS) \
  $(LIBLEVELDB) \
  $(LIBMEMENV) \
  $(LIBSECP256K1)

bitcoin_bin_ldadd += $(BACKTRACE_LIB) $(BDB_LIBS) $(MINIUPNPC_LIBS) $(NATPMP_LIBS) $(SQLITE_LIBS) $(EVENT_PTHREADS_LIBS) $(EVENT_LIBS) $(ZMQ_LIBS) $(GMP_LIBS)

dashd_SOURCES = $(bitcoin_daemon_sources) init/bitcoind.cpp
dashd_CPPFLAGS = $(bitcoin_bin_cppflags)
dashd_CXXFLAGS = $(bitcoin_bin_cxxflags)
dashd_LDFLAGS = $(bitcoin_bin_ldflags)
dashd_LDADD = $(LIBBITCOIN_NODE) $(bitcoin_bin_ldadd)

dash_node_SOURCES = $(bitcoin_daemon_sources) init/bitcoin-node.cpp
dash_node_CPPFLAGS = $(bitcoin_bin_cppflags)
dash_node_CXXFLAGS = $(bitcoin_bin_cxxflags)
dash_node_LDFLAGS = $(bitcoin_bin_ldflags)
dash_node_LDADD = $(LIBBITCOIN_NODE) $(bitcoin_bin_ldadd) $(LIBBITCOIN_IPC) $(LIBMULTIPROCESS_LIBS)

# dash-cli binary #
dash_cli_SOURCES = bitcoin-cli.cpp
dash_cli_CPPFLAGS = $(AM_CPPFLAGS) $(BITCOIN_INCLUDES) $(EVENT_CFLAGS)
dash_cli_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
dash_cli_LDFLAGS = $(LDFLAGS_WRAP_EXCEPTIONS) $(RELDFLAGS) $(AM_LDFLAGS) $(LIBTOOL_APP_LDFLAGS) $(PTHREAD_FLAGS)

if TARGET_WINDOWS
dash_cli_SOURCES += dash-cli-res.rc
endif

dash_cli_LDADD = \
  $(LIBBITCOIN_CLI) \
  $(LIBUNIVALUE) \
  $(LIBBITCOIN_UTIL) \
  $(LIBBITCOIN_CRYPTO)
dash_cli_LDADD += $(BACKTRACE_LIB) $(EVENT_LIBS) $(GMP_LIBS)
#

# dash-tx binary #
dash_tx_SOURCES = bitcoin-tx.cpp
dash_tx_CPPFLAGS = $(AM_CPPFLAGS) $(BITCOIN_INCLUDES)
dash_tx_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
dash_tx_LDFLAGS = $(LDFLAGS_WRAP_EXCEPTIONS) $(RELDFLAGS) $(AM_LDFLAGS) $(LIBTOOL_APP_LDFLAGS) $(PTHREAD_FLAGS)

if TARGET_WINDOWS
dash_tx_SOURCES += dash-tx-res.rc
endif

dash_tx_LDADD = \
  $(LIBUNIVALUE) \
  $(LIBBITCOIN_COMMON) \
  $(LIBBITCOIN_UTIL) \
  $(LIBBITCOIN_CONSENSUS) \
  $(LIBBITCOIN_CRYPTO) \
  $(LIBDASHBLS) \
  $(LIBSECP256K1)

dash_tx_LDADD += $(BACKTRACE_LIB) $(GMP_LIBS)
#

# dash-wallet binary #
dash_wallet_SOURCES = bitcoin-wallet.cpp
dash_wallet_CPPFLAGS = $(bitcoin_bin_cppflags)
dash_wallet_CXXFLAGS = $(bitcoin_bin_cxxflags)
dash_wallet_LDFLAGS = $(bitcoin_bin_ldflags)
dash_wallet_LDADD = $(LIBBITCOIN_WALLET_TOOL) $(bitcoin_bin_ldadd)

if TARGET_WINDOWS
dash_wallet_SOURCES += dash-wallet-res.rc
endif
#

# dashconsensus library #
if BUILD_BITCOIN_LIBS
include_HEADERS = script/bitcoinconsensus.h
libdashconsensus_la_SOURCES = support/cleanse.cpp $(crypto_libbitcoin_crypto_base_la_SOURCES) $(crypto_libbitcoin_crypto_sph_la_SOURCES) $(libbitcoin_consensus_a_SOURCES)

libdashconsensus_la_LDFLAGS = $(AM_LDFLAGS) -no-undefined $(RELDFLAGS)
libdashconsensus_la_LIBADD = $(LIBDASHBLS) $(LIBSECP256K1) $(GMP_LIBS)
libdashconsensus_la_CPPFLAGS = $(AM_CPPFLAGS) -I$(builddir)/obj -I$(srcdir)/secp256k1/include -DBUILD_BITCOIN_INTERNAL -DDISABLE_OPTIMIZED_SHA256
libdashconsensus_la_CPPFLAGS += -isystem$(srcdir)/dashbls/include -isystem$(srcdir)/dashbls/depends/relic/include -isystem$(srcdir)/dashbls/depends/minialloc/include
libdashconsensus_la_CPPFLAGS += -isystem$(srcdir)/immer
libdashconsensus_la_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)

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
CLEANFILES += crc32c/src/*.gcda crc32c/src/*.gcno
CLEANFILES += crypto/*.gcda crypto/*.gcno
CLEANFILES += index/*.gcda index/*.gcno
CLEANFILES += interfaces/*.gcda interfaces/*.gcno
CLEANFILES += node/*.gcda node/*.gcno
CLEANFILES += policy/*.gcda policy/*.gcno
CLEANFILES += primitives/*.gcda primitives/*.gcno
CLEANFILES += rpc/*.gcda rpc/*.gcno
CLEANFILES += script/*.gcda script/*.gcno
CLEANFILES += support/*.gcda support/*.gcno
CLEANFILES += univalue/*.gcda univalue/*.gcno
CLEANFILES += util/*.gcda util/*.gcno
CLEANFILES += wallet/*.gcda wallet/*.gcno
CLEANFILES += wallet/test/*.gcda wallet/test/*.gcno
CLEANFILES += zmq/*.gcda zmq/*.gcno
CLEANFILES += obj/build.h

IMMER_DIST = immer/immer
LIBDASHBLS_DIST = dashbls

EXTRA_DIST = \
  $(CTAES_DIST)   \
  $(IMMER_DIST)   \
  $(LIBDASHBLS_DIST)

config/bitcoin-config.h: config/stamp-h1
	@$(MAKE) -C $(top_builddir) $(subdir)/$(@)
config/stamp-h1: $(top_srcdir)/$(subdir)/config/bitcoin-config.h.in $(top_builddir)/config.status
	$(AM_V_at)$(MAKE) -C $(top_builddir) $(subdir)/$(@)
$(top_srcdir)/$(subdir)/config/bitcoin-config.h.in:  $(am__configure_deps)
	$(AM_V_at)$(MAKE) -C $(top_srcdir) $(subdir)/config/bitcoin-config.h.in


clean-local:
	-$(MAKE) -C dashbls clean
	-$(MAKE) -C secp256k1 clean
	-rm -f leveldb/*/*.gcda leveldb/*/*.gcno leveldb/helpers/memenv/*.gcda leveldb/helpers/memenv/*.gcno
	-rm -f config.h
	-rm -rf test/__pycache__
	-rm -rf *.dSYM test/*.dSYM bench/*.dSYM qt/*.dSYM qt/test/*.dSYM

.rc.o:
	@test -f $(WINDRES) || (echo "windres $(WINDRES) not found, but is required to compile windows resource files"; exit 1)
	## FIXME: How to get the appropriate modulename_CPPFLAGS in here?
	$(AM_V_GEN) $(WINDRES) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(CPPFLAGS) -DWINDRES_PREPROC -i $< -o $@

check-symbols: $(bin_PROGRAMS)
	@echo "Running symbol and dynamic library checks..."
	$(AM_V_at) $(PYTHON) $(top_srcdir)/contrib/devtools/symbol-check.py $(bin_PROGRAMS)

check-security: $(bin_PROGRAMS)
if HARDEN
	@echo "Checking binary security..."
	$(AM_V_at) $(PYTHON) $(top_srcdir)/contrib/devtools/security-check.py $(bin_PROGRAMS)
endif


osx_debug: $(bin_PROGRAMS)
	for i in $(bin_PROGRAMS); do mkdir -p $$i.dSYM/Contents/Resources/DWARF && $(DSYMUTIL_FLAT) -o $$i.dSYM/Contents/Resources/DWARF/$$(basename $$i) $$i &> /dev/null ; done

libbitcoin_ipc_mpgen_input = \
  ipc/capnp/echo.capnp \
  ipc/capnp/init.capnp
EXTRA_DIST += $(libbitcoin_ipc_mpgen_input)
%.capnp:

# Explicitly list dependencies on generated headers as described in
# https://www.gnu.org/software/automake/manual/html_node/Built-Sources-Example.html#Recording-Dependencies-manually
ipc/capnp/libbitcoin_ipc_a-protocol.$(OBJEXT): $(libbitcoin_ipc_mpgen_input:=.h)

if BUILD_MULTIPROCESS
LIBBITCOIN_IPC=libbitcoin_ipc.a
libbitcoin_ipc_a_SOURCES = \
  ipc/capnp/context.h \
  ipc/capnp/init-types.h \
  ipc/capnp/protocol.cpp \
  ipc/capnp/protocol.h \
  ipc/context.h \
  ipc/exception.h \
  ipc/interfaces.cpp \
  ipc/process.cpp \
  ipc/process.h \
  ipc/protocol.h
libbitcoin_ipc_a_CPPFLAGS = $(AM_CPPFLAGS) $(BITCOIN_INCLUDES)
libbitcoin_ipc_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS) $(LIBMULTIPROCESS_CFLAGS)

include $(MPGEN_PREFIX)/include/mpgen.mk
libbitcoin_ipc_mpgen_output = \
  $(libbitcoin_ipc_mpgen_input:=.c++) \
  $(libbitcoin_ipc_mpgen_input:=.h) \
  $(libbitcoin_ipc_mpgen_input:=.proxy-client.c++) \
  $(libbitcoin_ipc_mpgen_input:=.proxy-server.c++) \
  $(libbitcoin_ipc_mpgen_input:=.proxy-types.c++) \
  $(libbitcoin_ipc_mpgen_input:=.proxy-types.h) \
  $(libbitcoin_ipc_mpgen_input:=.proxy.h)
nodist_libbitcoin_ipc_a_SOURCES = $(libbitcoin_ipc_mpgen_output)
CLEANFILES += $(libbitcoin_ipc_mpgen_output)
endif

%.raw.h: %.raw
	@$(MKDIR_P) $(@D)
	$(AM_V_GEN) { \
	 echo "static unsigned const char $(*F)_raw[] = {" && \
	 $(HEXDUMP) -v -e '8/1 "0x%02x, "' -e '"\n"' $< | $(SED) -e 's/0x  ,//g' && \
	 echo "};"; \
	} > "$@.new" && mv -f "$@.new" "$@"

include Makefile.minisketch.include

include Makefile.crc32c.include
include Makefile.leveldb.include

include Makefile.test_util.include
include Makefile.test_fuzz.include

include Makefile.test.include

if ENABLE_BENCH
include Makefile.bench.include
endif

if ENABLE_QT
include Makefile.qt.include
endif

if ENABLE_QT_TESTS
include Makefile.qttest.include
endif

include Makefile.univalue.include
