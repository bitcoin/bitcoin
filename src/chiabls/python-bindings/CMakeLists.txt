CMAKE_MINIMUM_REQUIRED(VERSION 3.1.0 FATAL_ERROR)
set (CMAKE_CXX_STANDARD 11)

include_directories(
  ${INCLUDE_DIRECTORIES}
  ${CMAKE_CURRENT_SOURCE_DIR}/../contrib/relic/include
  ${CMAKE_BINARY_DIR}/contrib/relic/include
  ${CMAKE_CURRENT_SOURCE_DIR}/../contrib/catch
  )

pybind11_add_module(blspy ${CMAKE_CURRENT_SOURCE_DIR}/pythonbindings.cpp)

if (SODIUM_FOUND)
  target_link_libraries(blspy PRIVATE blstmp relic_s sodium)
else()
  target_link_libraries(blspy PRIVATE blstmp relic_s)
endif()
