# Copyright (c) 2023-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

if(TARGET bitcoin-util AND TARGET bitcoin-tx AND PYTHON_COMMAND)
  add_test(NAME util_test_runner
    COMMAND ${CMAKE_COMMAND} -E env BITCOINUTIL=$<TARGET_FILE:bitcoin-util> BITCOINTX=$<TARGET_FILE:bitcoin-tx> ${PYTHON_COMMAND} ${CMAKE_BINARY_DIR}/test/util/test_runner.py
  )
endif()

if(PYTHON_COMMAND)
  add_test(NAME util_rpcauth_test
    COMMAND ${PYTHON_COMMAND} ${CMAKE_BINARY_DIR}/test/util/rpcauth-test.py
  )
endif()

if(TARGET bench_bitcoin)
  add_test(NAME bench_sanity_check_high_priority
    COMMAND bench_bitcoin -sanity-check -priority-level=high
  )
endif()

if(TARGET test_bitcoin)
  function(add_boost_test source_file)
    if(NOT EXISTS ${source_file})
      return()
    endif()

    file(READ "${source_file}" source_file_content)
    string(REGEX
      MATCH "(BOOST_FIXTURE_TEST_SUITE|BOOST_AUTO_TEST_SUITE)\\(([A-Za-z0-9_]+)"
      test_suite_macro "${source_file_content}"
    )
    string(REGEX
      REPLACE "(BOOST_FIXTURE_TEST_SUITE|BOOST_AUTO_TEST_SUITE)\\(" ""
      test_suite_name "${test_suite_macro}"
    )
    if(test_suite_name)
      add_test(NAME ${test_suite_name}
        COMMAND test_bitcoin --run_test=${test_suite_name} --catch_system_error=no
      )
    endif()
  endfunction()

  function(add_all_test_targets)
    get_target_property(test_source_dir test_bitcoin SOURCE_DIR)
    get_target_property(test_sources test_bitcoin SOURCES)
    foreach(test_source ${test_sources})
      cmake_path(IS_RELATIVE test_source result)
      if(result)
        cmake_path(APPEND test_source_dir ${test_source} OUTPUT_VARIABLE test_source)
      endif()
      add_boost_test(${test_source})
    endforeach()
  endfunction()

  add_all_test_targets()
endif()

if(TARGET unitester)
  add_test(NAME univalue_test
    COMMAND unitester
  )
endif()

if(TARGET object)
  add_test(NAME univalue_object_test
    COMMAND object
  )
endif()

if(TARGET test_bitcoin-qt)
  add_test(NAME test_bitcoin-qt
    COMMAND test_bitcoin-qt
  )
  if(WIN32 AND VCPKG_TARGET_TRIPLET)
    # On Windows, vcpkg configures Qt with `-opengl dynamic`, which makes
    # the "minimal" platform plugin unusable due to internal Qt bugs.
    set_tests_properties(test_bitcoin-qt PROPERTIES
      ENVIRONMENT "QT_QPA_PLATFORM=windows"
    )
  endif()
endif()
