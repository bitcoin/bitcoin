# Copyright (c) 2025-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

# TODO: rework/remove once test discovery is implemented upstream:
# https://gitlab.kitware.com/cmake/cmake/-/issues/26920
function(discover_tests target)
  set(oneValueArgs DISCOVERY_MATCH TEST_NAME_REPLACEMENT TEST_ARGS_REPLACEMENT)
  set(multiValueArgs DISCOVERY_ARGS PROPERTIES)
  cmake_parse_arguments(PARSE_ARGV 1 arg "" "${oneValueArgs}" "${multiValueArgs}")

  set(file_base "${CMAKE_CURRENT_BINARY_DIR}/${target}")
  set(include_file "${file_base}_include.cmake")

  set(properies_content)
  list(LENGTH arg_PROPERTIES properties_len)
  if(properties_len GREATER "0")
    set(properies_content "      set_tests_properties(\"\${test_name}\" PROPERTIES\n")
    math(EXPR num_properties "${properties_len} / 2")
    foreach(i RANGE 0 ${num_properties} 2)
      math(EXPR value_index "${i} + 1")
      list(GET arg_PROPERTIES ${i} name)
      list(GET arg_PROPERTIES ${value_index} value)
      string(APPEND properies_content "        \"${name}\" \"${value}\"\n")
    endforeach()
    string(APPEND properies_content "      )\n")
  endif()

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
    ${properies_content}
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

  set_property(DIRECTORY APPEND PROPERTY TEST_INCLUDE_FILES "${include_file}")
endfunction()
