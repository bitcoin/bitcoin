# Copyright (c) 2023-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

if(BUILD_INDIVIDUAL_FUZZ_BINARIES)
  # bash command produces list of harnesses: <harness name> <source file>
  execute_process(
      COMMAND bash -c "grep -H \"^FUZZ_TARGET\" src/test/fuzz/*.cpp | sed -E 's/.*\\/([^/]+\\.cpp):FUZZ_TARGET(_DESERIALIZE)?\\(([a-zA-Z0-9_]+).*/\\3 \\1/'"
      OUTPUT_VARIABLE FUZZ_HARNESSES
      OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  string(REPLACE "\n" ";" FUZZ_HARNESS_LIST "${FUZZ_HARNESSES}")
  message(STATUS ${FUZZ_HARNESSES})

  foreach(HARNESS_INFO ${FUZZ_HARNESS_LIST})
    string(REPLACE " " ";" HARNESS_PARTS ${HARNESS_INFO})
    list(GET HARNESS_PARTS 0 HARNESS_NAME)
    list(GET HARNESS_PARTS 1 HARNESS_FILE)

    add_library(test_fuzz_${HARNESS_NAME} STATIC EXCLUDE_FROM_ALL
      descriptor.cpp
      mempool.cpp
      net.cpp
      ../fuzz.cpp
      ../util.cpp
    )

    target_link_libraries(test_fuzz_${HARNESS_NAME}
      PRIVATE
        core_interface
        test_util
        bitcoin_node
        Boost::headers
    )

    target_compile_definitions(test_fuzz_${HARNESS_NAME} PUBLIC FUZZ_HARNESS=${HARNESS_NAME})

    if(NOT FUZZ_BINARY_LINKS_WITHOUT_MAIN_FUNCTION)
      target_compile_definitions(test_fuzz_${HARNESS_NAME} PRIVATE PROVIDE_FUZZ_MAIN_FUNCTION)
    endif()
  endforeach()
else()
  add_library(test_fuzz STATIC EXCLUDE_FROM_ALL
    descriptor.cpp
    mempool.cpp
    net.cpp
    ../fuzz.cpp
    ../util.cpp
  )

  target_link_libraries(test_fuzz
    PRIVATE
      core_interface
      test_util
      bitcoin_node
      Boost::headers
  )

  if(NOT FUZZ_BINARY_LINKS_WITHOUT_MAIN_FUNCTION)
    target_compile_definitions(test_fuzz PRIVATE PROVIDE_FUZZ_MAIN_FUNCTION)
  endif()
endif()
