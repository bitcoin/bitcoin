CMAKE_MINIMUM_REQUIRED(VERSION 3.14 FATAL_ERROR)
set (CMAKE_CXX_STANDARD 17)

# CMake 3.14+
include(FetchContent)

FetchContent_Declare(
  pybind11
  GIT_REPOSITORY https://github.com/pybind/pybind11.git
  GIT_TAG        v2.6.2
)
FetchContent_MakeAvailable(pybind11 relic)


include_directories(
  ${INCLUDE_DIRECTORIES}
  ${relic_SOURCE_DIR}/include
  ${relic_BINARY_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/../contrib/catch
  )

if (GMP_FOUND)
  include_directories(${GMP_INCLUDES})
endif(GMP_FOUND)

pybind11_add_module(blspy ${CMAKE_CURRENT_SOURCE_DIR}/pythonbindings.cpp)

if (SODIUM_FOUND)
  set(BLSALLOC_SODIUM "1" CACHE STRING "")
  set(SODIUM_STATIC "1" CACHE STRING "")
  target_link_libraries(blspy PRIVATE blstmp relic_s sodium)
else()
  target_link_libraries(blspy PRIVATE blstmp relic_s)
endif()
