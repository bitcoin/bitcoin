# Copyright (c) 2023-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

add_library(test_util STATIC EXCLUDE_FROM_ALL
  blockfilter.cpp
  coins.cpp
  coverage.cpp
  index.cpp
  json.cpp
  logging.cpp
  mining.cpp
  net.cpp
  random.cpp
  script.cpp
  setup_common.cpp
  str.cpp
  time.cpp
  transaction_utils.cpp
  txmempool.cpp
  validation.cpp
  $<$<BOOL:${ENABLE_WALLET}>:${PROJECT_SOURCE_DIR}/src/wallet/test/util.cpp>
)

target_link_libraries(test_util
  PRIVATE
    core_interface
    Boost::headers
  PUBLIC
    univalue
)
