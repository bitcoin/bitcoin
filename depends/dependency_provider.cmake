# Copyright (c) 2025-present The Bitcoin Core developers
# Distributed under the MIT software license.

cmake_minimum_required(VERSION 3.24)

# Use CMAKE_FIND_ROOT_PATH (set by toolchain.cmake) for depends directory
set(DEPENDS_DIR "${CMAKE_FIND_ROOT_PATH}")

macro(bitcoin_depends_provide_dependency method package_name)
  if("${method}" STREQUAL "FIND_PACKAGE")
    set(${package_name}_ROOT "${DEPENDS_DIR}" CACHE PATH "" FORCE)
    set(${package_name}_INCLUDE_DIR "${DEPENDS_DIR}/include" CACHE PATH "" FORCE)
    set(${package_name}_INCLUDEDIR "${DEPENDS_DIR}/include" CACHE PATH "" FORCE)
    set(${package_name}_LIBRARY_DIR "${DEPENDS_DIR}/lib" CACHE PATH "" FORCE)
    find_package(${package_name} ${ARGN} BYPASS_PROVIDER)
  endif()
endmacro()

# Make sure the depends lib directory is in the library search path
link_directories(BEFORE "${DEPENDS_DIR}/lib")

set(BITCOIN_DEPENDENCY_PROVIDER_ACTIVE ON CACHE INTERNAL "Dependency provider active")
set(BITCOIN_DEPENDENCY_PROVIDER_PATH "${DEPENDS_DIR}/dependency_provider.cmake" CACHE INTERNAL "Dependency provider path")

cmake_language(
  SET_DEPENDENCY_PROVIDER bitcoin_depends_provide_dependency
  SUPPORTED_METHODS FIND_PACKAGE
)
