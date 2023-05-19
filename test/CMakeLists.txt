# Copyright (c) 2023 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

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
  set_configure_variable(WITH_SQLITE USE_SQLITE)
  set_configure_variable(WITH_BDB USE_BDB)
  set_configure_variable(BUILD_CLI BUILD_BITCOIN_CLI)
  set_configure_variable(BUILD_UTIL BUILD_BITCOIN_UTIL)
  set_configure_variable(BUILD_WALLET_TOOL BUILD_BITCOIN_WALLET)
  set_configure_variable(BUILD_DAEMON BUILD_BITCOIND_TRUE)
  set_configure_variable(WITH_ZMQ ENABLE_ZMQ)
  set_configure_variable(ENABLE_EXTERNAL_SIGNER ENABLE_EXTERNAL_SIGNER)
  set_configure_variable(USE_SYSCALL_SANDBOX ENABLE_SYSCALL_SANDBOX)
  set_configure_variable(ENABLE_TRACING ENABLE_USDT_TRACEPOINTS)

  configure_file(config.ini.in config.ini @ONLY)
endfunction()

create_test_config()

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/functional)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/fuzz)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/util)

function(create_test_script script)
  if(MSVC)
    file(CREATE_LINK ${CMAKE_CURRENT_SOURCE_DIR}/${script} ${CMAKE_CURRENT_BINARY_DIR}/${script} COPY_ON_ERROR)
  elseif(CMAKE_VERSION VERSION_GREATER_EQUAL 3.14)
    file(CREATE_LINK ${CMAKE_CURRENT_SOURCE_DIR}/${script} ${CMAKE_CURRENT_BINARY_DIR}/${script} COPY_ON_ERROR SYMBOLIC)
  else()
    file(REMOVE ${CMAKE_CURRENT_BINARY_DIR}/${script})
    execute_process(COMMAND ln -s ${CMAKE_CURRENT_SOURCE_DIR}/${script} ${CMAKE_CURRENT_BINARY_DIR}/${script})
  endif()
endfunction()

foreach(script functional/test_runner.py fuzz/test_runner.py util/rpcauth-test.py util/test_runner.py)
  create_test_script(${script})
endforeach()
