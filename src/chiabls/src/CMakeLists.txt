CMAKE_MINIMUM_REQUIRED(VERSION 3.14 FATAL_ERROR)
set (CMAKE_CXX_STANDARD 17)

# CMake 3.14+
include(FetchContent)
if (DEFINED ENV{RELIC_MAIN})
  set(RELIC_GIT_TAG "origin/main")
else ()
  set(RELIC_GIT_TAG "3a23142be0a5510a3aa93cd6c76fc59d3fc732a5")
endif ()

message(STATUS "Relic will be built from: ${RELIC_GIT_TAG}")

FetchContent_Declare(
  relic
  GIT_REPOSITORY https://github.com/relic-toolkit/relic.git
  GIT_TAG        ${RELIC_GIT_TAG}
)
FetchContent_MakeAvailable(relic)

file(GLOB HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp)
source_group("SrcHeaders" FILES ${HEADERS})

include_directories(
  ${INCLUDE_DIRECTORIES}
  ${relic_SOURCE_DIR}/include
  ${relic_BINARY_DIR}/include
  )

if (GMP_FOUND)
  include_directories(${GMP_INCLUDES})
endif(GMP_FOUND)

set(C_LIB ${CMAKE_BINARY_DIR}/libbls-dash.a)

add_library(bls-dash ${CMAKE_CURRENT_SOURCE_DIR}/privatekey.cpp)

add_library(blstmp ${HEADERS}
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

set(OPREFIX object_)
find_library(GMP_NAME NAMES libgmp.a gmp)
find_library(SODIUM_NAME NAMES libsodium.a sodium)

set(LIBRARIES_TO_COMBINE
      COMMAND mkdir ${OPREFIX}$<TARGET_NAME:blstmp> || true && cd ${OPREFIX}$<TARGET_NAME:blstmp> &&  ${CMAKE_AR} -x $<TARGET_FILE:blstmp>
      COMMAND mkdir ${OPREFIX}$<TARGET_NAME:relic_s> || true && cd ${OPREFIX}$<TARGET_NAME:relic_s> &&  ${CMAKE_AR} -x $<TARGET_FILE:relic_s>
)

if (GMP_FOUND)
  list(APPEND LIBRARIES_TO_COMBINE COMMAND mkdir ${OPREFIX}gmp || true && cd ${OPREFIX}gmp &&  ${CMAKE_AR} -x ${GMP_NAME})
endif()
if (SODIUM_FOUND)
  message("SODIUM_FOUND in src/CMakeLists.txt")
  list(APPEND LIBRARIES_TO_COMBINE COMMAND mkdir ${OPREFIX}sodium || true && cd ${OPREFIX}sodium &&  ${CMAKE_AR} -x ${SODIUM_NAME})
  target_compile_definitions(blstmp PRIVATE BLSALLOC_SODIUM=1)
else()
  target_compile_definitions(blstmp PRIVATE)
endif()

add_custom_target(combined_custom
        ${LIBRARIES_TO_COMBINE}
        COMMAND ${CMAKE_AR} -rs ${C_LIB} ${OPREFIX}*/*${CMAKE_C_OUTPUT_EXTENSION}
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        DEPENDS blstmp relic_s
        )

add_library(combined STATIC IMPORTED GLOBAL)
add_dependencies(combined combined_custom)
target_link_libraries(bls-dash combined)

set_target_properties(combined
        PROPERTIES
        IMPORTED_LOCATION ${C_LIB}
        )

file(GLOB includes "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp")
install(DIRECTORY ${relic_SOURCE_DIR}/include/ DESTINATION include/bls-dash)
install(DIRECTORY ${relic_BINARY_DIR}/include/ DESTINATION include/bls-dash)
install(FILES ${includes} DESTINATION include/bls-dash)
install(FILES ${C_LIB} DESTINATION lib)

if (BUILD_BLS_TESTS)
  include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../contrib/catch)
  add_executable(runtest test.cpp)
  if (SODIUM_FOUND)
    target_link_libraries(runtest blstmp relic_s sodium)
  else()
    target_link_libraries(runtest blstmp relic_s)
  endif()
endif()

if (BUILD_BLS_BENCHMARKS)
  add_executable(runbench test-bench.cpp)
  if (SODIUM_FOUND)
    target_link_libraries(runbench blstmp relic_s sodium)
  else()
    target_link_libraries(runbench blstmp relic_s)
  endif()
endif()
