# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

set(FUNCTIONAL_TEST_CONFIG_FILE "${PROJECT_BINARY_DIR}/test/config.ini")
set(FUNCTIONAL_TEST_CACHE_DIR "${PROJECT_BINARY_DIR}/test/functional/cache")
set(FUNCTIONAL_TEST_TMP_DIR "${PROJECT_BINARY_DIR}/test/functional/tmp")
set(FUNCTIONAL_TEST_RUNNER "${CMAKE_CURRENT_BINARY_DIR}/test_runner.py")

if(CMAKE_VERSION VERSION_GREATER_EQUAL 4.4)
  set(FUNCTIONAL_DISCOVERY_ARGS
    "--dump-ctest"
  )
  if(BUILD_EXTENDED_FUNCTIONAL_TESTS)
    list(APPEND FUNCTIONAL_DISCOVERY_ARGS "--extended")
  endif()

  # Empty replacements are retained as empty arguments by CMake. The test
  # runner ignores that argument when a test has no optional argument.
  discover_tests(
    COMMAND "${Python3_EXECUTABLE}" "${FUNCTIONAL_TEST_RUNNER}"
    DISCOVERY_ARGS ${FUNCTIONAL_DISCOVERY_ARGS}
    DISCOVERY_MATCH "^([^\t]+)\t([^\t]+)\t([0-9]+)\t([^\t]*)\t([^\t]*)\t([0-9]+)$"
    TEST_NAME "\\1"
    TEST_ARGS
      "--ctest-direct"
      "\\2"
      "--"
      "\\5"
      "--cachedir=${FUNCTIONAL_TEST_CACHE_DIR}"
      "--portseed=\\3"
      "--tmpdir=${FUNCTIONAL_TEST_TMP_DIR}/\\1"
    TEST_PROPERTIES
      ENVIRONMENT "PYTHON_GIL=1"
      FIXTURES_REQUIRED functional_cache
      LABELS "functional;\\4"
      COST "\\6"
      SKIP_RETURN_CODE 77
      WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
  )
else()
  if(BUILD_EXTENDED_FUNCTIONAL_TESTS)
    set(FUNCTIONAL_DISCOVERY_EXTENDED_ARG "--extended")
  endif()

  configure_file(functional_discovery.cmake.in functional_discovery.cmake @ONLY)
  set_property(DIRECTORY APPEND PROPERTY TEST_INCLUDE_FILES
    "${CMAKE_CURRENT_BINARY_DIR}/functional_discovery.cmake"
  )
endif()

add_test(NAME functional.cache_remove_before
  COMMAND ${CMAKE_COMMAND} -E rm -rf
    "${FUNCTIONAL_TEST_CACHE_DIR}"
    "${FUNCTIONAL_TEST_TMP_DIR}"
  WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
)
set_tests_properties(functional.cache_remove_before PROPERTIES
  FIXTURES_SETUP functional_cache
  LABELS functional
)

add_test(NAME functional.create_cache
  COMMAND Python3::Interpreter
    "${CMAKE_CURRENT_SOURCE_DIR}/create_cache.py"
    "--configfile=${FUNCTIONAL_TEST_CONFIG_FILE}"
    "--cachedir=${FUNCTIONAL_TEST_CACHE_DIR}"
    "--portseed=1"
  WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
)
set_tests_properties(functional.create_cache PROPERTIES
  DEPENDS functional.cache_remove_before
  ENVIRONMENT "PYTHON_GIL=1"
  FIXTURES_SETUP functional_cache
  LABELS functional
  SKIP_RETURN_CODE 77
)

add_test(NAME functional.cache_remove_after
  COMMAND ${CMAKE_COMMAND} -E rm -rf
    "${FUNCTIONAL_TEST_CACHE_DIR}"
  WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
)
set_tests_properties(functional.cache_remove_after PROPERTIES
  FIXTURES_CLEANUP functional_cache
  LABELS functional
)
