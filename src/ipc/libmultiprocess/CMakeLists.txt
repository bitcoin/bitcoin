# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

cmake_minimum_required(VERSION 3.12)

project("Libmultiprocess" CXX)
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  set(CMAKE_CXX_STANDARD 20)
  set(CMAKE_CXX_STANDARD_REQUIRED YES)
endif()

include("cmake/compat_find.cmake")

find_package(Threads REQUIRED)
find_package(CapnProto 0.7 QUIET NO_MODULE)
 if(NOT CapnProto_FOUND)
   message(FATAL_ERROR
     "Cap'n Proto is required but was not found.\n"
     "To resolve, choose one of the following:\n"
     "  - Install Cap'n Proto (version 1.0+ recommended)\n"
     "  - For Bitcoin Core compilation build with -DENABLE_IPC=OFF to disable multiprocess support\n"
   )
 endif()

# Cap'n Proto compatibility checks
set(CAPNPROTO_ISSUES "")
set(CAPNPROTO_CVE_AFFECTED FALSE)
set(CAPNPROTO_CLANG_INCOMPATIBLE FALSE)

# Check for list-of-pointers memory access bug from Nov 2022
# https://nvd.nist.gov/vuln/detail/CVE-2022-46149
# https://github.com/advisories/GHSA-qqff-4vw4-f6hx
# https://github.com/capnproto/capnproto/security/advisories/GHSA-qqff-4vw4-f6hx
# https://github.com/capnproto/capnproto/blob/master/security-advisories/2022-11-30-0-pointer-list-bounds.md
# https://capnproto.org/news/2022-11-30-CVE-2022-46149-security-advisory.html
# https://dwrensha.github.io/capnproto-rust/2022/11/30/out_of_bounds_memory_access_bug.html
if(CapnProto_VERSION STREQUAL "0.7.0"
   OR CapnProto_VERSION STREQUAL "0.8.0"
   OR CapnProto_VERSION STREQUAL "0.9.0"
   OR CapnProto_VERSION STREQUAL "0.9.1"
   OR CapnProto_VERSION STREQUAL "0.10.0"
   OR CapnProto_VERSION STREQUAL "0.10.1"
   OR CapnProto_VERSION STREQUAL "0.10.2")
  set(CAPNPROTO_CVE_AFFECTED TRUE)
  string(APPEND CAPNPROTO_ISSUES "- CVE-2022-46149 security vulnerability (details: https://github.com/advisories/GHSA-qqff-4vw4-f6hx)\n")
endif()

# Check for Cap'n Proto / Clang / C++20 incompatibility
# Cap'n Proto 0.9.x and 0.10.x are incompatible with Clang 16+ when using C++20
# due to P2468R2 implementation. This was fixed in Cap'n Proto 1.0+.
# See: https://github.com/bitcoin-core/libmultiprocess/issues/199
if((CapnProto_VERSION VERSION_GREATER_EQUAL "0.9.0") AND
   (CapnProto_VERSION VERSION_LESS "1.0.0") AND
   (CMAKE_CXX_COMPILER_ID STREQUAL "Clang") AND
   (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL "16") AND
   (CMAKE_CXX_STANDARD EQUAL 20))
  set(CAPNPROTO_CLANG_INCOMPATIBLE TRUE)
  string(APPEND CAPNPROTO_ISSUES "- Incompatible with Clang ${CMAKE_CXX_COMPILER_VERSION} when using C++20\n")
endif()

if(CAPNPROTO_CVE_AFFECTED OR CAPNPROTO_CLANG_INCOMPATIBLE)
  set(RESOLUTION_OPTIONS "")

  # Fixes both issues
  string(APPEND RESOLUTION_OPTIONS "  - Upgrade to Cap'n Proto version 1.0 or newer (recommended)\n")

  if(CAPNPROTO_CVE_AFFECTED AND NOT CAPNPROTO_CLANG_INCOMPATIBLE)
    string(APPEND RESOLUTION_OPTIONS "  - Upgrade to a patched minor version (0.7.1, 0.8.1, 0.9.2, 0.10.3, or later)\n")
  elseif(CAPNPROTO_CLANG_INCOMPATIBLE AND NOT CAPNPROTO_CVE_AFFECTED)
    string(APPEND RESOLUTION_OPTIONS "  - Use GCC instead of Clang\n")
  endif()

  string(APPEND RESOLUTION_OPTIONS "  - For Bitcoin Core compilation build with -DENABLE_IPC=OFF to disable multiprocess support\n")

  message(FATAL_ERROR
    "The version of Cap'n Proto detected: ${CapnProto_VERSION} has known compatibility issues:\n"
    "${CAPNPROTO_ISSUES}"
    "To resolve, choose one of the following:\n"
    "${RESOLUTION_OPTIONS}"
  )
endif()

set(MPGEN_EXECUTABLE "" CACHE FILEPATH "If specified, should be full path to an external mpgen binary to use rather than the one built internally.")

option(MP_ENABLE_CLANG_TIDY "Run clang-tidy with the compiler." OFF)
if(MP_ENABLE_CLANG_TIDY)
  find_program(CLANG_TIDY_EXECUTABLE NAMES clang-tidy)
  if(NOT CLANG_TIDY_EXECUTABLE)
    message(FATAL_ERROR "MP_ENABLE_CLANG_TIDY is ON but clang-tidy is not found.")
  endif()
  set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_EXECUTABLE}")

  # Workaround for nix from https://gitlab.kitware.com/cmake/cmake/-/issues/20912#note_793338
  # Nix injects header paths via $NIX_CFLAGS_COMPILE; CMake tags these as
  # CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES and omits them from the compile
  # database, so clang-tidy, which ignores $NIX_CFLAGS_COMPILE, can't find capnp
  # headers. Setting them as standard passes them to clang-tidy.
  set(CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES ${CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES})
endif()

option(MP_ENABLE_IWYU "Run include-what-you-use with the compiler." OFF)
if(MP_ENABLE_IWYU)
  find_program(IWYU_EXECUTABLE NAMES include-what-you-use iwyu)
  if(NOT IWYU_EXECUTABLE)
    message(FATAL_ERROR "MP_ENABLE_IWYU is ON but include-what-you-use was not found.")
  endif()
  set(CMAKE_CXX_INCLUDE_WHAT_YOU_USE "${IWYU_EXECUTABLE};-Xiwyu;--error")
  if(DEFINED ENV{IWYU_MAPPING_FILE})
    list(APPEND CMAKE_CXX_INCLUDE_WHAT_YOU_USE "-Xiwyu" "--mapping_file=$ENV{IWYU_MAPPING_FILE}")
  endif()
endif()

include("cmake/compat_config.cmake")
include("cmake/pthread_checks.cmake")
include(GNUInstallDirs)

# Set MP_INCLUDE_DIR as a global property so target_capnp_sources function can
# use it, and its callers don't need to specify the include directory manually
# to avoid "error: Import failed: /mp/proxy.capnp" failures from capnproto.
set_property(GLOBAL PROPERTY MP_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")

# Set a convenience variable for subdirectories.
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  set(MP_STANDALONE TRUE)
  include(CTest)
else()
  set(MP_STANDALONE FALSE)
endif()

# Prevent include directories from parent project from leaking into this one.
set_property(DIRECTORY PROPERTY INCLUDE_DIRECTORIES "")

# Generated C++ preprocessor defines
configure_file(include/mp/config.h.in "${CMAKE_CURRENT_BINARY_DIR}/include/mp/config.h")

# Generated C++ Capn'Proto schema files
capnp_generate_cpp(MP_PROXY_SRCS MP_PROXY_HDRS include/mp/proxy.capnp)
set_source_files_properties("${MP_PROXY_SRCS}" PROPERTIES SKIP_LINTING TRUE) # Ignored before cmake 3.27

# util library
add_library(mputil OBJECT src/mp/util.cpp)
target_include_directories(mputil PRIVATE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>)
target_link_libraries(mputil PUBLIC CapnProto::kj)

# libmultiprocess.a runtime library
set(MP_PUBLIC_HEADERS
  ${MP_PROXY_HDRS}
  include/mp/proxy-io.h
  include/mp/proxy-types.h
  include/mp/proxy.h
  include/mp/type-char.h
  include/mp/type-chrono.h
  include/mp/type-context.h
  include/mp/type-data.h
  include/mp/type-decay.h
  include/mp/type-exception.h
  include/mp/type-function.h
  include/mp/type-interface.h
  include/mp/type-map.h
  include/mp/type-message.h
  include/mp/type-number.h
  include/mp/type-optional.h
  include/mp/type-pair.h
  include/mp/type-pointer.h
  include/mp/type-set.h
  include/mp/type-string.h
  include/mp/type-struct.h
  include/mp/type-threadmap.h
  include/mp/type-tuple.h
  include/mp/type-vector.h
  include/mp/type-void.h
  include/mp/util.h)
add_library(multiprocess STATIC
  ${MP_PROXY_SRCS}
  ${MP_PUBLIC_HEADERS}
  src/mp/proxy.cpp
  $<TARGET_OBJECTS:mputil>)
add_library(Libmultiprocess::multiprocess ALIAS multiprocess)
target_include_directories(multiprocess PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_link_libraries(multiprocess PUBLIC CapnProto::capnp)
target_link_libraries(multiprocess PUBLIC CapnProto::capnp-rpc)
target_link_libraries(multiprocess PUBLIC CapnProto::kj)
target_link_libraries(multiprocess PUBLIC CapnProto::kj-async)
set_target_properties(multiprocess PROPERTIES
    PUBLIC_HEADER "${MP_PUBLIC_HEADERS}")
install(TARGETS multiprocess EXPORT LibTargets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT lib
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/mp COMPONENT lib)

# mpgen code generator
add_executable(mpgen src/mp/gen.cpp $<TARGET_OBJECTS:mputil>)
add_executable(Libmultiprocess::mpgen ALIAS mpgen)
target_include_directories(mpgen PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>)
target_include_directories(mpgen PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_link_libraries(mpgen PRIVATE CapnProto::capnp)
target_link_libraries(mpgen PRIVATE CapnProto::capnp-rpc)
target_link_libraries(mpgen PRIVATE CapnProto::capnpc)
target_link_libraries(mpgen PRIVATE CapnProto::kj)
target_link_libraries(mpgen PRIVATE Threads::Threads)
set_target_properties(mpgen PROPERTIES
    INSTALL_RPATH_USE_LINK_PATH TRUE)
set_target_properties(mpgen PROPERTIES
    PUBLIC_HEADER include/mp/proxy.capnp)
install(TARGETS mpgen EXPORT BinTargets
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT bin
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/mp COMPONENT bin)

# makefile include to invoke mpgen code generator, for downstream Make projects
install(FILES "include/mpgen.mk"
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR} COMPONENT bin)

# pkg-config module to build against libmultiprocess library, for downstream autoconf projects
configure_file(pkgconfig/libmultiprocess.pc.in "${CMAKE_CURRENT_BINARY_DIR}/pkgconfig/libmultiprocess.pc" @ONLY)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/pkgconfig/libmultiprocess.pc"
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig COMPONENT lib)

# cmake include to invoke mpgen code generator, for downstream CMake projects
install(
  FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/TargetCapnpSources.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Libmultiprocess COMPONENT bin)

# CMake target import files, for downstream CMake projects
install(EXPORT BinTargets
  NAMESPACE Libmultiprocess::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Libmultiprocess COMPONENT bin)
install(EXPORT LibTargets
  NAMESPACE Libmultiprocess::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Libmultiprocess COMPONENT lib)

# CMake find_package config file, for downstream CMake projects
include(CMakePackageConfigHelpers)
configure_package_config_file(
  ${PROJECT_SOURCE_DIR}/cmake/Config.cmake.in
  LibmultiprocessConfig.cmake
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Libmultiprocess
  NO_SET_AND_CHECK_MACRO)
install(
  FILES
    ${CMAKE_CURRENT_BINARY_DIR}/LibmultiprocessConfig.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Libmultiprocess
  COMPONENT common)

# Makefile targets to support "make install-bin" "make install-lib"
add_custom_target(install-bin
  COMMAND ${CMAKE_COMMAND} -DCOMPONENT=bin -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_install.cmake
  COMMAND ${CMAKE_COMMAND} -DCOMPONENT=common -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_install.cmake
  VERBATIM)
add_dependencies(install-bin mpgen)
add_custom_target(install-lib
  COMMAND ${CMAKE_COMMAND} -DCOMPONENT=lib -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_install.cmake
  COMMAND ${CMAKE_COMMAND} -DCOMPONENT=common -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_install.cmake
  VERBATIM)
add_dependencies(install-lib multiprocess)

# Example and test subdirectories
add_subdirectory(example EXCLUDE_FROM_ALL)
add_subdirectory(test EXCLUDE_FROM_ALL)
