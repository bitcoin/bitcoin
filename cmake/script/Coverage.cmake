# Copyright (c) 2024-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

include(${CMAKE_CURRENT_LIST_DIR}/CoverageInclude.cmake)

set(functional_test_runner test/functional/test_runner.py)
if(EXTENDED_FUNCTIONAL_TESTS)
  list(APPEND functional_test_runner --extended)
endif()
if(DEFINED JOBS)
  list(APPEND CMAKE_CTEST_COMMAND -j ${JOBS})
  list(APPEND functional_test_runner -j ${JOBS})
endif()

execute_process(
  COMMAND ${CMAKE_CTEST_COMMAND} --build-config Coverage
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
  COMMAND_ERROR_IS_FATAL ANY
)
execute_process(
  COMMAND ${LCOV_COMMAND} --capture --directory src --test-name test_bitcoin --output-file test_bitcoin.info
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
  COMMAND_ERROR_IS_FATAL ANY
)
execute_process(
  COMMAND ${LCOV_COMMAND} --zerocounters --directory src
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
  COMMAND_ERROR_IS_FATAL ANY
)
execute_process(
  COMMAND ${LCOV_FILTER_COMMAND} test_bitcoin.info test_bitcoin_filtered.info
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
  COMMAND_ERROR_IS_FATAL ANY
)
execute_process(
  COMMAND ${LCOV_COMMAND} --add-tracefile test_bitcoin_filtered.info --output-file test_bitcoin_filtered.info
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
  COMMAND_ERROR_IS_FATAL ANY
)
execute_process(
  COMMAND ${LCOV_COMMAND} --add-tracefile baseline_filtered.info --add-tracefile test_bitcoin_filtered.info --output-file test_bitcoin_coverage.info
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
  COMMAND_ERROR_IS_FATAL ANY
)
execute_process(
  COMMAND ${GENHTML_COMMAND} test_bitcoin_coverage.info --output-directory test_bitcoin.coverage
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
  COMMAND_ERROR_IS_FATAL ANY
)

execute_process(
  COMMAND ${functional_test_runner}
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
  COMMAND_ERROR_IS_FATAL ANY
)
execute_process(
  COMMAND ${LCOV_COMMAND} --capture --directory src --test-name functional-tests --output-file functional_test.info
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
  COMMAND_ERROR_IS_FATAL ANY
)
execute_process(
  COMMAND ${LCOV_COMMAND} --zerocounters --directory src
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
  COMMAND_ERROR_IS_FATAL ANY
)
execute_process(
  COMMAND ${LCOV_FILTER_COMMAND} functional_test.info functional_test_filtered.info
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
  COMMAND_ERROR_IS_FATAL ANY
)
execute_process(
  COMMAND ${LCOV_COMMAND} --add-tracefile functional_test_filtered.info --output-file functional_test_filtered.info
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
  COMMAND_ERROR_IS_FATAL ANY
)
execute_process(
  COMMAND ${LCOV_COMMAND} --add-tracefile baseline_filtered.info --add-tracefile test_bitcoin_filtered.info --add-tracefile functional_test_filtered.info --output-file total_coverage.info
  COMMAND ${GREP_EXECUTABLE} "%"
  COMMAND ${AWK_EXECUTABLE} "{ print substr($3,2,50) \"/\" $5 }"
  OUTPUT_FILE coverage_percent.txt
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
  COMMAND_ERROR_IS_FATAL ANY
)
execute_process(
  COMMAND ${GENHTML_COMMAND} total_coverage.info --output-directory total.coverage
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
  COMMAND_ERROR_IS_FATAL ANY
)
