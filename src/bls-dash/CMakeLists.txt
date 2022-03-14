CMAKE_MINIMUM_REQUIRED(VERSION 3.14.0 FATAL_ERROR)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 99)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release"
    CACHE STRING "Possible values are empty, Debug, Release, RelWithDebInfo, MinSizeRel, ..."
    FORCE
  )
endif()

project(BLS)

set(BUILD_BLS_PYTHON_BINDINGS "0" CACHE STRING "")
set(BUILD_BLS_TESTS "0" CACHE STRING "")
set(BUILD_BLS_BENCHMARKS "0" CACHE STRING "")

message(STATUS "Build python bindings: ${BUILD_BLS_PYTHON_BINDINGS}")
message(STATUS "Build tests: ${BUILD_BLS_TESTS}")
message(STATUS "Build benchmarks: ${BUILD_BLS_BENCHMARKS}")

# Add path for custom modules
set(CMAKE_MODULE_PATH
  ${CMAKE_MODULE_PATH}
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules
)

include(FetchContent)

FetchContent_Declare(Sodium
  GIT_REPOSITORY https://github.com/AmineKhaldi/libsodium-cmake.git
  # Latest commit at the moment this was added here
  # Anchored to libsodium v1.0.18
  GIT_TAG f73a3fe1afdc4e37ac5fe0ddd401bf521f6bba65
)
set(SODIUM_PCH "on" CACHE STRING "")
set(SODIUM_DISABLE_TESTS "on" CACHE STRING "")
set(SODIUM_CHIA_MINIMAL "on" CACHE STRING "")
FetchContent_MakeAvailable(Sodium)

if (DEFINED ENV{RELIC_MAIN})
  set(RELIC_GIT_TAG "origin/main")
else ()
  # This is currently anchored to upstream aecdcae7956f542fbee2392c1f0feb0a8ac41dc5
  set(RELIC_GIT_TAG "1d98e5abf3ca5b14fd729bd5bcced88ea70ecfd7")
endif ()

message(STATUS "Relic will be built from: ${RELIC_GIT_TAG}")

FetchContent_Declare(
  relic
  GIT_REPOSITORY https://github.com/Chia-Network/relic.git
  GIT_TAG ${RELIC_GIT_TAG}
)

# Relic related options

set(STBIN TRUE)
find_package(gmp)
if (GMP_FOUND)
  message(STATUS "Found libgmp")
  set(ARITH "gmp" CACHE STRING "")
else()
  set(ARITH "easy" CACHE STRING "")
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 4)
  set(WSIZE "32" CACHE STRING "Relic - Processor word size")
else()
  set(WSIZE "64" CACHE STRING "Relic - Processor word size")
endif()

if(EMSCRIPTEN)
  # emscripten needs arch set to be none since it can't compile assembly
  set(ARCH "" CACHE STRING "")
  # emscripten is a 32 bit compiler
  set(WSIZE "32" CACHE STRING "Relic - Processor word size")
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
  set(TIMER "ANSI" CACHE STRING "")
  set(MULTI "" CACHE STRING "")
else()
  set(TIMER "CYCLE" CACHE STRING "")
  set(MULTI "PTHREAD" CACHE STRING "")
endif()

set(CHECK "off" CACHE STRING "")
set(VERBS "off" CACHE STRING "")
set(ALLOC "AUTO" CACHE STRING "")
set(SHLIB "off" CACHE STRING "")
set(DOCUM "off" CACHE STRING "")
set(FP_PRIME "381" CACHE STRING "Relic - Prime modulus size")

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set(SEED "UDEV" CACHE STRING "")
  set(FP_QNRES "off" CACHE STRING "")
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
  set(SEED "WCGR" CACHE STRING "")
  set(FP_QNRES "on" CACHE STRING "")
else()
  set(SEED "UDEV" CACHE STRING "")
  set(FP_QNRES "on" CACHE STRING "")
endif()

set(STBIN "OFF" CACHE STRING "")

set(FP_METHD "INTEG;INTEG;INTEG;MONTY;LOWER;SLIDE" CACHE STRING "")

if(MSVC)
  set(COMP_FLAGS "" CACHE STRING "")
else()
  set(COMP_FLAGS "-O3 -funroll-loops -fomit-frame-pointer" CACHE STRING "")
endif()

set(FP_PMERS "off" CACHE STRING "")
set(FPX_METHD "INTEG;INTEG;LAZYR" CACHE STRING "")
set(EP_PLAIN "off" CACHE STRING "")
set(EP_SUPER "off" CACHE STRING "")
# Disable relic tests and benchmarks
set(TESTS "0" CACHE STRING "Relic - Number of times each test is ran")
set(BENCH "0" CACHE STRING "Relic - Number of times each benchmark is ran")

set(QUIET "on" CACHE STRING "Relic - Build with printing disabled")

set(PP_EXT "LAZYR" CACHE STRING "")
set(PP_METHD "LAZYR;OATEP" CACHE STRING "")

FetchContent_MakeAvailable(relic)

add_subdirectory(src)

if(EMSCRIPTEN)
  add_subdirectory(js-bindings)
else()
  # emscripten can't build python bindings, it produces only javascript
  # add_subdirectory(contrib/pybind11)
  if(BUILD_BLS_PYTHON_BINDINGS)
    add_subdirectory(python-bindings)
  endif()
endif()
