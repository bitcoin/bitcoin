# Copyright (c) 2023-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

add_library(univalue STATIC EXCLUDE_FROM_ALL
  lib/univalue.cpp
  lib/univalue_get.cpp
  lib/univalue_read.cpp
  lib/univalue_write.cpp
)
target_include_directories(univalue
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_link_libraries(univalue PRIVATE core_interface)

if(BUILD_TESTS)
  add_executable(unitester test/unitester.cpp)
  target_compile_definitions(unitester
    PRIVATE
      JSON_TEST_SRC=\"${CMAKE_CURRENT_SOURCE_DIR}/test\"
  )
  target_link_libraries(unitester
    PRIVATE
      core_interface
      univalue
  )

  add_executable(object test/object.cpp)
  target_link_libraries(object
    PRIVATE
      core_interface
      univalue
  )
endif()
