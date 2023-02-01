# Copyright (c) 2013-2016 The BritanniaCoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# Pattern rule to print variables, e.g. make print-top_srcdir
print-%: FORCE
	@echo '$*'='$($*)'

DIST_SUBDIRS = secp256k1

AM_LDFLAGS = $(LIBTOOL_LDFLAGS) $(HARDENED_LDFLAGS) $(GPROF_LDFLAGS) $(SANITIZER_LDFLAGS) $(LTO_LDFLAGS) $(CORE_LDFLAGS)
AM_CXXFLAGS = $(DEBUG_CXXFLAGS) $(HARDENED_CXXFLAGS) $(WARN_CXXFLAGS) $(NOWARN_CXXFLAGS) $(ERROR_CXXFLAGS) $(GPROF_CXXFLAGS) $(SANITIZER_CXXFLAGS) $(LTO_CXXFLAGS) $(CORE_CXXFLAGS)
AM_CPPFLAGS = $(DEBUG_CPPFLAGS) $(HARDENED_CPPFLAGS) $(CORE_CPPFLAGS)
AM_LIBTOOLFLAGS = --preserve-dup-deps
PTHREAD_FLAGS = $(PTHREAD_CFLAGS) $(PTHREAD_LIBS)
EXTRA_LIBRARIES =

lib_LTLIBRARIES =
noinst_LTLIBRARIES =

bin_PROGRAMS =
noinst_PROGRAMS =
check_PROGRAMS =
TESTS =
BENCHMARKS =

BRITANNIACOIN_INCLUDES=-I$(builddir) -I$(srcdir)/$(MINISKETCH_INCLUDE_DIR_INT) -I$(srcdir)/secp256k1/include -I$(srcdir)/$(UNIVALUE_INCLUDE_DIR_INT) $(LEVELDB_CPPFLAGS)

LIBBRITANNIACOIN_NODE=libbritanniacoin_node.a
LIBBRITANNIACOIN_COMMON=libbritanniacoin_common.a
LIBBRITANNIACOIN_CONSENSUS=libbritanniacoin_consensus.a
LIBBRITANNIACOIN_CLI=libbritanniacoin_cli.a
LIBBRITANNIACOIN_UTIL=libbritanniacoin_util.a
LIBBRITANNIACOIN_CRYPTO_BASE=crypto/libbritanniacoin_crypto_base.la
LIBBRITANNIACOINQT=qt/libbritanniacoinqt.a
LIBSECP256K1=secp256k1/libsecp256k1.la

if ENABLE_ZMQ
LIBBRITANNIACOIN_ZMQ=libbritanniacoin_zmq.a
endif
if BUILD_BRITANNIACOIN_LIBS
LIBBRITANNIACOINCONSENSUS=libbritanniacoinconsensus.la
endif
if BUILD_BRITANNIACOIN_KERNEL_LIB
LIBBRITANNIACOINKERNEL=libbritanniacoinkernel.la
endif
if ENABLE_WALLET
LIBBRITANNIACOIN_WALLET=libbritanniacoin_wallet.a
LIBBRITANNIACOIN_WALLET_TOOL=libbritanniacoin_wallet_tool.a
endif

LIBBRITANNIACOIN_CRYPTO = $(LIBBRITANNIACOIN_CRYPTO_BASE)
if ENABLE_SSE41
LIBBRITANNIACOIN_CRYPTO_SSE41 = crypto/libbritanniacoin_crypto_sse41.la
LIBBRITANNIACOIN_CRYPTO += $(LIBBRITANNIACOIN_CRYPTO_SSE41)
endif
if ENABLE_AVX2
LIBBRITANNIACOIN_CRYPTO_AVX2 = crypto/libbritanniacoin_crypto_avx2.la
LIBBRITANNIACOIN_CRYPTO += $(LIBBRITANNIACOIN_CRYPTO_AVX2)
endif
if ENABLE_X86_SHANI
LIBBRITANNIACOIN_CRYPTO_X86_SHANI = crypto/libbritanniacoin_crypto_x86_shani.la
LIBBRITANNIACOIN_CRYPTO += $(LIBBRITANNIACOIN_CRYPTO_X86_SHANI)
endif
if ENABLE_ARM_SHANI
LIBBRITANNIACOIN_CRYPTO_ARM_SHANI = crypto/libbritanniacoin_crypto_arm_shani.la
LIBBRITANNIACOIN_CRYPTO += $(LIBBRITANNIACOIN_CRYPTO_ARM_SHANI)
endif
noinst_LTLIBRARIES += $(LIBBRITANNIACOIN_CRYPTO)

$(LIBSECP256K1): $(wildcard secp256k1/src/*.h) $(wildcard secp256k1/src/*.c) $(wildcard secp256k1/include/*)
	$(AM_V_at)$(MAKE) $(AM_MAKEFLAGS) -C $(@D) $(@F)

# Make is not made aware of per-object dependencies to avoid limiting building parallelization
# But to build the less dependent modules first, we manually select their order here:
EXTRA_LIBRARIES += \
  $(LIBBRITANNIACOIN_UTIL) \
  $(LIBBRITANNIACOIN_COMMON) \
  $(LIBBRITANNIACOIN_CONSENSUS) \
  $(LIBBRITANNIACOIN_NODE) \
  $(LIBBRITANNIACOIN_CLI) \
  $(LIBBRITANNIACOIN_IPC) \
  $(LIBBRITANNIACOIN_WALLET) \
  $(LIBBRITANNIACOIN_WALLET_TOOL) \
  $(LIBBRITANNIACOIN_ZMQ)

if BUILD_BRITANNIACOIND
  bin_PROGRAMS += britanniacoind
endif

if BUILD_BRITANNIACOIN_NODE
  bin_PROGRAMS += britanniacoin-node
endif

if BUILD_BRITANNIACOIN_CLI
  bin_PROGRAMS += britanniacoin-cli
endif

if BUILD_BRITANNIACOIN_TX
  bin_PROGRAMS += britanniacoin-tx
endif

if ENABLE_WALLET
if BUILD_BRITANNIACOIN_WALLET
  bin_PROGRAMS += britanniacoin-wallet
endif
endif

if BUILD_BRITANNIACOIN_UTIL
  bin_PROGRAMS += britanniacoin-util
endif

if BUILD_BRITANNIACOIN_CHAINSTATE
  bin_PROGRAMS += britanniacoin-chainstate
endif

.PHONY: FORCE check-symbols check-security
# britanniacoin core #
BRITANNIACOIN_CORE_H = \
  addrdb.h \
  addrman.h \
  addrman_impl.h \
  attributes.h \
  banman.h \
  base58.h \
  bech32.h \
  blockencodings.h \
  blockfilter.h \
  chain.h \
  chainparams.h \
  chainparamsbase.h \
  chainparamsseeds.h \
  checkqueue.h \
  clientversion.h \
  coins.h \
  common/bloom.h \
  common/run_command.h \
  common/url.h \
  compat/assumptions.h \
  compat/byteswap.h \
  compat/compat.h \
  compat/cpuid.h \
  compat/endian.h \
  compressor.h \
  consensus/consensus.h \
  consensus/tx_check.h \
  consensus/tx_verify.h \
  core_io.h \
  core_memusage.h \
  cuckoocache.h \
  dbwrapper.h \
  deploymentinfo.h \
  deploymentstatus.h \
  external_signer.h \
  flatfile.h \
  fs.h \
  headerssync.h \
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
  interfaces/echo.h \
  interfaces/handler.h \
  interfaces/init.h \
  interfaces/ipc.h \
  interfaces/node.h \
  interfaces/wallet.h \
  kernel/chain.h \
  kernel/chainstatemanager_opts.h \
  kernel/checks.h \
  kernel/coinstats.h \
  kernel/context.h \
  kernel/cs_main.h \
  kernel/mempool_entry.h \
  kernel/mempool_limits.h \
  kernel/mempool_options.h \
  kernel/mempool_persist.h \
  kernel/validation_cache_sizes.h \
  key.h \
  key_io.h \
  logging.h \
  logging/timer.h \
  mapport.h \
  memusage.h \
  merkleblock.h \
  net.h \
  net_permissions.h \
  net_processing.h \
  net_types.h \
  netaddress.h \
  netbase.h \
  netgroup.h \
  netmessagemaker.h \
  node/blockstorage.h \
  node/caches.h \
  node/chainstate.h \
  node/chainstatemanager_args.h \
  node/coin.h \
  node/connection_types.h \
  node/context.h \
  node/eviction.h \
  node/interface_ui.h \
  node/mempool_args.h \
  node/mempool_persist_args.h \
  node/miner.h \
  node/minisketchwrapper.h \
  node/psbt.h \
  node/transaction.h \
  node/txreconciliation.h \
  node/utxo_snapshot.h \
  node/validation_cache_args.h \
  noui.h \
  outputtype.h \
  policy/feerate.h \
  policy/fees.h \
  policy/fees_args.h \
  policy/packages.h \
  policy/policy.h \
  policy/rbf.h \
  policy/settings.h \
  pow.h \
  protocol.h \
  psbt.h \
  random.h \
  randomenv.h \
  rest.h \
  reverse_iterator.h \
  rpc/blockchain.h \
  rpc/client.h \
  rpc/mempool.h \
  rpc/mining.h \
  rpc/protocol.h \
  rpc/rawtransaction_util.h \
  rpc/register.h \
  rpc/request.h \
  rpc/server.h \
  rpc/server_util.h \
  rpc/util.h \
  scheduler.h \
  script/descriptor.h \
  script/keyorigin.h \
  script/miniscript.h \
  script/sigcache.h \
  script/sign.h \
  script/signingprovider.h \
  script/standard.h \
  shutdown.h \
  signet.h \
  streams.h \
  support/allocators/secure.h \
  support/allocators/zeroafterfree.h \
  support/cleanse.h \
  support/events.h \
  support/lockedpool.h \
  sync.h \
  threadsafety.h \
  timedata.h \
  torcontrol.h \
  txdb.h \
  txmempool.h \
  txorphanage.h \
  txrequest.h \
  undo.h \
  util/asmap.h \
  util/bip32.h \
  util/bitdeque.h \
  util/bytevectorhash.h \
  util/check.h \
  util/epochguard.h \
  util/error.h \
  util/fastrange.h \
  util/fees.h \
  util/getuniquepath.h \
  util/golombrice.h \
  util/hash_type.h \
  util/hasher.h \
  util/macros.h \
  util/message.h \
  util/moneystr.h \
  util/overflow.h \
  util/overloaded.h \
  util/rbf.h \
  util/readwritefile.h \
  util/result.h \
  util/serfloat.h \
  util/settings.h \
  util/sock.h \
  util/spanparsing.h \
  util/string.h \
  util/syscall_sandbox.h \
  util/syserror.h \
  util/system.h \
  util/thread.h \
  util/threadinterrupt.h \
  util/threadnames.h \
  util/time.h \
  util/tokenpipe.h \
  util/trace.h \
  util/translation.h \
  util/types.h \
  util/ui_change_type.h \
  util/vector.h \
  validation.h \
  validationinterface.h \
  versionbits.h \
  wallet/bdb.h \
  wallet/coincontrol.h \
  wallet/coinselection.h \
  wallet/context.h \
  wallet/crypter.h \
  wallet/db.h \
  wallet/dump.h \
  wallet/external_signer_scriptpubkeyman.h \
  wallet/feebumper.h \
  wallet/fees.h \
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
	@$(top_srcdir)/share/genbuild.sh "$(abs_top_builddir)/src/obj/build.h" \
	  "$(abs_top_srcdir)"
libbritanniacoin_util_a-clientversion.$(OBJEXT): obj/build.h

# node #
libbritanniacoin_node_a_CPPFLAGS = $(AM_CPPFLAGS) $(BRITANNIACOIN_INCLUDES) $(BOOST_CPPFLAGS) $(MINIUPNPC_CPPFLAGS) $(NATPMP_CPPFLAGS) $(EVENT_CFLAGS) $(EVENT_PTHREADS_CFLAGS)
libbritanniacoin_node_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libbritanniacoin_node_a_SOURCES = \
  addrdb.cpp \
  addrman.cpp \
  banman.cpp \
  blockencodings.cpp \
  blockfilter.cpp \
  chain.cpp \
  consensus/tx_verify.cpp \
  dbwrapper.cpp \
  deploymentstatus.cpp \
  flatfile.cpp \
  headerssync.cpp \
  httprpc.cpp \
  httpserver.cpp \
  i2p.cpp \
  index/base.cpp \
  index/blockfilterindex.cpp \
  index/coinstatsindex.cpp \
  index/txindex.cpp \
  init.cpp \
  kernel/chain.cpp \
  kernel/checks.cpp \
  kernel/coinstats.cpp \
  kernel/context.cpp \
  kernel/cs_main.cpp \
  kernel/mempool_persist.cpp \
  mapport.cpp \
  net.cpp \
  net_processing.cpp \
  netgroup.cpp \
  node/blockstorage.cpp \
  node/caches.cpp \
  node/chainstate.cpp \
  node/chainstatemanager_args.cpp \
  node/coin.cpp \
  node/connection_types.cpp \
  node/context.cpp \
  node/eviction.cpp \
  node/interface_ui.cpp \
  node/interfaces.cpp \
  node/mempool_args.cpp \
  node/mempool_persist_args.cpp \
  node/miner.cpp \
  node/minisketchwrapper.cpp \
  node/psbt.cpp \
  node/transaction.cpp \
  node/txreconciliation.cpp \
  node/utxo_snapshot.cpp \
  node/validation_cache_args.cpp \
  noui.cpp \
  policy/fees.cpp \
  policy/fees_args.cpp \
  policy/packages.cpp \
  policy/rbf.cpp \
  policy/settings.cpp \
  pow.cpp \
  rest.cpp \
  rpc/blockchain.cpp \
  rpc/fees.cpp \
  rpc/mempool.cpp \
  rpc/mining.cpp \
  rpc/net.cpp \
  rpc/node.cpp \
  rpc/output_script.cpp \
  rpc/rawtransaction.cpp \
  rpc/server.cpp \
  rpc/server_util.cpp \
  rpc/signmessage.cpp \
  rpc/txoutproof.cpp \
  script/sigcache.cpp \
  shutdown.cpp \
  signet.cpp \
  timedata.cpp \
  torcontrol.cpp \
  txdb.cpp \
  txmempool.cpp \
  txorphanage.cpp \
  txrequest.cpp \
  validation.cpp \
  validationinterface.cpp \
  versionbits.cpp \
  $(BRITANNIACOIN_CORE_H)

if ENABLE_WALLET
libbritanniacoin_node_a_SOURCES += wallet/init.cpp
libbritanniacoin_node_a_CPPFLAGS += $(BDB_CPPFLAGS)
endif
if !ENABLE_WALLET
libbritanniacoin_node_a_SOURCES += dummywallet.cpp
endif
#

# zmq #
if ENABLE_ZMQ
libbritanniacoin_zmq_a_CPPFLAGS = $(AM_CPPFLAGS) $(BRITANNIACOIN_INCLUDES) $(ZMQ_CFLAGS)
libbritanniacoin_zmq_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libbritanniacoin_zmq_a_SOURCES = \
  zmq/zmqabstractnotifier.cpp \
  zmq/zmqnotificationinterface.cpp \
  zmq/zmqpublishnotifier.cpp \
  zmq/zmqrpc.cpp \
  zmq/zmqutil.cpp
endif
#

# wallet #
libbritanniacoin_wallet_a_CPPFLAGS = $(AM_CPPFLAGS) $(BRITANNIACOIN_INCLUDES) $(BOOST_CPPFLAGS) $(BDB_CPPFLAGS) $(SQLITE_CFLAGS)
libbritanniacoin_wallet_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libbritanniacoin_wallet_a_SOURCES = \
  wallet/coincontrol.cpp \
  wallet/context.cpp \
  wallet/crypter.cpp \
  wallet/db.cpp \
  wallet/dump.cpp \
  wallet/external_signer_scriptpubkeyman.cpp \
  wallet/feebumper.cpp \
  wallet/fees.cpp \
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
  $(BRITANNIACOIN_CORE_H)

if USE_SQLITE
libbritanniacoin_wallet_a_SOURCES += wallet/sqlite.cpp
endif
if USE_BDB
libbritanniacoin_wallet_a_SOURCES += wallet/bdb.cpp wallet/salvage.cpp
endif
#

# wallet tool #
libbritanniacoin_wallet_tool_a_CPPFLAGS = $(AM_CPPFLAGS) $(BRITANNIACOIN_INCLUDES) $(BOOST_CPPFLAGS)
libbritanniacoin_wallet_tool_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libbritanniacoin_wallet_tool_a_SOURCES = \
  wallet/wallettool.cpp \
  $(BRITANNIACOIN_CORE_H)
#

# crypto #
crypto_libbritanniacoin_crypto_base_la_CPPFLAGS = $(AM_CPPFLAGS)

# Specify -static in both CXXFLAGS and LDFLAGS so libtool will only build a
# static version of this library. We don't need a dynamic version, and a dynamic
# version can't be used on windows anyway because the library doesn't currently
# export DLL symbols.
crypto_libbritanniacoin_crypto_base_la_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS) -static
crypto_libbritanniacoin_crypto_base_la_LDFLAGS = $(AM_LDFLAGS) -static

crypto_libbritanniacoin_crypto_base_la_SOURCES = \
  crypto/aes.cpp \
  crypto/aes.h \
  crypto/chacha_poly_aead.h \
  crypto/chacha_poly_aead.cpp \
  crypto/chacha20.h \
  crypto/chacha20.cpp \
  crypto/common.h \
  crypto/hkdf_sha256_32.cpp \
  crypto/hkdf_sha256_32.h \
  crypto/hmac_sha256.cpp \
  crypto/hmac_sha256.h \
  crypto/hmac_sha512.cpp \
  crypto/hmac_sha512.h \
  crypto/poly1305.h \
  crypto/poly1305.cpp \
  crypto/muhash.h \
  crypto/muhash.cpp \
  crypto/ripemd160.cpp \
  crypto/ripemd160.h \
  crypto/sha1.cpp \
  crypto/sha1.h \
  crypto/sha256.cpp \
  crypto/sha256.h \
  crypto/sha3.cpp \
  crypto/sha3.h \
  crypto/sha512.cpp \
  crypto/sha512.h \
  crypto/siphash.cpp \
  crypto/siphash.h

if USE_ASM
crypto_libbritanniacoin_crypto_base_la_SOURCES += crypto/sha256_sse4.cpp
endif

# See explanation for -static in crypto_libbritanniacoin_crypto_base_la's LDFLAGS and
# CXXFLAGS above
crypto_libbritanniacoin_crypto_sse41_la_LDFLAGS = $(AM_LDFLAGS) -static
crypto_libbritanniacoin_crypto_sse41_la_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS) -static
crypto_libbritanniacoin_crypto_sse41_la_CPPFLAGS = $(AM_CPPFLAGS)
crypto_libbritanniacoin_crypto_sse41_la_CXXFLAGS += $(SSE41_CXXFLAGS)
crypto_libbritanniacoin_crypto_sse41_la_CPPFLAGS += -DENABLE_SSE41
crypto_libbritanniacoin_crypto_sse41_la_SOURCES = crypto/sha256_sse41.cpp

# See explanation for -static in crypto_libbritanniacoin_crypto_base_la's LDFLAGS and
# CXXFLAGS above
crypto_libbritanniacoin_crypto_avx2_la_LDFLAGS = $(AM_LDFLAGS) -static
crypto_libbritanniacoin_crypto_avx2_la_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS) -static
crypto_libbritanniacoin_crypto_avx2_la_CPPFLAGS = $(AM_CPPFLAGS)
crypto_libbritanniacoin_crypto_avx2_la_CXXFLAGS += $(AVX2_CXXFLAGS)
crypto_libbritanniacoin_crypto_avx2_la_CPPFLAGS += -DENABLE_AVX2
crypto_libbritanniacoin_crypto_avx2_la_SOURCES = crypto/sha256_avx2.cpp

# See explanation for -static in crypto_libbritanniacoin_crypto_base_la's LDFLAGS and
# CXXFLAGS above
crypto_libbritanniacoin_crypto_x86_shani_la_LDFLAGS = $(AM_LDFLAGS) -static
crypto_libbritanniacoin_crypto_x86_shani_la_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS) -static
crypto_libbritanniacoin_crypto_x86_shani_la_CPPFLAGS = $(AM_CPPFLAGS)
crypto_libbritanniacoin_crypto_x86_shani_la_CXXFLAGS += $(X86_SHANI_CXXFLAGS)
crypto_libbritanniacoin_crypto_x86_shani_la_CPPFLAGS += -DENABLE_X86_SHANI
crypto_libbritanniacoin_crypto_x86_shani_la_SOURCES = crypto/sha256_x86_shani.cpp

# See explanation for -static in crypto_libbritanniacoin_crypto_base_la's LDFLAGS and
# CXXFLAGS above
crypto_libbritanniacoin_crypto_arm_shani_la_LDFLAGS = $(AM_LDFLAGS) -static
crypto_libbritanniacoin_crypto_arm_shani_la_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS) -static
crypto_libbritanniacoin_crypto_arm_shani_la_CPPFLAGS = $(AM_CPPFLAGS)
crypto_libbritanniacoin_crypto_arm_shani_la_CXXFLAGS += $(ARM_SHANI_CXXFLAGS)
crypto_libbritanniacoin_crypto_arm_shani_la_CPPFLAGS += -DENABLE_ARM_SHANI
crypto_libbritanniacoin_crypto_arm_shani_la_SOURCES = crypto/sha256_arm_shani.cpp
#

# consensus #
libbritanniacoin_consensus_a_CPPFLAGS = $(AM_CPPFLAGS) $(BRITANNIACOIN_INCLUDES)
libbritanniacoin_consensus_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libbritanniacoin_consensus_a_SOURCES = \
  arith_uint256.cpp \
  arith_uint256.h \
  consensus/amount.h \
  consensus/merkle.cpp \
  consensus/merkle.h \
  consensus/params.h \
  consensus/tx_check.cpp \
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
  script/britanniacoinconsensus.cpp \
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
  util/strencodings.cpp \
  util/strencodings.h \
  version.h
#

# common #
libbritanniacoin_common_a_CPPFLAGS = $(AM_CPPFLAGS) $(BRITANNIACOIN_INCLUDES) $(BOOST_CPPFLAGS)
libbritanniacoin_common_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libbritanniacoin_common_a_SOURCES = \
  base58.cpp \
  bech32.cpp \
  chainparams.cpp \
  coins.cpp \
  common/bloom.cpp \
  common/interfaces.cpp \
  common/run_command.cpp \
  compressor.cpp \
  core_read.cpp \
  core_write.cpp \
  deploymentinfo.cpp \
  external_signer.cpp \
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
  rpc/external_signer.cpp \
  rpc/rawtransaction_util.cpp \
  rpc/request.cpp \
  rpc/util.cpp \
  scheduler.cpp \
  script/descriptor.cpp \
  script/miniscript.cpp \
  script/sign.cpp \
  script/signingprovider.cpp \
  script/standard.cpp \
  warnings.cpp \
  $(BRITANNIACOIN_CORE_H)

if USE_LIBEVENT
libbritanniacoin_common_a_CPPFLAGS += $(EVENT_CFLAGS)
libbritanniacoin_common_a_SOURCES += common/url.cpp
endif
#

# util #
libbritanniacoin_util_a_CPPFLAGS = $(AM_CPPFLAGS) $(BRITANNIACOIN_INCLUDES)
libbritanniacoin_util_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libbritanniacoin_util_a_SOURCES = \
  support/lockedpool.cpp \
  chainparamsbase.cpp \
  clientversion.cpp \
  fs.cpp \
  logging.cpp \
  random.cpp \
  randomenv.cpp \
  support/cleanse.cpp \
  sync.cpp \
  util/asmap.cpp \
  util/bip32.cpp \
  util/bytevectorhash.cpp \
  util/check.cpp \
  util/error.cpp \
  util/fees.cpp \
  util/getuniquepath.cpp \
  util/hasher.cpp \
  util/sock.cpp \
  util/syserror.cpp \
  util/system.cpp \
  util/message.cpp \
  util/moneystr.cpp \
  util/rbf.cpp \
  util/readwritefile.cpp \
  util/settings.cpp \
  util/thread.cpp \
  util/threadinterrupt.cpp \
  util/threadnames.cpp \
  util/serfloat.cpp \
  util/spanparsing.cpp \
  util/strencodings.cpp \
  util/string.cpp \
  util/syscall_sandbox.cpp \
  util/time.cpp \
  util/tokenpipe.cpp \
  $(BRITANNIACOIN_CORE_H)
#

# cli #
libbritanniacoin_cli_a_CPPFLAGS = $(AM_CPPFLAGS) $(BRITANNIACOIN_INCLUDES)
libbritanniacoin_cli_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libbritanniacoin_cli_a_SOURCES = \
  compat/stdin.h \
  compat/stdin.cpp \
  rpc/client.cpp \
  $(BRITANNIACOIN_CORE_H)

nodist_libbritanniacoin_util_a_SOURCES = $(srcdir)/obj/build.h
#

# britanniacoind & britanniacoin-node binaries #
britanniacoin_daemon_sources = britanniacoind.cpp
britanniacoin_bin_cppflags = $(AM_CPPFLAGS) $(BRITANNIACOIN_INCLUDES)
britanniacoin_bin_cxxflags = $(AM_CXXFLAGS) $(PIE_FLAGS)
britanniacoin_bin_ldflags = $(RELDFLAGS) $(AM_LDFLAGS) $(LIBTOOL_APP_LDFLAGS) $(PTHREAD_FLAGS)

if TARGET_WINDOWS
britanniacoin_daemon_sources += britanniacoind-res.rc
endif

britanniacoin_bin_ldadd = \
  $(LIBBRITANNIACOIN_WALLET) \
  $(LIBBRITANNIACOIN_COMMON) \
  $(LIBBRITANNIACOIN_UTIL) \
  $(LIBUNIVALUE) \
  $(LIBBRITANNIACOIN_ZMQ) \
  $(LIBBRITANNIACOIN_CONSENSUS) \
  $(LIBBRITANNIACOIN_CRYPTO) \
  $(LIBLEVELDB) \
  $(LIBMEMENV) \
  $(LIBSECP256K1)

britanniacoin_bin_ldadd += $(BDB_LIBS) $(MINIUPNPC_LIBS) $(NATPMP_LIBS) $(EVENT_PTHREADS_LIBS) $(EVENT_LIBS) $(ZMQ_LIBS) $(SQLITE_LIBS)

britanniacoind_SOURCES = $(britanniacoin_daemon_sources) init/britanniacoind.cpp
britanniacoind_CPPFLAGS = $(britanniacoin_bin_cppflags)
britanniacoind_CXXFLAGS = $(britanniacoin_bin_cxxflags)
britanniacoind_LDFLAGS = $(britanniacoin_bin_ldflags)
britanniacoind_LDADD = $(LIBBRITANNIACOIN_NODE) $(britanniacoin_bin_ldadd)

britanniacoin_node_SOURCES = $(britanniacoin_daemon_sources) init/britanniacoin-node.cpp
britanniacoin_node_CPPFLAGS = $(britanniacoin_bin_cppflags)
britanniacoin_node_CXXFLAGS = $(britanniacoin_bin_cxxflags)
britanniacoin_node_LDFLAGS = $(britanniacoin_bin_ldflags)
britanniacoin_node_LDADD = $(LIBBRITANNIACOIN_NODE) $(britanniacoin_bin_ldadd) $(LIBBRITANNIACOIN_IPC) $(LIBMULTIPROCESS_LIBS)

# britanniacoin-cli binary #
britanniacoin_cli_SOURCES = britanniacoin-cli.cpp
britanniacoin_cli_CPPFLAGS = $(AM_CPPFLAGS) $(BRITANNIACOIN_INCLUDES) $(EVENT_CFLAGS)
britanniacoin_cli_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
britanniacoin_cli_LDFLAGS = $(RELDFLAGS) $(AM_LDFLAGS) $(LIBTOOL_APP_LDFLAGS) $(PTHREAD_FLAGS)

if TARGET_WINDOWS
britanniacoin_cli_SOURCES += britanniacoin-cli-res.rc
endif

britanniacoin_cli_LDADD = \
  $(LIBBRITANNIACOIN_CLI) \
  $(LIBUNIVALUE) \
  $(LIBBRITANNIACOIN_COMMON) \
  $(LIBBRITANNIACOIN_UTIL) \
  $(LIBBRITANNIACOIN_CRYPTO)

britanniacoin_cli_LDADD += $(EVENT_LIBS)
#

# britanniacoin-tx binary #
britanniacoin_tx_SOURCES = britanniacoin-tx.cpp
britanniacoin_tx_CPPFLAGS = $(AM_CPPFLAGS) $(BRITANNIACOIN_INCLUDES)
britanniacoin_tx_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
britanniacoin_tx_LDFLAGS = $(RELDFLAGS) $(AM_LDFLAGS) $(LIBTOOL_APP_LDFLAGS) $(PTHREAD_FLAGS)

if TARGET_WINDOWS
britanniacoin_tx_SOURCES += britanniacoin-tx-res.rc
endif

britanniacoin_tx_LDADD = \
  $(LIBUNIVALUE) \
  $(LIBBRITANNIACOIN_COMMON) \
  $(LIBBRITANNIACOIN_UTIL) \
  $(LIBBRITANNIACOIN_CONSENSUS) \
  $(LIBBRITANNIACOIN_CRYPTO) \
  $(LIBSECP256K1)
#

# britanniacoin-wallet binary #
britanniacoin_wallet_SOURCES = britanniacoin-wallet.cpp
britanniacoin_wallet_SOURCES += init/britanniacoin-wallet.cpp
britanniacoin_wallet_CPPFLAGS = $(britanniacoin_bin_cppflags)
britanniacoin_wallet_CXXFLAGS = $(britanniacoin_bin_cxxflags)
britanniacoin_wallet_LDFLAGS = $(britanniacoin_bin_ldflags)
britanniacoin_wallet_LDADD = \
  $(LIBBRITANNIACOIN_WALLET_TOOL) \
  $(LIBBRITANNIACOIN_WALLET) \
  $(LIBBRITANNIACOIN_COMMON) \
  $(LIBBRITANNIACOIN_UTIL) \
  $(LIBUNIVALUE) \
  $(LIBBRITANNIACOIN_CONSENSUS) \
  $(LIBBRITANNIACOIN_CRYPTO) \
  $(LIBSECP256K1) \
  $(BDB_LIBS) \
  $(SQLITE_LIBS)

if TARGET_WINDOWS
britanniacoin_wallet_SOURCES += britanniacoin-wallet-res.rc
endif
#

# britanniacoin-util binary #
britanniacoin_util_SOURCES = britanniacoin-util.cpp
britanniacoin_util_CPPFLAGS = $(AM_CPPFLAGS) $(BRITANNIACOIN_INCLUDES)
britanniacoin_util_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
britanniacoin_util_LDFLAGS = $(RELDFLAGS) $(AM_LDFLAGS) $(LIBTOOL_APP_LDFLAGS) $(PTHREAD_FLAGS)

if TARGET_WINDOWS
britanniacoin_util_SOURCES += britanniacoin-util-res.rc
endif

britanniacoin_util_LDADD = \
  $(LIBBRITANNIACOIN_COMMON) \
  $(LIBBRITANNIACOIN_UTIL) \
  $(LIBUNIVALUE) \
  $(LIBBRITANNIACOIN_CONSENSUS) \
  $(LIBBRITANNIACOIN_CRYPTO) \
  $(LIBSECP256K1)
#

# britanniacoin-chainstate binary #
britanniacoin_chainstate_SOURCES = britanniacoin-chainstate.cpp
britanniacoin_chainstate_CPPFLAGS = $(AM_CPPFLAGS) $(BRITANNIACOIN_INCLUDES) $(BOOST_CPPFLAGS)
britanniacoin_chainstate_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)

# $(LIBTOOL_APP_LDFLAGS) deliberately omitted here so that we can test linking
# britanniacoin-chainstate against libbritanniacoinkernel as a shared or static library by
# setting --{en,dis}able-shared.
britanniacoin_chainstate_LDFLAGS = $(RELDFLAGS) $(AM_LDFLAGS) $(PTHREAD_FLAGS)
britanniacoin_chainstate_LDADD = $(LIBBRITANNIACOINKERNEL)
#

# britanniacoinkernel library #
if BUILD_BRITANNIACOIN_KERNEL_LIB
lib_LTLIBRARIES += $(LIBBRITANNIACOINKERNEL)

libbritanniacoinkernel_la_LDFLAGS = $(AM_LDFLAGS) -no-undefined $(RELDFLAGS) $(PTHREAD_FLAGS)
libbritanniacoinkernel_la_LIBADD = $(LIBBRITANNIACOIN_CRYPTO) $(LIBUNIVALUE) $(LIBLEVELDB) $(LIBMEMENV) $(LIBSECP256K1)
libbritanniacoinkernel_la_CPPFLAGS = $(AM_CPPFLAGS) -I$(builddir)/obj -I$(srcdir)/secp256k1/include -DBUILD_BRITANNIACOIN_INTERNAL $(BOOST_CPPFLAGS) $(LEVELDB_CPPFLAGS) -I$(srcdir)/$(UNIVALUE_INCLUDE_DIR_INT)

# libbritanniacoinkernel requires default symbol visibility, explicitly specify that
# here so that things still work even when user configures with
#   --enable-reduce-exports
#
# Note this is a quick hack that will be removed as we incrementally define what
# to export from the library.
libbritanniacoinkernel_la_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS) -fvisibility=default

# TODO: For now, Specify -static in both CXXFLAGS and LDFLAGS when building for
#       windows targets so libtool will only build a static version of this
#       library. There are unresolved problems when building dll's for mingw-w64
#       and attempting to statically embed libstdc++, libpthread, etc.
if TARGET_WINDOWS
libbritanniacoinkernel_la_LDFLAGS += -static
libbritanniacoinkernel_la_CXXFLAGS += -static
endif

# TODO: libbritanniacoinkernel is a work in progress consensus engine library, as more
#       and more modules are decoupled from the consensus engine, this list will
#       shrink to only those which are absolutely necessary.
libbritanniacoinkernel_la_SOURCES = \
  kernel/britanniacoinkernel.cpp \
  arith_uint256.cpp \
  chain.cpp \
  chainparamsbase.cpp \
  chainparams.cpp \
  clientversion.cpp \
  coins.cpp \
  compressor.cpp \
  consensus/merkle.cpp \
  consensus/tx_check.cpp \
  consensus/tx_verify.cpp \
  core_read.cpp \
  dbwrapper.cpp \
  deploymentinfo.cpp \
  deploymentstatus.cpp \
  flatfile.cpp \
  fs.cpp \
  hash.cpp \
  kernel/chain.cpp \
  kernel/checks.cpp \
  kernel/coinstats.cpp \
  kernel/context.cpp \
  kernel/cs_main.cpp \
  kernel/mempool_persist.cpp \
  key.cpp \
  logging.cpp \
  node/blockstorage.cpp \
  node/chainstate.cpp \
  node/interface_ui.cpp \
  node/utxo_snapshot.cpp \
  policy/feerate.cpp \
  policy/fees.cpp \
  policy/packages.cpp \
  policy/policy.cpp \
  policy/rbf.cpp \
  policy/settings.cpp \
  pow.cpp \
  primitives/block.cpp \
  primitives/transaction.cpp \
  pubkey.cpp \
  random.cpp \
  randomenv.cpp \
  scheduler.cpp \
  script/interpreter.cpp \
  script/script.cpp \
  script/script_error.cpp \
  script/sigcache.cpp \
  script/standard.cpp \
  shutdown.cpp \
  signet.cpp \
  support/cleanse.cpp \
  support/lockedpool.cpp \
  sync.cpp \
  txdb.cpp \
  txmempool.cpp \
  uint256.cpp \
  util/check.cpp \
  util/getuniquepath.cpp \
  util/hasher.cpp \
  util/moneystr.cpp \
  util/rbf.cpp \
  util/serfloat.cpp \
  util/settings.cpp \
  util/strencodings.cpp \
  util/string.cpp \
  util/syscall_sandbox.cpp \
  util/syserror.cpp \
  util/system.cpp \
  util/thread.cpp \
  util/threadnames.cpp \
  util/time.cpp \
  util/tokenpipe.cpp \
  validation.cpp \
  validationinterface.cpp \
  versionbits.cpp \
  warnings.cpp

# Required for obj/build.h to be generated first.
# More details: https://www.gnu.org/software/automake/manual/html_node/Built-Sources-Example.html
libbritanniacoinkernel_la-clientversion.l$(OBJEXT): obj/build.h
endif # BUILD_BRITANNIACOIN_KERNEL_LIB
#

# britanniacoinconsensus library #
if BUILD_BRITANNIACOIN_LIBS
lib_LTLIBRARIES += $(LIBBRITANNIACOINCONSENSUS)

include_HEADERS = script/britanniacoinconsensus.h
libbritanniacoinconsensus_la_SOURCES = support/cleanse.cpp $(crypto_libbritanniacoin_crypto_base_la_SOURCES) $(libbritanniacoin_consensus_a_SOURCES)

libbritanniacoinconsensus_la_LDFLAGS = $(AM_LDFLAGS) -no-undefined $(RELDFLAGS)
libbritanniacoinconsensus_la_LIBADD = $(LIBSECP256K1)
libbritanniacoinconsensus_la_CPPFLAGS = $(AM_CPPFLAGS) -I$(builddir)/obj -I$(srcdir)/secp256k1/include -DBUILD_BRITANNIACOIN_INTERNAL
libbritanniacoinconsensus_la_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)

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

EXTRA_DIST = $(CTAES_DIST)


config/britanniacoin-config.h: config/stamp-h1
	@$(MAKE) -C $(top_builddir) $(subdir)/$(@)
config/stamp-h1: $(top_srcdir)/$(subdir)/config/britanniacoin-config.h.in $(top_builddir)/config.status
	$(AM_V_at)$(MAKE) -C $(top_builddir) $(subdir)/$(@)
$(top_srcdir)/$(subdir)/config/britanniacoin-config.h.in:  $(am__configure_deps)
	$(AM_V_at)$(MAKE) -C $(top_srcdir) $(subdir)/config/britanniacoin-config.h.in

clean-local:
	-$(MAKE) -C secp256k1 clean
	-rm -f leveldb/*/*.gcda leveldb/*/*.gcno leveldb/helpers/memenv/*.gcda leveldb/helpers/memenv/*.gcno
	-rm -f config.h
	-rm -rf test/__pycache__

.rc.o:
	@test -f $(WINDRES)
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

libbritanniacoin_ipc_mpgen_input = \
  ipc/capnp/echo.capnp \
  ipc/capnp/init.capnp
EXTRA_DIST += $(libbritanniacoin_ipc_mpgen_input)
%.capnp:

# Explicitly list dependencies on generated headers as described in
# https://www.gnu.org/software/automake/manual/html_node/Built-Sources-Example.html#Recording-Dependencies-manually
ipc/capnp/libbritanniacoin_ipc_a-protocol.$(OBJEXT): $(libbritanniacoin_ipc_mpgen_input:=.h)

if BUILD_MULTIPROCESS
LIBBRITANNIACOIN_IPC=libbritanniacoin_ipc.a
libbritanniacoin_ipc_a_SOURCES = \
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
libbritanniacoin_ipc_a_CPPFLAGS = $(AM_CPPFLAGS) $(BRITANNIACOIN_INCLUDES)
libbritanniacoin_ipc_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS) $(LIBMULTIPROCESS_CFLAGS)

include $(MPGEN_PREFIX)/include/mpgen.mk
libbritanniacoin_ipc_mpgen_output = \
  $(libbritanniacoin_ipc_mpgen_input:=.c++) \
  $(libbritanniacoin_ipc_mpgen_input:=.h) \
  $(libbritanniacoin_ipc_mpgen_input:=.proxy-client.c++) \
  $(libbritanniacoin_ipc_mpgen_input:=.proxy-server.c++) \
  $(libbritanniacoin_ipc_mpgen_input:=.proxy-types.c++) \
  $(libbritanniacoin_ipc_mpgen_input:=.proxy-types.h) \
  $(libbritanniacoin_ipc_mpgen_input:=.proxy.h)
nodist_libbritanniacoin_ipc_a_SOURCES = $(libbritanniacoin_ipc_mpgen_output)
CLEANFILES += $(libbritanniacoin_ipc_mpgen_output)
endif

%.raw.h: %.raw
	@$(MKDIR_P) $(@D)
	@{ \
	 echo "static unsigned const char $(*F)_raw[] = {" && \
	 $(HEXDUMP) -v -e '8/1 "0x%02x, "' -e '"\n"' $< | $(SED) -e 's/0x  ,//g' && \
	 echo "};"; \
	} > "$@.new" && mv -f "$@.new" "$@"
	@echo "Generated $@"

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
