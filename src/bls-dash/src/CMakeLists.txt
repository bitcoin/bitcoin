file(GLOB HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp)
source_group("SrcHeaders" FILES ${HEADERS})

add_library(bls-dash
  ${HEADERS}
  ${CMAKE_CURRENT_SOURCE_DIR}/privatekey.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/bls.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/chaincode.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/elements.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/extendedprivatekey.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/extendedpublickey.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/legacy.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/schemes.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/threshold.cpp
)

target_include_directories(bls-dash
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    $<$<BOOL:${GMP_FOUND}>:${GMP_INCLUDES}>
    ${relic_SOURCE_DIR}/include
    ${relic_BINARY_DIR}/include
)

target_compile_definitions(bls-dash
  PRIVATE
    BLSALLOC_SODIUM=1
)

target_link_libraries(bls-dash
  PUBLIC
    relic_s
    sodium
)

install(DIRECTORY ${relic_SOURCE_DIR}/include/ DESTINATION include/bls-dash)
install(DIRECTORY ${relic_BINARY_DIR}/include/ DESTINATION include/bls-dash)
install(FILES ${HEADERS} DESTINATION include/bls-dash)
install(FILES $<TARGET_FILE:bls-dash> DESTINATION lib)

if(BUILD_BLS_TESTS)
  add_executable(runtest test.cpp)
  INCLUDE(FindPkgConfig)
  pkg_check_modules(CATCH2 catch2)
  if (CATCH2_FOUND)
    # Adding "catch2" subdir to include dirs because "catch.hpp" is included
    # instead of "catch2/catch.hpp"
    if (NOT CATCH2_INCLUDE_DIRS)
      set(CATCH2_INCLUDE_DIRS ${CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES})
    endif()
    list(TRANSFORM CATCH2_INCLUDE_DIRS APPEND /catch2)
    target_include_directories(runtest PRIVATE ${CATCH2_INCLUDE_DIRS})
  else()
    target_include_directories(runtest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../contrib/catch)
  endif()
  target_link_libraries(runtest PRIVATE bls-dash)
endif()

if(BUILD_BLS_BENCHMARKS)
  add_executable(runbench test-bench.cpp)
  target_link_libraries(runbench PRIVATE bls-dash)
endif()
