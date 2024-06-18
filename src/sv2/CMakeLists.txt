# Copyright (c) 2024-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

add_library(bitcoin_sv2 STATIC EXCLUDE_FROM_ALL
  noise.cpp
  transport.cpp
  connman.cpp
  template_provider.cpp
)

target_link_libraries(bitcoin_sv2
  PRIVATE
    core_interface
    bitcoin_clientversion
    bitcoin_crypto
    bitcoin_common # for SockMan
    $<$<PLATFORM_ID:Windows>:ws2_32>
)
