CMAKE_MINIMUM_REQUIRED(VERSION 3.1.0 FATAL_ERROR)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

IF(NOT CMAKE_BUILD_TYPE)
  SET(CMAKE_BUILD_TYPE "RELEASE")
ENDIF()

project(BLS)

# Add path for custom modules
set(CMAKE_MODULE_PATH
	${CMAKE_MODULE_PATH}
	${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules
)

find_package(sodium)
if (SODIUM_FOUND)
  message(STATUS "Found libsodium")
  message(STATUS "Sodium include dir = ${sodium_INCLUDE_DIR}")
  set(BLSALLOC_SODIUM "1" CACHE STRING "")
  include_directories(${sodium_INCLUDE_DIR})
endif()

find_package(gmp)
if (GMP_FOUND)
  message(STATUS "Found libgmp")
  include_directories(${GMP_INCLUDE_DIR})
  set(ARITH "gmp" CACHE STRING "")
else()
  set(ARITH "easy" CACHE STRING "")
endif()


set(WSIZE 64 CACHE INTEGER "")
set(TIMER "CYCLE" CACHE STRING "")
set(CHECK "off" CACHE STRING "")
set(VERBS "off" CACHE STRING "")
set(ALLOC "AUTO" CACHE STRING "")
set(SHLIB "OFF" CACHE STRING "")
set(MULTI "PTHREAD" CACHE STRING "")

set(FP_PRIME 381 CACHE INTEGER "")

IF (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set(DSEED "UDEV" CACHE STRING "")
  set(FP_QNRES "off" CACHE STRING "")
ELSEIF (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
  set(SEED "WCGR" CACHE STRING "")
  set(FP_QNRES "on" CACHE STRING "")
ELSE()
  set(DSEED "DEV" CACHE STRING "")
  set(FP_QNRES "on" CACHE STRING "")
ENDIF()
set(STBIN "OFF" CACHE STRING "")

set(FP_METHD "INTEG;INTEG;INTEG;MONTY;LOWER;SLIDE" CACHE STRING "")
set(COMP "-O3 -funroll-loops -fomit-frame-pointer" CACHE STRING "")
set(FP_PMERS "off" CACHE STRING "")
set(FPX_METHD "INTEG;INTEG;LAZYR" CACHE STRING "")
set(EP_PLAIN "off" CACHE STRING "")
set(EP_SUPER "off" CACHE STRING "")
# Disable relic tests and benchmarks
set(TESTS 0 CACHE INTEGER "")
set(BENCH 0 CACHE INTEGER "")

set(PP_EXT "LAZYR" CACHE STRING "")
set(PP_METHD "LAZYR;OATEP" CACHE STRING "")

add_subdirectory(contrib/relic)
#add_subdirectory(contrib/pybind11)
add_subdirectory(src)
#add_subdirectory(python-bindings)
