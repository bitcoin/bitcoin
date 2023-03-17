CMAKE_MINIMUM_REQUIRED(VERSION 3.14.0 FATAL_ERROR)

set(PROJECT_NAME "catch2")

project(
    ${PROJECT_NAME}
    VERSION 2.13
    LANGUAGES CXX
)

set(
    ${PROJECT_NAME}_HEADERS
    catch2/catch.hpp
)

list(
    TRANSFORM ${PROJECT_NAME}_HEADERS
    PREPEND "${CMAKE_CURRENT_LIST_DIR}/include/"
)

add_library(
    ${PROJECT_NAME}
    INTERFACE
)

target_sources(
    ${PROJECT_NAME}
    INTERFACE
    "${${PROJECT_NAME}_HEADERS}"
)

target_include_directories(
    ${PROJECT_NAME}
    INTERFACE
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)
