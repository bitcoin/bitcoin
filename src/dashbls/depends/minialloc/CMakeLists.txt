CMAKE_MINIMUM_REQUIRED(VERSION 3.14.0 FATAL_ERROR)

if(POLICY CMP0077)
    cmake_policy(SET CMP0077 NEW)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(PROJECT_NAME "minialloc")

project(
    ${PROJECT_NAME}
    VERSION 23
    LANGUAGES CXX
)

set(
    ${PROJECT_NAME}_CONTRIB
    "bitcoin/support/cleanse.cpp"
    "bitcoin/support/cleanse.h"
    "bitcoin/support/lockedpool.cpp"
    "bitcoin/support/lockedpool.h"
)

set(
    ${PROJECT_NAME}_SOURCES
    "minialloc.cpp"
)

list(
    TRANSFORM ${PROJECT_NAME}_CONTRIB
    PREPEND "${CMAKE_CURRENT_LIST_DIR}/contrib/"
)

list(
    TRANSFORM ${PROJECT_NAME}_SOURCES
    PREPEND "${CMAKE_CURRENT_LIST_DIR}/src/"
)

set(
    ${PROJECT_NAME}_STATIC "1" CACHE STRING "Build ${PROJECT_NAME} as a static library"
)

add_library(
    ${PROJECT_NAME}
    "${${PROJECT_NAME}_CONTRIB}"
    "${${PROJECT_NAME}_SOURCES}"
)

target_include_directories(
    ${PROJECT_NAME}
    PRIVATE
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/src/>"
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include/minialloc/>"
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/contrib/bitcoin/>"
    PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include/>"
)

target_compile_definitions(
    ${PROJECT_NAME}
    PRIVATE
        LIBRARY_STATIC="${${PROJECT_NAME}_STATIC}"
    PUBLIC
        LIBRARY_STATIC="${${PROJECT_NAME}_STATIC}"
)

