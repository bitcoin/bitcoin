# Copyright (c) 2023-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

function(create_test_config)
  set(abs_top_srcdir ${PROJECT_SOURCE_DIR})
  set(abs_top_builddir ${PROJECT_BINARY_DIR})
  set(EXEEXT ${CMAKE_EXECUTABLE_SUFFIX})

  macro(set_configure_variable var conf_var)
    if(${var})
      set(${conf_var}_TRUE "")
    else()
      set(${conf_var}_TRUE "#")
    endif()
  endmacro()

  set_configure_variable(ENABLE_WALLET ENABLE_WALLET)
  set_configure_variable(BUILD_CLI BUILD_BITCOIN_CLI)
  set_configure_variable(BUILD_UTIL BUILD_BITCOIN_UTIL)
  set_configure_variable(BUILD_UTIL_CHAINSTATE BUILD_BITCOIN_CHAINSTATE)
  set_configure_variable(BUILD_WALLET_TOOL BUILD_BITCOIN_WALLET)
  set_configure_variable(BUILD_DAEMON BUILD_BITCOIND)
  set_configure_variable(BUILD_FUZZ_BINARY ENABLE_FUZZ_BINARY)
  set_configure_variable(WITH_ZMQ ENABLE_ZMQ)
  set_configure_variable(ENABLE_EXTERNAL_SIGNER ENABLE_EXTERNAL_SIGNER)
  set_configure_variable(WITH_USDT ENABLE_USDT_TRACEPOINTS)

  configure_file(config.ini.in config.ini USE_SOURCE_PERMISSIONS @ONLY)
endfunction()

create_test_config()

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/functional)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/fuzz)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/util)

file(GLOB_RECURSE functional_tests RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} functional/*)
foreach(script ${functional_tests} fuzz/test_runner.py util/rpcauth-test.py util/test_runner.py)
  if(CMAKE_HOST_WIN32)
    set(symlink)
  else()
    set(symlink SYMBOLIC)
  endif()
  file(CREATE_LINK ${CMAKE_CURRENT_SOURCE_DIR}/${script} ${CMAKE_CURRENT_BINARY_DIR}/${script} COPY_ON_ERROR ${symlink})
endforeach()
unset(functional_tests)
