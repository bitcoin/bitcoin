# Copyright (c) 2013-2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# Pattern rule to print variables, e.g. make print-top_srcdir
print-%:
	@echo '$*' = '$($*)'

DIST_SUBDIRS = secp256k1 univalue

AM_LDFLAGS = $(LIBTOOL_LDFLAGS) $(HARDENED_LDFLAGS) $(GPROF_LDFLAGS) $(SANITIZER_LDFLAGS)
AM_CXXFLAGS = $(DEBUG_CXXFLAGS) $(HARDENED_CXXFLAGS) $(WARN_CXXFLAGS) $(NOWARN_CXXFLAGS) $(ERROR_CXXFLAGS) $(GPROF_CXXFLAGS) $(SANITIZER_CXXFLAGS)
AM_CPPFLAGS = $(DEBUG_CPPFLAGS) $(HARDENED_CPPFLAGS)
AM_LIBTOOLFLAGS = --preserve-dup-deps
PTHREAD_FLAGS = $(PTHREAD_CFLAGS) $(PTHREAD_LIBS)
EXTRA_LIBRARIES =

if EMBEDDED_UNIVALUE
LIBUNIVALUE = univalue/libunivalue.la

$(LIBUNIVALUE): $(wildcard univalue/lib/*) $(wildcard univalue/include/*)
	$(AM_V_at)$(MAKE) $(AM_MAKEFLAGS) -C $(@D) $(@F)
else
LIBUNIVALUE = $(UNIVALUE_LIBS)
endif

SYSCOIN_INCLUDES=-I$(builddir) -I$(srcdir)/secp256k1/include $(BDB_CPPFLAGS) $(BOOST_CPPFLAGS) $(LEVELDB_CPPFLAGS)

SYSCOIN_INCLUDES += $(UNIVALUE_CFLAGS)
SYSCOIN_INCLUDES += $(LIBCHIABLS_CFLAGS)
SYSCOIN_INCLUDES += $(LIBCURL_CFLAGS)



if EMBEDDED_CHIABLS
LIBCHIA = bls-dash/build/libbls-dash.a
LIBCHIABLS =$(LIBCURL_LIBS) $(LIBCHIA) -lgmp
$(LIBCHIA):
	cd $(srcdir)/bls-dash/build && $(MAKE) $(AM_MAKEFLAGS)
else
LIBCHIABLS =$(LIBCURL_LIBS) $(LIBCHIABLS_LIBS) -lgmp
endif

LIBSYSCOIN_SERVER=libsyscoin_server.a
LIBSYSCOIN_COMMON=libsyscoin_common.a
LIBSYSCOIN_CONSENSUS=libsyscoin_consensus.a
LIBSYSCOIN_CLI=libsyscoin_cli.a
LIBSYSCOIN_UTIL=libsyscoin_util.a
LIBSYSCOIN_CRYPTO_BASE=crypto/libsyscoin_crypto_base.a
LIBSYSCOINQT=qt/libsyscoinqt.a
LIBSECP256K1=secp256k1/libsecp256k1.la
LIBNEVM=nevm/libnevm.a
LIBSYSCOIN_BLS=libsyscoin_bls.a
if ENABLE_ZMQ
LIBSYSCOIN_ZMQ=libsyscoin_zmq.a
endif
if BUILD_SYSCOIN_LIBS
LIBSYSCOINCONSENSUS=libsyscoinconsensus.la
endif
if ENABLE_WALLET
LIBSYSCOIN_WALLET=libsyscoin_wallet.a
LIBSYSCOIN_WALLET_TOOL=libsyscoin_wallet_tool.a
endif

LIBSYSCOIN_CRYPTO= $(LIBSYSCOIN_CRYPTO_BASE)
if ENABLE_SSE41
LIBSYSCOIN_CRYPTO_SSE41 = crypto/libsyscoin_crypto_sse41.a
LIBSYSCOIN_CRYPTO += $(LIBSYSCOIN_CRYPTO_SSE41)
endif
if ENABLE_AVX2
LIBSYSCOIN_CRYPTO_AVX2 = crypto/libsyscoin_crypto_avx2.a
LIBSYSCOIN_CRYPTO += $(LIBSYSCOIN_CRYPTO_AVX2)
endif
if ENABLE_SHANI
LIBSYSCOIN_CRYPTO_SHANI = crypto/libsyscoin_crypto_shani.a
LIBSYSCOIN_CRYPTO += $(LIBSYSCOIN_CRYPTO_SHANI)
endif

$(LIBSECP256K1): $(wildcard secp256k1/src/*.h) $(wildcard secp256k1/src/*.c) $(wildcard secp256k1/include/*)
	$(AM_V_at)$(MAKE) $(AM_MAKEFLAGS) -C $(@D) $(@F)

# Make is not made aware of per-object dependencies to avoid limiting building parallelization
# But to build the less dependent modules first, we manually select their order here:
EXTRA_LIBRARIES += \
  $(LIBSYSCOIN_CRYPTO) \
  $(LIBSYSCOIN_UTIL) \
  $(LIBSYSCOIN_COMMON) \
  $(LIBSYSCOIN_CONSENSUS) \
  $(LIBSYSCOIN_SERVER) \
  $(LIBSYSCOIN_CLI) \
  $(LIBSYSCOIN_IPC) \
  $(LIBSYSCOIN_WALLET) \
  $(LIBSYSCOIN_WALLET_TOOL) \
  $(LIBSYSCOIN_ZMQ) \
  $(LIBNEVM) \
  $(LIBSYSCOIN_BLS)

lib_LTLIBRARIES = $(LIBSYSCOINCONSENSUS)

bin_PROGRAMS =
noinst_PROGRAMS =
TESTS =
BENCHMARKS =

if BUILD_SYSCOIND
  bin_PROGRAMS += syscoind
endif

if BUILD_SYSCOIN_NODE
  bin_PROGRAMS += syscoin-node
endif

if BUILD_SYSCOIN_CLI
  bin_PROGRAMS += syscoin-cli
endif

if BUILD_SYSCOIN_TX
  bin_PROGRAMS += syscoin-tx
endif

if ENABLE_WALLET
if BUILD_SYSCOIN_WALLET
  bin_PROGRAMS += syscoin-wallet
endif
endif

if BUILD_SYSCOIN_UTIL
  bin_PROGRAMS += syscoin-util
endif

.PHONY: FORCE check-symbols check-security
# syscoin core #
SYSCOIN_CORE_H = \
  services/asset.h \
  services/assetconsensus.h \
  services/rpc/assetrpc.h \
  services/rpc/wallet/assetwalletrpc.h \
  spork.h \
  dsnotificationinterface.h \
  governance/governance.h \
  governance/governanceclasses.h \
  governance/governanceexceptions.h \
  governance/governanceobject.h \
  governance/governancevalidators.h \
  governance/governancevote.h \
  governance/governancevotedb.h \
  concurrentqueue.h \
  ctpl.h \
  cxxtimer.hpp \
  evo/cbtx.h \
  evo/deterministicmns.h \
  evo/evodb.h \
  evo/mnauth.h \
  evo/providertx.h \
  evo/simplifiedmns.h \
  evo/specialtx.h \
  masternode/activemasternode.h \
  masternode/masternodemeta.h \
  masternode/masternodepayments.h \
  masternode/masternodesync.h \
  masternode/masternodeutils.h \
  flatdatabase.h \
  messagesigner.h \
  netfulfilledman.h \
  cachemap.h \
  cachemultimap.h \
  addrdb.h \
  addrman.h \
  attributes.h \
  auxpow.h \
  banman.h \
  base58.h \
  batchedlogger.h \
  bech32.h \
  blockencodings.h \
  blockfilter.h \
  bloom.h \
  chain.h \
  chainparams.h \
  chainparamsbase.h \
  chainparamsseeds.h \
  checkqueue.h \
  clientversion.h \
  coins.h \
  compat.h \
  compat/assumptions.h \
  compat/byteswap.h \
  compat/cpuid.h \
  compat/endian.h \
  compat/sanity.h \
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
  key.h \
  key_io.h \
  llmq/quorums.h \
  llmq/quorums_blockprocessor.h \
  llmq/quorums_commitment.h \
  llmq/quorums_chainlocks.h \
  llmq/quorums_debug.h \
  llmq/quorums_dkgsessionhandler.h \
  llmq/quorums_dkgsessionmgr.h \
  llmq/quorums_dkgsession.h \
  llmq/quorums_init.h \
  llmq/quorums_signing.h \
  llmq/quorums_signing_shares.h \
  llmq/quorums_utils.h \
  logging.h \
  logging/timer.h \
  mapport.h \
  memusage.h \
  merkleblock.h \
  miner.h \
  net.h \
  net_permissions.h \
  net_processing.h \
  net_types.h \
  netaddress.h \
  netbase.h \
  netmessagemaker.h \
  node/blockstorage.h \
  node/coin.h \
  node/coinstats.h \
  node/context.h \
  node/psbt.h \
  node/transaction.h \
  node/ui_interface.h \
  node/utxo_snapshot.h \
  noui.h \
  outputtype.h \
  policy/feerate.h \
  policy/fees.h \
  policy/packages.h \
  policy/policy.h \
  policy/rbf.h \
  policy/settings.h \
  pow.h \
  protocol.h \
  psbt.h \
  random.h \
  randomenv.h \
  reverse_iterator.h \
  rpc/blockchain.h \
  rpc/auxpow_miner.h \
  rpc/client.h \
  rpc/mining.h \
  rpc/net.h \
  rpc/protocol.h \
  rpc/rawtransaction_util.h \
  rpc/register.h \
  rpc/request.h \
  rpc/server.h \
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
  signet.h \
  streams.h \
  support/allocators/secure.h \
  support/allocators/zeroafterfree.h \
  support/allocators/pooled_secure.h \
  support/allocators/mt_pooled_secure.h \
  support/cleanse.h \
  support/events.h \
  support/lockedpool.h \
  sync.h \
  threadinterrupt.h \
  threadsafety.h \
  timedata.h \
  torcontrol.h \
  txdb.h \
  txmempool.h \
  txorphanage.h \
  txrequest.h \
  undo.h \
  unordered_lru_cache.h \
  util/asmap.h \
  util/bip32.h \
  util/bytevectorhash.h \
  util/check.h \
  util/epochguard.h \
  util/error.h \
  util/fees.h \
  util/getuniquepath.h \
  util/golombrice.h \
  util/hash_type.h \
  util/hasher.h \
  util/macros.h \
  util/message.h \
  util/moneystr.h \
  util/rbf.h \
  util/readwritefile.h \
  util/serfloat.h \
  util/settings.h \
  util/sock.h \
  util/spanparsing.h \
  util/string.h \
  util/system.h \
  util/thread.h \
  util/threadnames.h \
  util/time.h \
  util/tokenpipe.h \
  util/trace.h \
  util/translation.h \
  util/ui_change_type.h \
  util/url.h \
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
  wallet/rpcwallet.h \
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

 # nevm
 nevm_libnevm_a_CPPFLAGS = $(AM_CPPFLAGS) $(SYSCOIN_INCLUDES)
 nevm_libnevm_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
 nevm_libnevm_a_SOURCES = \
  nevm/address.cpp \
  nevm/address.h \
  nevm/common.cpp \
  nevm/common.h \
  nevm/commondata.cpp \
  nevm/commondata.h \
  nevm/exceptions.h \
  nevm/fixedhash.cpp \
  nevm/fixedhash.h \
  nevm/rlp.cpp \
  nevm/rlp.h \
  nevm/sha3.cpp \
  nevm/sha3.h \
  nevm/nevm.cpp \
  nevm/nevm.h \
  nevm/vector_ref.h


 # BLS
 libsyscoin_bls_a_CPPFLAGS = $(AM_CPPFLAGS) $(SYSCOIN_INCLUDES)
 libsyscoin_bls_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
 libsyscoin_bls_a_SOURCES = \
  bls/bls.h \
  bls/bls_batchverifier.h \
  bls/bls_ies.h \
  bls/bls_worker.h \
  bls/bls.cpp \
  bls/bls_ies.cpp \
  bls/bls_worker.cpp \
  bls-dash/src/bls.hpp \
  bls-dash/src/bls.cpp \
  bls-dash/src/privatekey.hpp \
  bls-dash/src/privatekey.cpp \
  bls-dash/src/chaincode.hpp \
  bls-dash/src/chaincode.cpp \
  bls-dash/src/extendedprivatekey.hpp \
  bls-dash/src/extendedprivatekey.cpp \
  bls-dash/src/extendedpublickey.hpp \
  bls-dash/src/extendedpublickey.cpp \
  bls-dash/src/util.hpp \
  bls-dash/src/elements.hpp \
  bls-dash/src/elements.cpp \
  bls-dash/src/hdkeys.hpp \
  bls-dash/src/hkdf.hpp \
  bls-dash/src/legacy.hpp \
  bls-dash/src/legacy.cpp \
  bls-dash/src/schemes.hpp \
  bls-dash/src/schemes.cpp \
  bls-dash/src/threshold.hpp \
  bls-dash/src/threshold.cpp \
  $(SYSCOIN_CORE_H)


# server: shared between syscoind and syscoin-qt
libsyscoin_util_a-clientversion.$(OBJEXT): obj/build.h

ipc/capnp/libsyscoin_ipc_a-ipc.$(OBJEXT): $(libsyscoin_ipc_mpgen_input:=.h)

# server: shared between syscoind and syscoin-qt
# Contains code accessing mempool and chain state that is meant to be separated
# from wallet and gui code (see node/README.md). Shared code should go in
# libsyscoin_common or libsyscoin_util libraries, instead.
libsyscoin_server_a_CPPFLAGS = $(AM_CPPFLAGS) $(SYSCOIN_INCLUDES) $(MINIUPNPC_CPPFLAGS) $(NATPMP_CPPFLAGS) $(EVENT_CFLAGS) $(EVENT_PTHREADS_CFLAGS)
libsyscoin_server_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libsyscoin_server_a_SOURCES = \
  services/asset.cpp \
  services/assetconsensus.cpp \
  services/rpc/assetrpc.cpp \
  core_write.cpp \
  dsnotificationinterface.cpp \
  governance/governance.cpp \
  governance/governanceclasses.cpp \
  governance/governanceobject.cpp \
  governance/governancevalidators.cpp \
  governance/governancevote.cpp \
  governance/governancevotedb.cpp \
  evo/cbtx.cpp \
  evo/deterministicmns.cpp \
  evo/evodb.cpp \
  evo/mnauth.cpp \
  evo/providertx.cpp \
  evo/simplifiedmns.cpp \
  evo/specialtx.cpp \
  llmq/quorums.cpp \
  llmq/quorums_blockprocessor.cpp \
  llmq/quorums_commitment.cpp \
  llmq/quorums_chainlocks.cpp \
  llmq/quorums_debug.cpp \
  llmq/quorums_dkgsessionhandler.cpp \
  llmq/quorums_dkgsessionmgr.cpp \
  llmq/quorums_dkgsession.cpp \
  llmq/quorums_init.cpp \
  llmq/quorums_signing.cpp \
  llmq/quorums_signing_shares.cpp \
  llmq/quorums_utils.cpp \
  masternode/activemasternode.cpp \
  masternode/masternodemeta.cpp \
  masternode/masternodepayments.cpp \
  masternode/masternodesync.cpp \
  masternode/masternodeutils.cpp \
  messagesigner.cpp \
  spork.cpp \
  addrdb.cpp \
  addrman.cpp \
  batchedlogger.cpp \
  banman.cpp \
  blockencodings.cpp \
  blockfilter.cpp \
  chain.cpp \
  consensus/tx_verify.cpp \
  dbwrapper.cpp \
  deploymentstatus.cpp \
  flatfile.cpp \
  httprpc.cpp \
  httpserver.cpp \
  i2p.cpp \
  index/base.cpp \
  index/blockfilterindex.cpp \
  index/coinstatsindex.cpp \
  index/txindex.cpp \
  init.cpp \
  mapport.cpp \
  miner.cpp \
  net.cpp \
  netfulfilledman.cpp \
  net_processing.cpp \
  node/blockstorage.cpp \
  node/coin.cpp \
  node/coinstats.cpp \
  node/context.cpp \
  node/interfaces.cpp \
  node/psbt.cpp \
  node/transaction.cpp \
  node/ui_interface.cpp \
  noui.cpp \
  policy/fees.cpp \
  policy/packages.cpp \
  policy/rbf.cpp \
  policy/settings.cpp \
  primitives/transaction.cpp \
  pow.cpp \
  rest.cpp \
  rpc/auxpow_miner.cpp \
  rpc/blockchain.cpp \
  rpc/mining.cpp \
  rpc/misc.cpp \
  rpc/net.cpp \
  rpc/rawtransaction.cpp \
  rpc/server.cpp \
  rpc/masternode.cpp \
  rpc/governance.cpp \
  rpc/rpcevo.cpp \
  rpc/rpcquorums.cpp \
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
  $(SYSCOIN_CORE_H)

if ENABLE_WALLET
libsyscoin_server_a_SOURCES += wallet/init.cpp
endif
if !ENABLE_WALLET
libsyscoin_server_a_SOURCES += dummywallet.cpp
endif

if ENABLE_ZMQ
libsyscoin_zmq_a_CPPFLAGS = $(AM_CPPFLAGS) $(SYSCOIN_INCLUDES) $(ZMQ_CFLAGS)
libsyscoin_zmq_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libsyscoin_zmq_a_SOURCES = \
  zmq/zmqabstractnotifier.cpp \
  zmq/zmqnotificationinterface.cpp \
  zmq/zmqpublishnotifier.cpp \
  zmq/zmqrpc.cpp \
  zmq/zmqutil.cpp
endif


# wallet: shared between syscoind and syscoin-qt, but only linked
# when wallet enabled
libsyscoin_wallet_a_CPPFLAGS = $(AM_CPPFLAGS) $(SYSCOIN_INCLUDES) $(SQLITE_CFLAGS)
libsyscoin_wallet_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libsyscoin_wallet_a_SOURCES = \
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
  wallet/rpcdump.cpp \
  wallet/rpcwallet.cpp \
  wallet/scriptpubkeyman.cpp \
  wallet/spend.cpp \
  wallet/transaction.cpp \
  wallet/wallet.cpp \
  wallet/walletdb.cpp \
  wallet/walletutil.cpp \
  wallet/coinselection.cpp \
  rpc/governance.cpp \
  rpc/rawtransaction.cpp \
  rpc/rpcevo.cpp \
  rpc/masternode.cpp \
  rpc/auxpow_miner.cpp \
  services/rpc/wallet/assetwalletrpc.cpp \
  wallet/rpcevo.cpp \
  wallet/rpcgovernance.cpp \
  wallet/rpcmasternode.cpp \
  $(SYSCOIN_CORE_H)

if USE_SQLITE
libsyscoin_wallet_a_SOURCES += wallet/sqlite.cpp
endif
if USE_BDB
libsyscoin_wallet_a_SOURCES += wallet/bdb.cpp wallet/salvage.cpp
endif

libsyscoin_wallet_tool_a_CPPFLAGS = $(AM_CPPFLAGS) $(SYSCOIN_INCLUDES)
libsyscoin_wallet_tool_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libsyscoin_wallet_tool_a_SOURCES = \
  wallet/wallettool.cpp \
  $(SYSCOIN_CORE_H)

# crypto primitives library
crypto_libsyscoin_crypto_base_a_CPPFLAGS = $(AM_CPPFLAGS)
crypto_libsyscoin_crypto_base_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
crypto_libsyscoin_crypto_base_a_SOURCES = \
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
crypto_libsyscoin_crypto_base_a_SOURCES += crypto/sha256_sse4.cpp
endif

crypto_libsyscoin_crypto_sse41_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
crypto_libsyscoin_crypto_sse41_a_CPPFLAGS = $(AM_CPPFLAGS)
crypto_libsyscoin_crypto_sse41_a_CXXFLAGS += $(SSE41_CXXFLAGS)
crypto_libsyscoin_crypto_sse41_a_CPPFLAGS += -DENABLE_SSE41
crypto_libsyscoin_crypto_sse41_a_SOURCES = crypto/sha256_sse41.cpp

crypto_libsyscoin_crypto_avx2_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
crypto_libsyscoin_crypto_avx2_a_CPPFLAGS = $(AM_CPPFLAGS)
crypto_libsyscoin_crypto_avx2_a_CXXFLAGS += $(AVX2_CXXFLAGS)
crypto_libsyscoin_crypto_avx2_a_CPPFLAGS += -DENABLE_AVX2
crypto_libsyscoin_crypto_avx2_a_SOURCES = crypto/sha256_avx2.cpp

crypto_libsyscoin_crypto_shani_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
crypto_libsyscoin_crypto_shani_a_CPPFLAGS = $(AM_CPPFLAGS)
crypto_libsyscoin_crypto_shani_a_CXXFLAGS += $(SHANI_CXXFLAGS)
crypto_libsyscoin_crypto_shani_a_CPPFLAGS += -DENABLE_SHANI
crypto_libsyscoin_crypto_shani_a_SOURCES = crypto/sha256_shani.cpp

# consensus: shared between all executables that validate any consensus rules.
libsyscoin_consensus_a_CPPFLAGS = $(AM_CPPFLAGS) $(SYSCOIN_INCLUDES)
libsyscoin_consensus_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libsyscoin_consensus_a_SOURCES = \
  amount.h \
  arith_uint256.cpp \
  arith_uint256.h \
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
  primitives/pureheader.cpp \
  primitives/pureheader.h \
  primitives/transaction.cpp \
  primitives/transaction.h \
  pubkey.cpp \
  pubkey.h \
  script/syscoinconsensus.cpp \
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

# common: shared between syscoind, and syscoin-qt and non-server tools
libsyscoin_common_a_CPPFLAGS = $(AM_CPPFLAGS) $(SYSCOIN_INCLUDES)
libsyscoin_common_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libsyscoin_common_a_SOURCES = \
  services/asset.cpp \
  services/assetconsensus.cpp \
  auxpow.cpp \
  base58.cpp \
  bech32.cpp \
  bloom.cpp \
  chainparams.cpp \
  coins.cpp \
  core_read.cpp \
  core_write.cpp \
  dbwrapper.cpp \
  deploymentinfo.cpp \
  external_signer.cpp \
  init/common.cpp \
  key.cpp \
  key_io.cpp \
  merkleblock.cpp \
  messagesigner.cpp \
  netaddress.cpp \
  netbase.cpp \
  net_permissions.cpp \
  outputtype.cpp \
  policy/feerate.cpp \
  policy/policy.cpp \
  protocol.cpp \
  saltedhasher.cpp \
  psbt.cpp \
  rpc/rawtransaction_util.cpp \
  rpc/external_signer.cpp \
  rpc/util.cpp \
  scheduler.cpp \
  script/descriptor.cpp \
  script/sign.cpp \
  script/signingprovider.cpp \
  script/standard.cpp \
  warnings.cpp \
  $(SYSCOIN_CORE_H)

# util: shared between all executables.
# This library *must* be included to make sure that the glibc
# backward-compatibility objects and their sanity checks are linked.
libsyscoin_util_a_CPPFLAGS = $(AM_CPPFLAGS) $(SYSCOIN_INCLUDES)
libsyscoin_util_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libsyscoin_util_a_SOURCES = \
  support/lockedpool.cpp \
  chainparamsbase.cpp \
  clientversion.cpp \
  compat/glibcxx_sanity.cpp \
  compat/strnlen.cpp \
  fs.cpp \
  interfaces/echo.cpp \
  interfaces/handler.cpp \
  interfaces/init.cpp \
  logging.cpp \
  random.cpp \
  randomenv.cpp \
  rpc/request.cpp \
  support/cleanse.cpp \
  sync.cpp \
  threadinterrupt.cpp \
  util/asmap.cpp \
  util/bip32.cpp \
  util/bytevectorhash.cpp \
  util/error.cpp \
  util/fees.cpp \
  util/getuniquepath.cpp \
  util/hasher.cpp \
  util/sock.cpp \
  util/system.cpp \
  util/message.cpp \
  util/moneystr.cpp \
  util/rbf.cpp \
  util/readwritefile.cpp \
  util/settings.cpp \
  util/thread.cpp \
  util/threadnames.cpp \
  util/serfloat.cpp \
  util/spanparsing.cpp \
  util/strencodings.cpp \
  util/string.cpp \
  util/time.cpp \
  util/tokenpipe.cpp \
  $(SYSCOIN_CORE_H)

if USE_LIBEVENT
libsyscoin_util_a_SOURCES += util/url.cpp
endif

if GLIBC_BACK_COMPAT
libsyscoin_util_a_SOURCES += compat/glibc_compat.cpp
AM_LDFLAGS += $(COMPAT_LDFLAGS)
endif

# cli: shared between syscoin-cli and syscoin-qt
libsyscoin_cli_a_CPPFLAGS = $(AM_CPPFLAGS) $(SYSCOIN_INCLUDES)
libsyscoin_cli_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libsyscoin_cli_a_SOURCES = \
  compat/stdin.h \
  compat/stdin.cpp \
  rpc/client.cpp \
  $(SYSCOIN_CORE_H)

if GLIBC_BACK_COMPAT
libsyscoin_cli_a_SOURCES += compat/glibc_compat.cpp
AM_LDFLAGS += $(COMPAT_LDFLAGS)
endif

nodist_libsyscoin_util_a_SOURCES = $(srcdir)/obj/build.h
#

# syscoind & syscoin-node binaries #
syscoin_daemon_sources = syscoind.cpp
syscoin_bin_cppflags = $(AM_CPPFLAGS) $(SYSCOIN_INCLUDES)
syscoin_bin_cxxflags = $(AM_CXXFLAGS) $(PIE_FLAGS)
syscoin_bin_ldflags = $(RELDFLAGS) $(AM_LDFLAGS) $(LIBTOOL_APP_LDFLAGS) $(PTHREAD_FLAGS)

if TARGET_WINDOWS
syscoin_daemon_sources += syscoind-res.rc
endif
# SYSCOIN need getauxwork and mempool stuff inside of wallet so server needs linking here for now but order has to be after wallet
syscoin_bin_ldadd = \
  $(LIBSYSCOIN_WALLET) \
  $(LIBSYSCOIN_SERVER) \
  $(LIBSYSCOIN_BLS) \
  $(LIBSYSCOIN_COMMON) \
  $(LIBSYSCOIN_UTIL) \
  $(LIBCHIABLS) \
  $(LIBUNIVALUE) \
  $(LIBSYSCOIN_ZMQ) \
  $(LIBSYSCOIN_CONSENSUS) \
  $(LIBSYSCOIN_CRYPTO) \
  $(LIBLEVELDB) \
  $(LIBLEVELDB_SSE42) \
  $(LIBMEMENV) \
  $(LIBSECP256K1) \
  $(LIBNEVM)

syscoin_bin_ldadd += $(BOOST_LIBS) $(BDB_LIBS) $(MINIUPNPC_LIBS) $(NATPMP_LIBS) $(EVENT_PTHREADS_LIBS) $(EVENT_LIBS) $(ZMQ_LIBS) $(SQLITE_LIBS)

syscoind_SOURCES = $(syscoin_daemon_sources) init/syscoind.cpp
syscoind_CPPFLAGS = $(syscoin_bin_cppflags)
syscoind_CXXFLAGS = $(syscoin_bin_cxxflags)
syscoind_LDFLAGS = $(syscoin_bin_ldflags)
syscoind_LDADD = $(LIBSYSCOIN_SERVER) $(syscoin_bin_ldadd)

syscoin_node_SOURCES = $(syscoin_daemon_sources) init/syscoin-node.cpp
syscoin_node_CPPFLAGS = $(syscoin_bin_cppflags)
syscoin_node_CXXFLAGS = $(syscoin_bin_cxxflags)
syscoin_node_LDFLAGS = $(syscoin_bin_ldflags)
syscoin_node_LDADD = $(LIBSYSCOIN_SERVER) $(syscoin_bin_ldadd) $(LIBSYSCOIN_IPC) $(LIBMULTIPROCESS_LIBS)

# syscoin-cli binary #
syscoin_cli_SOURCES = syscoin-cli.cpp
syscoin_cli_CPPFLAGS = $(AM_CPPFLAGS) $(SYSCOIN_INCLUDES) $(EVENT_CFLAGS)
syscoin_cli_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
syscoin_cli_LDFLAGS = $(RELDFLAGS) $(AM_LDFLAGS) $(LIBTOOL_APP_LDFLAGS) $(PTHREAD_FLAGS)

if TARGET_WINDOWS
syscoin_cli_SOURCES += syscoin-cli-res.rc
endif


syscoin_cli_LDADD = \
  $(LIBSYSCOIN_CLI) \
  $(LIBUNIVALUE) \
  $(LIBSYSCOIN_UTIL) \
  $(LIBSYSCOIN_CRYPTO)

syscoin_cli_LDADD += $(BOOST_LIBS) $(EVENT_LIBS)
#

# syscoin-tx binary #
syscoin_tx_SOURCES = syscoin-tx.cpp
syscoin_tx_CPPFLAGS = $(AM_CPPFLAGS) $(SYSCOIN_INCLUDES)
syscoin_tx_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
syscoin_tx_LDFLAGS = $(RELDFLAGS) $(AM_LDFLAGS) $(LIBTOOL_APP_LDFLAGS) $(PTHREAD_FLAGS)

if TARGET_WINDOWS
syscoin_tx_SOURCES += syscoin-tx-res.rc
endif

syscoin_tx_LDADD = \
  $(LIBUNIVALUE) \
  $(LIBSYSCOIN_COMMON) \
  $(LIBSYSCOIN_BLS) \
  $(LIBSYSCOIN_UTIL) \
  $(LIBCHIABLS) \
  $(LIBSYSCOIN_CONSENSUS) \
  $(LIBSYSCOIN_CRYPTO) \
  $(LIBLEVELDB) \
  $(LIBLEVELDB_SSE42) \
  $(LIBMEMENV) \
  $(LIBSECP256K1) \
  $(LIBNEVM)

syscoin_tx_LDADD += $(BOOST_LIBS)
  
#

# syscoin-wallet binary #
syscoin_wallet_SOURCES = syscoin-wallet.cpp
syscoin_wallet_CPPFLAGS = $(syscoin_bin_cppflags)
syscoin_wallet_CXXFLAGS = $(syscoin_bin_cxxflags)
syscoin_wallet_LDFLAGS = $(syscoin_bin_ldflags)
syscoin_wallet_LDADD = $(LIBSYSCOIN_WALLET_TOOL) $(syscoin_bin_ldadd)

if TARGET_WINDOWS
syscoin_wallet_SOURCES += syscoin-wallet-res.rc
endif
#

# syscoin-util binary #
syscoin_util_SOURCES = syscoin-util.cpp
syscoin_util_CPPFLAGS = $(AM_CPPFLAGS) $(SYSCOIN_INCLUDES)
syscoin_util_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
syscoin_util_LDFLAGS = $(RELDFLAGS) $(AM_LDFLAGS) $(LIBTOOL_APP_LDFLAGS) $(PTHREAD_FLAGS)

if TARGET_WINDOWS
syscoin_util_SOURCES += syscoin-util-res.rc
endif

syscoin_util_LDADD = \
  $(LIBSYSCOIN_COMMON) \
  $(LIBSYSCOIN_UTIL) \
  $(LIBUNIVALUE) \
  $(LIBSYSCOIN_CONSENSUS) \
  $(LIBSYSCOIN_CRYPTO) \
  $(LIBSECP256K1)

syscoin_util_LDADD += $(BOOST_LIBS)
#

# syscoinconsensus library #
if BUILD_SYSCOIN_LIBS
include_HEADERS = script/syscoinconsensus.h
libsyscoinconsensus_la_SOURCES = support/cleanse.cpp $(crypto_libsyscoin_crypto_base_a_SOURCES) $(libsyscoin_consensus_a_SOURCES)

if GLIBC_BACK_COMPAT
  libsyscoinconsensus_la_SOURCES += compat/glibc_compat.cpp
endif

libsyscoinconsensus_la_LDFLAGS = $(AM_LDFLAGS) -no-undefined $(RELDFLAGS)
libsyscoinconsensus_la_LIBADD = $(LIBSECP256K1)
libsyscoinconsensus_la_CPPFLAGS = $(AM_CPPFLAGS) -I$(builddir)/obj -I$(srcdir)/secp256k1/include -DBUILD_SYSCOIN_INTERNAL $(LEVELDB_CPPFLAGS)
libsyscoinconsensus_la_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)

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

BLSDASH_DIST = bls-dash/CMakeLists.txt
BLSDASH_DIST += bls-dash/README.md
BLSDASH_DIST += bls-dash/build/.configure-custom.sh
BLSDASH_DIST += bls-dash/build/configure
BLSDASH_DIST += bls-dash/cmake_modules
BLSDASH_DIST += bls-dash/contrib
BLSDASH_DIST += bls-dash/src

EXTRA_DIST = $(CTAES_DIST) $(BLSDASH_DIST) immer


config/syscoin-config.h: config/stamp-h1
	@$(MAKE) -C $(top_builddir) $(subdir)/$(@)
config/stamp-h1: $(top_srcdir)/$(subdir)/config/syscoin-config.h.in $(top_builddir)/config.status
	$(AM_V_at)$(MAKE) -C $(top_builddir) $(subdir)/$(@)
$(top_srcdir)/$(subdir)/config/syscoin-config.h.in:  $(am__configure_deps)
	$(AM_V_at)$(MAKE) -C $(top_srcdir) $(subdir)/config/syscoin-config.h.in

clean-local:
	-$(MAKE) -C secp256k1 clean
	-$(MAKE) -C univalue clean
	-rm -f leveldb/*/*.gcda leveldb/*/*.gcno leveldb/helpers/memenv/*.gcda leveldb/helpers/memenv/*.gcno
	-rm -f config.h
	-rm -rf test/__pycache__
	-rm -f bls-dash/build/*.a bls-dash/src/*.o
	-rm -rf bls-dash/build/_deps bls-dash/build/object_bls-dashtmp bls-dash/build/object_relic_s bls-dash/build/object_sodium

.rc.o:
	@test -f $(WINDRES)
	## FIXME: How to get the appropriate modulename_CPPFLAGS in here?
	$(AM_V_GEN) $(WINDRES) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(CPPFLAGS) -DWINDRES_PREPROC -i $< -o $@

check-symbols: $(bin_PROGRAMS)
if TARGET_DARWIN
	@echo "Checking macOS dynamic libraries..."
	$(AM_V_at) OTOOL=$(OTOOL) $(PYTHON) $(top_srcdir)/contrib/devtools/symbol-check.py $(bin_PROGRAMS)
endif

if TARGET_WINDOWS
	@echo "Checking Windows dynamic libraries..."
	$(AM_V_at) OBJDUMP=$(OBJDUMP) $(PYTHON) $(top_srcdir)/contrib/devtools/symbol-check.py $(bin_PROGRAMS)
endif

if TARGET_LINUX
	@echo "Checking glibc back compat..."
	$(AM_V_at) CPPFILT=$(CPPFILT) $(PYTHON) $(top_srcdir)/contrib/devtools/symbol-check.py $(bin_PROGRAMS)
endif

check-security: $(bin_PROGRAMS)
if HARDEN
	@echo "Checking binary security..."
	$(AM_V_at) OBJDUMP=$(OBJDUMP) OTOOL=$(OTOOL) $(PYTHON) $(top_srcdir)/contrib/devtools/security-check.py $(bin_PROGRAMS)
endif

libsyscoin_ipc_mpgen_input = \
  ipc/capnp/echo.capnp \
  ipc/capnp/init.capnp
EXTRA_DIST += $(libsyscoin_ipc_mpgen_input)
%.capnp:

if BUILD_MULTIPROCESS
LIBSYSCOIN_IPC=libsyscoin_ipc.a
libsyscoin_ipc_a_SOURCES = \
  ipc/capnp/init-types.h \
  ipc/capnp/protocol.cpp \
  ipc/capnp/protocol.h \
  ipc/exception.h \
  ipc/interfaces.cpp \
  ipc/process.cpp \
  ipc/process.h \
  ipc/protocol.h
libsyscoin_ipc_a_CPPFLAGS = $(AM_CPPFLAGS) $(SYSCOIN_INCLUDES)
libsyscoin_ipc_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS) $(LIBMULTIPROCESS_CFLAGS)

include $(MPGEN_PREFIX)/include/mpgen.mk
libsyscoin_ipc_mpgen_output = \
  $(libsyscoin_ipc_mpgen_input:=.c++) \
  $(libsyscoin_ipc_mpgen_input:=.h) \
  $(libsyscoin_ipc_mpgen_input:=.proxy-client.c++) \
  $(libsyscoin_ipc_mpgen_input:=.proxy-server.c++) \
  $(libsyscoin_ipc_mpgen_input:=.proxy-types.c++) \
  $(libsyscoin_ipc_mpgen_input:=.proxy-types.h) \
  $(libsyscoin_ipc_mpgen_input:=.proxy.h)
nodist_libsyscoin_ipc_a_SOURCES = $(libsyscoin_ipc_mpgen_output)
CLEANFILES += $(libsyscoin_ipc_mpgen_output)
endif

if EMBEDDED_LEVELDB
include Makefile.crc32c.include
include Makefile.leveldb.include
endif

include Makefile.test_util.include
include Makefile.test_fuzz.include

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