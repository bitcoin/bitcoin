# Copyright (c) 2025-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

# https://gitlab.kitware.com/cmake/cmake/-/issues/26920
function(discover_tests target)
  set(oneValueArgs DISCOVERY_MATCH TEST_NAME_REPLACEMENT TEST_ARGS_REPLACEMENT)
  set(multiValueArgs DISCOVERY_ARGS PROPERTIES)
  cmake_parse_arguments(PARSE_ARGV 1 arg "" "${oneValueArgs}" "${multiValueArgs}")

  get_property(has_counter TARGET ${target} PROPERTY CTEST_DISCOVERED_TEST_COUNTER SET)
  if(has_counter)
    get_property(counter TARGET ${target} PROPERTY CTEST_DISCOVERED_TEST_COUNTER)
    math(EXPR counter "${counter} + 1")
  else()
    set(counter 1)
  endif()
  set_property(TARGET ${target} PROPERTY CTEST_DISCOVERED_TEST_COUNTER ${counter})

  set(file_base "${CMAKE_CURRENT_BINARY_DIR}/${target}[${counter}]")
  set(include_file "${file_base}_include.cmake")

  string(CONCAT include_content
    "set(runner   [[$<TARGET_FILE:${target}>]])\n"
    "set(launcher [[$<TARGET_PROPERTY:${target},TEST_LAUNCHER>]])\n"
    "set(emulator [[$<$<BOOL:${CMAKE_CROSSCOMPILING}>:$<TARGET_PROPERTY:${target},CROSSCOMPILING_EMULATOR>>]])\n"
    "\n"
    "execute_process(\n"
    "  COMMAND \${launcher} \${emulator} \${runner} ${arg_DISCOVERY_ARGS}\n"
    "  OUTPUT_VARIABLE output OUTPUT_STRIP_TRAILING_WHITESPACE\n"
    "  ERROR_VARIABLE  output ERROR_STRIP_TRAILING_WHITESPACE\n"
    "  RESULT_VARIABLE result\n"
    ")\n"
    "\n"
    "if(NOT result EQUAL 0)\n"
    "  add_test([[${target}_DISCOVERY_FAILURE]] \${launcher} \${emulator} \${runner} ${arg_DISCOVERY_ARGS})\n"
    "else()\n"
    "  string(REPLACE \"\\n\" \";\" lines \"\${output}\")\n"
    "  foreach(line IN LISTS lines)\n"
    "    if(line MATCHES [[${arg_DISCOVERY_MATCH}]])\n"
    "      string(REGEX REPLACE [[${arg_DISCOVERY_MATCH}]] [[${arg_TEST_NAME_REPLACEMENT}]] test_name \"\${line}\")\n"
    "      string(REGEX REPLACE [[${arg_DISCOVERY_MATCH}]] [[${arg_TEST_ARGS_REPLACEMENT}]] test_args \"\${line}\")\n"
    "      separate_arguments(test_args)\n"
    "      add_test(\"\${test_name}\" \${launcher} \${emulator} \${runner} \${test_args})\n"
    "      set_tests_properties(\"\${test_name}\" PROPERTIES ${arg_PROPERTIES})\n"
    "    endif()\n"
    "  endforeach()\n"
    "endif()\n"
  )

  get_property(is_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
  if(is_multi_config)
    file(GENERATE
      OUTPUT "${file_base}_include-$<CONFIG>.cmake"
      CONTENT "${include_content}"
    )
    file(WRITE "${include_file}"
      "include(\"${file_base}_include-\${CTEST_CONFIGURATION_TYPE}.cmake\")"
    )
  else()
    file(GENERATE
      OUTPUT "${include_file}"
      CONTENT "${include_content}"
    )
  endif()

  # Add discovered tests to directory TEST_INCLUDE_FILES
  set_property(DIRECTORY APPEND PROPERTY TEST_INCLUDE_FILES "${include_file}")
endfunction()
