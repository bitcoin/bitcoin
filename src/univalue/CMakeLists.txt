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
  add_executable(unitester
    test/unitester.cpp
  )
  include(TargetDataSources)
  target_json_data_sources(unitester
    test/fail1.json
    test/fail10.json
    test/fail11.json
    test/fail12.json
    test/fail13.json
    test/fail14.json
    test/fail15.json
    test/fail16.json
    test/fail17.json
    test/fail18.json
    test/fail19.json
    test/fail2.json
    test/fail20.json
    test/fail21.json
    test/fail22.json
    test/fail23.json
    test/fail24.json
    test/fail25.json
    test/fail26.json
    test/fail27.json
    test/fail28.json
    test/fail29.json
    test/fail3.json
    test/fail30.json
    test/fail31.json
    test/fail32.json
    test/fail33.json
    test/fail34.json
    test/fail35.json
    test/fail36.json
    test/fail37.json
    test/fail38.json
    test/fail39.json
    test/fail4.json
    test/fail40.json
    test/fail41.json
    test/fail42.json
    test/fail44.json
    test/fail45.json
    test/fail5.json
    test/fail6.json
    test/fail7.json
    test/fail8.json
    test/fail9.json
    test/pass1.json
    test/pass2.json
    test/pass3.json
    test/pass4.json
    test/round1.json
    test/round2.json
    test/round3.json
    test/round4.json
    test/round5.json
    test/round6.json
    test/round7.json
  )
  target_link_libraries(unitester
    PRIVATE
      core_interface
      univalue
  )
  add_test(NAME univalue_test
    COMMAND unitester
  )

  add_executable(object test/object.cpp)
  target_link_libraries(object
    PRIVATE
      core_interface
      univalue
  )
  add_test(NAME univalue_object_test
    COMMAND object
  )
endif()
