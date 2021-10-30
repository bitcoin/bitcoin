# Copyright 2017 The CRC32C Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file. See the AUTHORS file for names of contributors.

cmake_minimum_required(VERSION 3.1)
project(Crc32c VERSION 1.1.0 LANGUAGES C CXX)

# This project can use C11, but will gracefully decay down to C89.
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED OFF)
set(CMAKE_C_EXTENSIONS OFF)

# This project requires C++11.
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# https://github.com/izenecloud/cmake/blob/master/SetCompilerWarningAll.cmake
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  # Use the highest warning level for Visual Studio.
  set(CMAKE_CXX_WARNING_LEVEL 4)
  if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
    string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  else(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
  endif(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")

  # Disable C++ exceptions.
  string(REGEX REPLACE "/EH[a-z]+" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHs-c-")
  add_definitions(-D_HAS_EXCEPTIONS=0)

  # Disable RTTI.
  string(REGEX REPLACE "/GR" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /GR-")
else(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  # Use -Wall for clang and gcc.
  if(NOT CMAKE_CXX_FLAGS MATCHES "-Wall")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
  endif(NOT CMAKE_CXX_FLAGS MATCHES "-Wall")

  # Use -Wextra for clang and gcc.
  if(NOT CMAKE_CXX_FLAGS MATCHES "-Wextra")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wextra")
  endif(NOT CMAKE_CXX_FLAGS MATCHES "-Wextra")

  # Use -Werror for clang and gcc.
  if(NOT CMAKE_CXX_FLAGS MATCHES "-Werror")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror")
  endif(NOT CMAKE_CXX_FLAGS MATCHES "-Werror")

  # Disable C++ exceptions.
  string(REGEX REPLACE "-fexceptions" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-exceptions")

  # Disable RTTI.
  string(REGEX REPLACE "-frtti" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")
endif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")

option(CRC32C_BUILD_TESTS "Build CRC32C's unit tests" ON)
option(CRC32C_BUILD_BENCHMARKS "Build CRC32C's benchmarks" ON)
option(CRC32C_USE_GLOG "Build CRC32C's tests with Google Logging" ON)
option(CRC32C_INSTALL "Install CRC32C's header and library" ON)

include(TestBigEndian)
test_big_endian(BYTE_ORDER_BIG_ENDIAN)

include(CheckCXXCompilerFlag)
# Used by glog.
check_cxx_compiler_flag(-Wno-deprecated CRC32C_HAVE_NO_DEPRECATED)
# Used by glog.
check_cxx_compiler_flag(-Wno-sign-compare CRC32C_HAVE_NO_SIGN_COMPARE)
# Used by glog.
check_cxx_compiler_flag(-Wno-unused-parameter CRC32C_HAVE_NO_UNUSED_PARAMETER)
# Used by googletest.
check_cxx_compiler_flag(-Wno-missing-field-initializers
                        CRC32C_HAVE_NO_MISSING_FIELD_INITIALIZERS)

# Check for __builtin_prefetch support in the compiler.
include(CheckCXXSourceCompiles)
check_cxx_source_compiles("
int main() {
  char data = 0;
  const char* address = &data;
  __builtin_prefetch(address, 0, 0);
  return 0;
}
"  HAVE_BUILTIN_PREFETCH)

# Check for _mm_prefetch support in the compiler.
include(CheckCXXSourceCompiles)
check_cxx_source_compiles("
#if defined(_MSC_VER)
#include <intrin.h>
#else  // !defined(_MSC_VER)
#include <xmmintrin.h>
#endif  // defined(_MSC_VER)

int main() {
  char data = 0;
  const char* address = &data;
  _mm_prefetch(address, _MM_HINT_NTA);
  return 0;
}
"  HAVE_MM_PREFETCH)

# Check for SSE4.2 support in the compiler.
set(OLD_CMAKE_REQURED_FLAGS ${CMAKE_REQUIRED_FLAGS})
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  set(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} /arch:AVX")
else(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  set(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} -msse4.2")
endif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
check_cxx_source_compiles("
#if defined(_MSC_VER)
#include <intrin.h>
#else  // !defined(_MSC_VER)
#include <cpuid.h>
#include <nmmintrin.h>
#endif  // defined(_MSC_VER)

int main() {
  _mm_crc32_u8(0, 0); _mm_crc32_u32(0, 0);
#if defined(_M_X64) || defined(__x86_64__)
   _mm_crc32_u64(0, 0);
#endif // defined(_M_X64) || defined(__x86_64__)
  return 0;
}
"  HAVE_SSE42)
set(CMAKE_REQUIRED_FLAGS ${OLD_CMAKE_REQURED_FLAGS})

# Check for ARMv8 w/ CRC and CRYPTO extensions support in the compiler.
set(OLD_CMAKE_REQURED_FLAGS ${CMAKE_REQUIRED_FLAGS})
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  # TODO(pwnall): Insert correct flag when VS gets ARM CRC32C support.
  set(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} /arch:NOTYET")
else(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  set(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} -march=armv8-a+crc+crypto")
endif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
check_cxx_source_compiles("
#include <arm_acle.h>
#include <arm_neon.h>

int main() {
  __crc32cb(0, 0); __crc32ch(0, 0); __crc32cw(0, 0); __crc32cd(0, 0);
  vmull_p64(0, 0);
  return 0;
}
" HAVE_ARM64_CRC32C)
set(CMAKE_REQUIRED_FLAGS ${OLD_CMAKE_REQURED_FLAGS})

# Check for strong getauxval() support in the system headers.
check_cxx_source_compiles("
#include <arm_acle.h>
#include <arm_neon.h>
#include <sys/auxv.h>

int main() {
  getauxval(AT_HWCAP);
  return 0;
}
" HAVE_STRONG_GETAUXVAL)

# Check for weak getauxval() support in the compiler.
check_cxx_source_compiles("
unsigned long getauxval(unsigned long type) __attribute__((weak));
#define AT_HWCAP 16

int main() {
  getauxval(AT_HWCAP);
  return 0;
}
" HAVE_WEAK_GETAUXVAL)

if(CRC32C_USE_GLOG)
  # glog requires this setting to avoid using dynamic_cast.
  set(DISABLE_RTTI ON CACHE BOOL "" FORCE)

  # glog's test targets trigger deprecation warnings, and compiling them burns
  # CPU cycles on the CI.
  set(BUILD_TESTING_SAVED "${BUILD_TESTING}")
  set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
  add_subdirectory("third_party/glog" EXCLUDE_FROM_ALL)
  set(BUILD_TESTING "${BUILD_TESTING_SAVED}" CACHE BOOL "" FORCE)

  # glog triggers deprecation warnings on OSX.
  # https://github.com/google/glog/issues/185
  if(CRC32C_HAVE_NO_DEPRECATED)
    set_property(TARGET glog APPEND PROPERTY COMPILE_OPTIONS -Wno-deprecated)
  endif(CRC32C_HAVE_NO_DEPRECATED)

  # glog triggers sign comparison warnings on gcc.
  if(CRC32C_HAVE_NO_SIGN_COMPARE)
    set_property(TARGET glog APPEND PROPERTY COMPILE_OPTIONS -Wno-sign-compare)
  endif(CRC32C_HAVE_NO_SIGN_COMPARE)

  # glog triggers unused parameter warnings on clang.
  if(CRC32C_HAVE_NO_UNUSED_PARAMETER)
    set_property(TARGET glog
                 APPEND PROPERTY COMPILE_OPTIONS -Wno-unused-parameter)
  endif(CRC32C_HAVE_NO_UNUSED_PARAMETER)

  set(CRC32C_TESTS_BUILT_WITH_GLOG 1)
endif(CRC32C_USE_GLOG)

configure_file(
  "src/crc32c_config.h.in"
  "${PROJECT_BINARY_DIR}/include/crc32c/crc32c_config.h"
)

include_directories("${PROJECT_BINARY_DIR}/include")

# ARM64 CRC32C code is built separately, so we don't accidentally compile
# unsupported instructions into code that gets run without ARM32 support.
add_library(crc32c_arm64 OBJECT "")
target_sources(crc32c_arm64
  PRIVATE
    "${PROJECT_BINARY_DIR}/include/crc32c/crc32c_config.h"
    "src/crc32c_arm64.cc"
    "src/crc32c_arm64.h"
)
if(HAVE_ARM64_CRC32C)
  if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    # TODO(pwnall): Insert correct flag when VS gets ARM64 CRC32C support.
    target_compile_options(crc32c_arm64 PRIVATE "/arch:NOTYET")
  else(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(crc32c_arm64 PRIVATE "-march=armv8-a+crc+crypto")
  endif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
endif(HAVE_ARM64_CRC32C)

# CMake only enables PIC by default in SHARED and MODULE targets.
if(BUILD_SHARED_LIBS)
  set_property(TARGET crc32c_arm64 PROPERTY POSITION_INDEPENDENT_CODE TRUE)
endif(BUILD_SHARED_LIBS)

# SSE4.2 code is built separately, so we don't accidentally compile unsupported
# instructions into code that gets run without SSE4.2 support.
add_library(crc32c_sse42 OBJECT "")
target_sources(crc32c_sse42
  PRIVATE
    "${PROJECT_BINARY_DIR}/include/crc32c/crc32c_config.h"
    "src/crc32c_sse42.cc"
    "src/crc32c_sse42.h"
)
if(HAVE_SSE42)
  if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(crc32c_sse42 PRIVATE "/arch:AVX")
  else(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(crc32c_sse42 PRIVATE "-msse4.2")
  endif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
endif(HAVE_SSE42)

# CMake only enables PIC by default in SHARED and MODULE targets.
if(BUILD_SHARED_LIBS)
  set_property(TARGET crc32c_sse42 PROPERTY POSITION_INDEPENDENT_CODE TRUE)
endif(BUILD_SHARED_LIBS)

# Must be included before CMAKE_INSTALL_INCLUDEDIR is used.
include(GNUInstallDirs)

add_library(crc32c ""
  # TODO(pwnall): Move the TARGET_OBJECTS generator expressions to the PRIVATE
  # section of target_sources when cmake_minimum_required becomes 3.9 or above.
  $<TARGET_OBJECTS:crc32c_arm64>
  $<TARGET_OBJECTS:crc32c_sse42>
)
target_sources(crc32c
  PRIVATE
    "${PROJECT_BINARY_DIR}/include/crc32c/crc32c_config.h"
    "src/crc32c_arm64.h"
    "src/crc32c_arm64_linux_check.h"
    "src/crc32c_internal.h"
    "src/crc32c_portable.cc"
    "src/crc32c_prefetch.h"
    "src/crc32c_read_le.h"
    "src/crc32c_round_up.h"
    "src/crc32c_sse42.h"
    "src/crc32c_sse42_check.h"
    "src/crc32c.cc"

  # Only CMake 3.3+ supports PUBLIC sources in targets exported by "install".
  $<$<VERSION_GREATER:CMAKE_VERSION,3.2>:PUBLIC>
    "include/crc32c/crc32c.h"
)

target_include_directories(crc32c
  PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_compile_definitions(crc32c
PRIVATE
  CRC32C_HAVE_CONFIG_H=1
)

set_target_properties(crc32c
  PROPERTIES VERSION ${PROJECT_VERSION} SOVERSION ${PROJECT_VERSION_MAJOR})

# Warnings as errors in Visual Studio for this project's targets.
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  set_property(TARGET crc32c APPEND PROPERTY COMPILE_OPTIONS "/WX")
  set_property(TARGET crc32c_arm64 APPEND PROPERTY COMPILE_OPTIONS "/WX")
  set_property(TARGET crc32c_sse42 APPEND PROPERTY COMPILE_OPTIONS "/WX")
endif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")

if(CRC32C_BUILD_TESTS)
  enable_testing()

  # Prevent overriding the parent project's compiler/linker settings on Windows.
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  set(install_gtest OFF)
  set(install_gmock OFF)

  # This project is tested using GoogleTest.
  add_subdirectory("third_party/googletest")

  # GoogleTest triggers a missing field initializers warning.
  if(CRC32C_HAVE_NO_MISSING_FIELD_INITIALIZERS)
    set_property(TARGET gtest
        APPEND PROPERTY COMPILE_OPTIONS -Wno-missing-field-initializers)
    set_property(TARGET gmock
        APPEND PROPERTY COMPILE_OPTIONS -Wno-missing-field-initializers)
  endif(CRC32C_HAVE_NO_MISSING_FIELD_INITIALIZERS)

  add_executable(crc32c_tests "")
  target_sources(crc32c_tests
    PRIVATE
      "${PROJECT_BINARY_DIR}/include/crc32c/crc32c_config.h"
      "src/crc32c_arm64_unittest.cc"
      "src/crc32c_extend_unittests.h"
      "src/crc32c_portable_unittest.cc"
      "src/crc32c_prefetch_unittest.cc"
      "src/crc32c_read_le_unittest.cc"
      "src/crc32c_round_up_unittest.cc"
      "src/crc32c_sse42_unittest.cc"
      "src/crc32c_unittest.cc"
      "src/crc32c_test_main.cc"
  )
  target_link_libraries(crc32c_tests crc32c gtest)

  # Warnings as errors in Visual Studio for this project's targets.
  if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set_property(TARGET crc32c_tests APPEND PROPERTY COMPILE_OPTIONS "/WX")
  endif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")

  if(CRC32C_USE_GLOG)
    target_link_libraries(crc32c_tests glog)
  endif(CRC32C_USE_GLOG)

  add_test(NAME crc32c_tests COMMAND crc32c_tests)

  add_executable(crc32c_capi_tests "")
  target_sources(crc32c_capi_tests
    PRIVATE
      "src/crc32c_capi_unittest.c"
  )
  target_link_libraries(crc32c_capi_tests crc32c)

  # Warnings as errors in Visual Studio for this project's targets.
  if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set_property(TARGET crc32c_capi_tests APPEND PROPERTY COMPILE_OPTIONS "/WX")
  endif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")

  add_test(NAME crc32c_capi_tests COMMAND crc32c_capi_tests)
endif(CRC32C_BUILD_TESTS)

if(CRC32C_BUILD_BENCHMARKS)
  add_executable(crc32c_bench "")
  target_sources(crc32c_bench
    PRIVATE
      "${PROJECT_BINARY_DIR}/include/crc32c/crc32c_config.h"
      "src/crc32c_benchmark.cc"
  )
  target_link_libraries(crc32c_bench crc32c)

  # This project uses Google benchmark for benchmarking.
  set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
  set(BENCHMARK_ENABLE_EXCEPTIONS OFF CACHE BOOL "" FORCE)
  add_subdirectory("third_party/benchmark")
  target_link_libraries(crc32c_bench benchmark)

  if(CRC32C_USE_GLOG)
    target_link_libraries(crc32c_bench glog)
  endif(CRC32C_USE_GLOG)

  # Warnings as errors in Visual Studio for this project's targets.
  if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set_property(TARGET crc32c_bench APPEND PROPERTY COMPILE_OPTIONS "/WX")
  endif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
endif(CRC32C_BUILD_BENCHMARKS)

if(CRC32C_INSTALL)
  install(TARGETS crc32c
    EXPORT Crc32cTargets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  )
  install(
    FILES
      "include/crc32c/crc32c.h"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/crc32c"
  )

  include(CMakePackageConfigHelpers)
  write_basic_package_version_file(
      "${PROJECT_BINARY_DIR}/Crc32cConfigVersion.cmake"
      COMPATIBILITY SameMajorVersion
  )
  install(
    EXPORT Crc32cTargets
    NAMESPACE Crc32c::
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/Crc32c"
  )
  install(
    FILES
      "Crc32cConfig.cmake"
      "${PROJECT_BINARY_DIR}/Crc32cConfigVersion.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/Crc32c"
  )
endif(CRC32C_INSTALL)
