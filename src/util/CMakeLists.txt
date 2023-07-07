# Copyright (c) 2023 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

add_library(bitcoin_util STATIC EXCLUDE_FROM_ALL
  asmap.cpp
  batchpriority.cpp
  bip32.cpp
  bytevectorhash.cpp
  chaintype.cpp
  check.cpp
  error.cpp
  exception.cpp
  fees.cpp
  fs.cpp
  fs_helpers.cpp
  getuniquepath.cpp
  hasher.cpp
  message.cpp
  moneystr.cpp
  rbf.cpp
  readwritefile.cpp
  serfloat.cpp
  signalinterrupt.cpp
  sock.cpp
  spanparsing.cpp
  strencodings.cpp
  string.cpp
  syserror.cpp
  thread.cpp
  threadinterrupt.cpp
  threadnames.cpp
  time.cpp
  tokenpipe.cpp
  ../chainparamsbase.cpp
  ../clientversion.cpp
  ../logging.cpp
  ../random.cpp
  ../randomenv.cpp
  ../support/cleanse.cpp
  ../support/lockedpool.cpp
  ../sync.cpp
)

target_compile_definitions(bitcoin_util
  PRIVATE
    ${THREAD_LOCAL_IF_AVAILABLE}
    $<$<CXX_COMPILER_ID:MSVC>:_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING>
)

target_link_libraries(bitcoin_util
  PRIVATE
    bitcoin_crypto
    univalue
    Threads::Threads
    $<TARGET_NAME_IF_EXISTS:std_filesystem>
    $<$<BOOL:${MINGW}>:ws2_32>
)
