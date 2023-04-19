# Copyright (c) 2023 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

add_subdirectory(util)

add_executable(fuzz
  addition_overflow.cpp
  addrman.cpp
  asmap.cpp
  asmap_direct.cpp
  autofile.cpp
  banman.cpp
  base_encode_decode.cpp
  bech32.cpp
  bitdeque.cpp
  block.cpp
  block_header.cpp
  blockfilter.cpp
  bloom_filter.cpp
  buffered_file.cpp
  chain.cpp
  checkqueue.cpp
  coins_view.cpp
  connman.cpp
  crypto.cpp
  crypto_aes256.cpp
  crypto_aes256cbc.cpp
  crypto_chacha20.cpp
  crypto_chacha20_poly1305_aead.cpp
  crypto_common.cpp
  crypto_diff_fuzz_chacha20.cpp
  crypto_hkdf_hmac_sha256_l32.cpp
  crypto_poly1305.cpp
  cuckoocache.cpp
  decode_tx.cpp
  descriptor_parse.cpp
  deserialize.cpp
  eval_script.cpp
  fee_rate.cpp
  fees.cpp
  flatfile.cpp
  float.cpp
  golomb_rice.cpp
  hex.cpp
  http_request.cpp
  i2p.cpp
  integer.cpp
  key.cpp
  key_io.cpp
  kitchen_sink.cpp
  load_external_block_file.cpp
  locale.cpp
  merkleblock.cpp
  message.cpp
  miniscript.cpp
  minisketch.cpp
  muhash.cpp
  multiplication_overflow.cpp
  net.cpp
  net_permissions.cpp
  netaddress.cpp
  netbase_dns_lookup.cpp
  node_eviction.cpp
  p2p_transport_serialization.cpp
  parse_hd_keypath.cpp
  parse_numbers.cpp
  parse_script.cpp
  parse_univalue.cpp
  policy_estimator.cpp
  policy_estimator_io.cpp
  pow.cpp
  prevector.cpp
  primitives_transaction.cpp
  process_message.cpp
  process_messages.cpp
  protocol.cpp
  psbt.cpp
  random.cpp
  rbf.cpp
  rolling_bloom_filter.cpp
  rpc.cpp
  script.cpp
  script_assets_test_minimizer.cpp
  script_descriptor_cache.cpp
  script_flags.cpp
  script_format.cpp
  script_interpreter.cpp
  script_ops.cpp
  script_sigcache.cpp
  script_sign.cpp
  scriptnum_ops.cpp
  secp256k1_ec_seckey_import_export_der.cpp
  secp256k1_ecdsa_signature_parse_der_lax.cpp
  signature_checker.cpp
  signet.cpp
  socks5.cpp
  span.cpp
  spanparsing.cpp
  string.cpp
  strprintf.cpp
  system.cpp
  timedata.cpp
  torcontrol.cpp
  transaction.cpp
  tx_in.cpp
  tx_out.cpp
  tx_pool.cpp
  txorphan.cpp
  txrequest.cpp
  utxo_snapshot.cpp
  validation_load_mempool.cpp
  versionbits.cpp
)
target_link_libraries(fuzz
  test_fuzz
  bitcoin_cli
  bitcoin_common
  minisketch
  leveldb
  univalue
  secp256k1
  Boost::headers
  libevent::libevent
)

if(BUILD_BITCOINCONSENSUS_LIB)
  target_sources(fuzz
    PRIVATE
      script_bitcoin_consensus.cpp
  )
  if(TARGET bitcoinconsensus_static)
    target_link_libraries(fuzz bitcoinconsensus_static)
  elseif(TARGET bitcoinconsensus AND NOT WIN32)
    target_link_libraries(fuzz bitcoinconsensus)
  endif()
endif()

if(ENABLE_WALLET)
  target_sources(fuzz
    PRIVATE
      ${CMAKE_SOURCE_DIR}/src/wallet/test/fuzz/coinselection.cpp
      ${CMAKE_SOURCE_DIR}/src/wallet/test/fuzz/parse_iso8601.cpp
      $<$<BOOL:${USE_SQLITE}>:${CMAKE_SOURCE_DIR}/src/wallet/test/fuzz/notifications.cpp>
  )
  target_link_libraries(fuzz bitcoin_wallet)
endif()

set(builtin_mul_overflow_source "
  bool f(long long x, long long y, long long* p)
  {
    return __builtin_mul_overflow(x, y, p);
  }
  int main() { return 0; }
")
include(CheckCXXSourceCompiles)
check_cxx_source_compiles("${builtin_mul_overflow_source}" HAVE_BUILTIN_MUL_OVERFLOW)
if(HAVE_BUILTIN_MUL_OVERFLOW)
  target_compile_definitions(fuzz PRIVATE HAVE_BUILTIN_MUL_OVERFLOW)
elseif(CMAKE_VERSION VERSION_GREATER_EQUAL 3.14)
  set(CMAKE_REQUIRED_LINK_OPTIONS rtlib=compiler-rt)
  set(CMAKE_REQUIRED_LIBRARIES gcc_s)
  check_cxx_source_compiles("${builtin_mul_overflow_source}" HAVE_BUILTIN_MUL_OVERFLOW_NEEDS_LINK_TO_RT)
  if(HAVE_BUILTIN_MUL_OVERFLOW_NEEDS_LINK_TO_RT)
    target_compile_definitions(fuzz PRIVATE HAVE_BUILTIN_MUL_OVERFLOW)
    target_link_options(fuzz PRIVATE ${CMAKE_REQUIRED_LINK_OPTIONS})
    target_link_libraries(fuzz PRIVATE ${CMAKE_REQUIRED_LIBRARIES})
  endif()
  set(CMAKE_REQUIRED_LINK_OPTIONS)
  set(CMAKE_REQUIRED_LIBRARIES)
endif()
