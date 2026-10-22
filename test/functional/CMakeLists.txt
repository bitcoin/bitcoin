# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

set(functional_test_runner "${CMAKE_CURRENT_BINARY_DIR}/test_runner.py")
set(functional_cache_dir "${CMAKE_CURRENT_BINARY_DIR}/cache")

discover_tests(
  COMMAND Python3::Interpreter "${functional_test_runner}"
  DISCOVERY_ARGS --dump-ctest
  DISCOVERY_MATCH "^([^\t]+)\t([0-9]+)\t([^\t]*)\t([0-9]+)\t([^\t]+)$"
  TEST_NAME "\\1"
  TEST_ARGS
    "--ctest-direct=\\5"
    "--cachedir=${functional_cache_dir}"
    "--portseed=\\2"
  TEST_PROPERTIES
    ENVIRONMENT "PYTHON_GIL=1"
    FIXTURES_REQUIRED functional_cache
    LABELS "functional;\\3"
    COST "\\4"
    SKIP_RETURN_CODE 77
)

add_test(NAME functional.cache_remove_before
  COMMAND ${CMAKE_COMMAND} -E rm -rf "${functional_cache_dir}"
)
set_tests_properties(functional.cache_remove_before PROPERTIES
  FIXTURES_SETUP functional_cache
  LABELS functional
)

add_test(NAME functional.create_cache
  COMMAND Python3::Interpreter "${functional_test_runner}"
    "--ctest-direct=create_cache.py"
    "--cachedir=${functional_cache_dir}"
    "--portseed=1"
)
set_tests_properties(functional.create_cache PROPERTIES
  DEPENDS functional.cache_remove_before
  ENVIRONMENT "PYTHON_GIL=1"
  FIXTURES_SETUP functional_cache
  LABELS functional
  SKIP_RETURN_CODE 77
)

add_test(NAME functional.cache_remove_after
  COMMAND ${CMAKE_COMMAND} -E rm -rf "${functional_cache_dir}"
)
set_tests_properties(functional.cache_remove_after PROPERTIES
  FIXTURES_CLEANUP functional_cache
  LABELS functional
)
