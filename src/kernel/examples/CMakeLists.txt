# Copyright (c) 2024-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

# Build kernel library examples

# Basic context exampleCMakeLists.txt
add_executable(kernel_example_basic basic_context.cpp)
target_include_directories(kernel_example_basic PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(kernel_example_basic PRIVATE bitcoinkernel)
set_target_properties(kernel_example_basic PROPERTIES
  CXX_STANDARD 20
  CXX_STANDARD_REQUIRED ON
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/examples"
)

# Script verification example
add_executable(kernel_example_verify verify_script.cpp)
target_include_directories(kernel_example_verify PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(kernel_example_verify PRIVATE bitcoinkernel)
set_target_properties(kernel_example_verify PROPERTIES
  CXX_STANDARD 20
  CXX_STANDARD_REQUIRED ON
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/examples"
)

# Install examples to lib/examples
install(TARGETS kernel_example_basic kernel_example_verify
  RUNTIME DESTINATION "${CMAKE_INSTALL_LIBDIR}/examples"
)