# Copyright (c) 2023-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

add_library(bitcoin_util_kernel STATIC EXCLUDE_FROM_ALL
  chaintype.cpp
  check.cpp
  exception.cpp
  expected.cpp
  feefrac.cpp
  fs.cpp
  fs_helpers.cpp
  hasher.cpp
  moneystr.cpp
  rbf.cpp
  signalinterrupt.cpp
  syserror.cpp
  thread.cpp
  threadnames.cpp
  time.cpp
  tokenpipe.cpp
  ../logging.cpp
  ../random.cpp
  ../randomenv.cpp
  ../streams.cpp
  ../sync.cpp
)

target_link_libraries(bitcoin_util_kernel
  PRIVATE
    core_interface
    bitcoin_clientversion
    bitcoin_crypto
    $<$<PLATFORM_ID:Windows>:ws2_32>
    $<$<PLATFORM_ID:Windows>:iphlpapi>
    $<$<PLATFORM_ID:Windows>:bcrypt>
)

add_library(bitcoin_util STATIC EXCLUDE_FROM_ALL
  asmap.cpp
  batchpriority.cpp
  bip32.cpp
  bytevectorhash.cpp
  exec.cpp
  readwritefile.cpp
  serfloat.cpp
  sock.cpp
  strencodings.cpp
  string.cpp
  threadinterrupt.cpp
)

target_link_libraries(bitcoin_util
  PRIVATE
    core_interface
    bitcoin_clientversion
    bitcoin_crypto
    bitcoin_util_kernel
    $<$<PLATFORM_ID:Windows>:ws2_32>
)
