# Copyright (c) 2023-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

add_library(test_util STATIC EXCLUDE_FROM_ALL
  blockfilter.cpp
  coins.cpp
  index.cpp
  json.cpp
  logging.cpp
  mining.cpp
  net.cpp
  random.cpp
  script.cpp
  setup_common.cpp
  str.cpp
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

function(make_libbitcoinconsensus_dll_available target)
  if(TARGET bitcoinconsensus AND WIN32 AND BUILD_SHARED_LIBS)
    # The DLL must reside either in the same folder where the executable is
    # or somewhere in PATH. Using the former option.
    add_custom_command(
      TARGET ${target} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:bitcoinconsensus> $<TARGET_FILE_DIR:${target}>
      VERBATIM
    )
  endif()
endfunction()
