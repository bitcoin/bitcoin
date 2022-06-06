# Copyright (c) 2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# Home for common functionality shared by different executables and libraries.
# Similar to `bitcoin_common` library, but lower-level.
add_library(bitcoin_util STATIC EXCLUDE_FROM_ALL)
target_sources(bitcoin_util
  PRIVATE
    asmap.cpp
    bip32.cpp
    bytevectorhash.cpp
    check.cpp
    error.cpp
    fees.cpp
    getuniquepath.cpp
    hasher.cpp
    sock.cpp
    syserror.cpp
    system.cpp
    message.cpp
    moneystr.cpp
    rbf.cpp
    readwritefile.cpp
    settings.cpp
    thread.cpp
    threadnames.cpp
    serfloat.cpp
    spanparsing.cpp
    strencodings.cpp
    string.cpp
    syscall_sandbox.cpp
    time.cpp
    tokenpipe.cpp

    url.cpp

    ../chainparamsbase.cpp
    ../clientversion.cpp
    ../fs.cpp
    ../interfaces/echo.cpp
    ../interfaces/handler.cpp
    ../interfaces/init.cpp
    ../logging.cpp
    ../random.cpp
    ../randomenv.cpp
    ../rpc/request.cpp
    ../support/cleanse.cpp
    ../support/lockedpool.cpp
    ../sync.cpp
    ../threadinterrupt.cpp
)
target_include_directories(bitcoin_util
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>/src
)
if(HAVE_GMTIME_R)
  target_compile_definitions(bitcoin_util PRIVATE HAVE_GMTIME_R)
endif()
target_link_libraries(bitcoin_util
  PRIVATE
    univalue
    Boost::headers
    Threads::Threads
)
