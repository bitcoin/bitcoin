# Copyright (c) 2023 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

include(CTest)

if(TARGET bitcoin-util AND TARGET bitcoin-tx)
  add_test(NAME util_test_runner
    COMMAND ${CMAKE_COMMAND} -E env BITCOINUTIL=$<TARGET_FILE:bitcoin-util> BITCOINTX=$<TARGET_FILE:bitcoin-tx> ${PYTHON_COMMAND} ${CMAKE_BINARY_DIR}/test/util/test_runner.py
  )
endif()

add_test(NAME util_rpcauth_test
  COMMAND ${PYTHON_COMMAND} ${CMAKE_BINARY_DIR}/test/util/rpcauth-test.py
)

if(TARGET bench_bitcoin)
  add_test(NAME bench_sanity_check_high_priority
    COMMAND bench_bitcoin -sanity-check -priority-level=high
  )
endif()

if(TARGET test_bitcoin)
  function(add_boost_test source_dir source_file)
    set(source_file_path ${source_dir}/${source_file})
    if(NOT EXISTS ${source_file_path})
      return()
    endif()

    file(READ "${source_file_path}" source_file_content)
    string(REGEX
      MATCH "(BOOST_FIXTURE_TEST_SUITE|BOOST_AUTO_TEST_SUITE)\\(([A-Za-z0-9_]+)"
      test_suite_macro "${source_file_content}"
    )
    string(REGEX
      REPLACE "(BOOST_FIXTURE_TEST_SUITE|BOOST_AUTO_TEST_SUITE)\\(" ""
      test_suite_name "${test_suite_macro}"
    )
    if(test_suite_name)
      add_test(NAME ${test_suite_name}:${source_file}
        COMMAND test_bitcoin --run_test=${test_suite_name} --catch_system_error=no
      )
    endif()
  endfunction()

  function(add_all_test_targets)
    get_target_property(test_source_dir test_bitcoin SOURCE_DIR)
    get_target_property(test_sources test_bitcoin SOURCES)
    foreach(test_source ${test_sources})
      add_boost_test(${test_source_dir} ${test_source})
    endforeach()
  endfunction()

  add_all_test_targets()
endif()
