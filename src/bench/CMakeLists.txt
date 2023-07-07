# Copyright (c) 2023 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

include(GenerateHeaders)
generate_header_from_raw(data/block413567.raw)

add_executable(bench_bitcoin
  bench_bitcoin.cpp
  bench.cpp
  data.cpp
  nanobench.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/data/block413567.raw.h
# Benchmarks:
  addrman.cpp
  base58.cpp
  bech32.cpp
  bip324_ecdh.cpp
  block_assemble.cpp
  ccoins_caching.cpp
  chacha20.cpp
  chacha_poly_aead.cpp
  checkblock.cpp
  checkqueue.cpp
  crypto_hash.cpp
  descriptors.cpp
  duplicate_inputs.cpp
  ellswift.cpp
  examples.cpp
  gcs_filter.cpp
  hashpadding.cpp
  load_external.cpp
  lockedpool.cpp
  logging.cpp
  mempool_eviction.cpp
  mempool_stress.cpp
  merkle_root.cpp
  peer_eviction.cpp
  poly1305.cpp
  pool.cpp
  prevector.cpp
  rollingbloom.cpp
  rpc_blockchain.cpp
  rpc_mempool.cpp
  streams_findbyte.cpp
  strencodings.cpp
  util_time.cpp
  verify_script.cpp
)

target_link_libraries(bench_bitcoin
  test_util
  leveldb
  univalue
  Boost::headers
)

if(ENABLE_WALLET)
  target_sources(bench_bitcoin
    PRIVATE
      coin_selection.cpp
      wallet_balance.cpp
      wallet_create_tx.cpp
      wallet_loading.cpp
  )
  target_link_libraries(bench_bitcoin bitcoin_wallet)
endif()
