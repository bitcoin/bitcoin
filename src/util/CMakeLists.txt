# Copyright (c) 2023-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

add_library(bitcoin_kernel_util STATIC EXCLUDE_FROM_ALL
  chaintype.cpp
  check.cpp
  feefrac.cpp
  fs.cpp
  fs_helpers.cpp
  hasher.cpp
  moneystr.cpp
  rbf.cpp
  readwritefile.cpp
  serfloat.cpp
  syserror.cpp
  signalinterrupt.cpp
  strencodings.cpp
  threadnames.cpp
  time.cpp
  tokenpipe.cpp
  ../logging.cpp
  ../random.cpp
  ../randomenv.cpp
  ../streams.cpp
  ../support/lockedpool.cpp
  ../sync.cpp
)

target_link_libraries(bitcoin_kernel_util
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
  exception.cpp
  sock.cpp
  string.cpp
  thread.cpp
  threadinterrupt.cpp
)

target_link_libraries(bitcoin_util
  PRIVATE
    core_interface
    bitcoin_clientversion
    bitcoin_crypto
    bitcoin_kernel_util
    $<$<PLATFORM_ID:Windows>:ws2_32>
)
