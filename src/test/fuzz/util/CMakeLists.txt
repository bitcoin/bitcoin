# Copyright (c) 2023 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

add_library(test_fuzz STATIC EXCLUDE_FROM_ALL
  mempool.cpp
  net.cpp
  ../fuzz.cpp
  ../util.cpp
)
target_link_libraries(test_fuzz
  PRIVATE
    test_util
    bitcoin_node
    leveldb
    Boost::headers
)

set(fuzz_binary_code "
  #include <cstdint>
  #include <cstddef>
  extern \"C\" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) { return 0; }
  // No main() function.
")
try_append_linker_flag(fuzz_binary_linker_flags "${sanitizers_linker_flags}"
  SOURCE "${fuzz_binary_code}"
)
if(NOT try_append_linker_flag_result)
  target_compile_definitions(test_fuzz PRIVATE PROVIDE_FUZZ_MAIN_FUNCTION)
endif()
