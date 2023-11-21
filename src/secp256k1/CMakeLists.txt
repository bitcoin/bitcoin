cmake_minimum_required(VERSION 3.13)

if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.15)
  # MSVC runtime library flags are selected by the CMAKE_MSVC_RUNTIME_LIBRARY abstraction.
  cmake_policy(SET CMP0091 NEW)
  # MSVC warning flags are not in CMAKE_<LANG>_FLAGS by default.
  cmake_policy(SET CMP0092 NEW)
endif()

project(libsecp256k1
  # The package (a.k.a. release) version is based on semantic versioning 2.0.0 of
  # the API. All changes in experimental modules are treated as
  # backwards-compatible and therefore at most increase the minor version.
  VERSION 0.3.3
  DESCRIPTION "Optimized C library for ECDSA signatures and secret/public key operations on curve secp256k1."
  HOMEPAGE_URL "https://github.com/bitcoin-core/secp256k1"
  LANGUAGES C
)

if(CMAKE_VERSION VERSION_LESS 3.21)
  get_directory_property(parent_directory PARENT_DIRECTORY)
  if(parent_directory)
    set(PROJECT_IS_TOP_LEVEL OFF CACHE INTERNAL "Emulates CMake 3.21+ behavior.")
    set(${PROJECT_NAME}_IS_TOP_LEVEL OFF CACHE INTERNAL "Emulates CMake 3.21+ behavior.")
  else()
    set(PROJECT_IS_TOP_LEVEL ON CACHE INTERNAL "Emulates CMake 3.21+ behavior.")
    set(${PROJECT_NAME}_IS_TOP_LEVEL ON CACHE INTERNAL "Emulates CMake 3.21+ behavior.")
  endif()
  unset(parent_directory)
endif()

# The library version is based on libtool versioning of the ABI. The set of
# rules for updating the version can be found here:
# https://www.gnu.org/software/libtool/manual/html_node/Updating-version-info.html
# All changes in experimental modules are treated as if they don't affect the
# interface and therefore only increase the revision.
set(${PROJECT_NAME}_LIB_VERSION_CURRENT 2)
set(${PROJECT_NAME}_LIB_VERSION_REVISION 3)
set(${PROJECT_NAME}_LIB_VERSION_AGE 0)

set(CMAKE_C_STANDARD 90)
set(CMAKE_C_EXTENSIONS OFF)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

option(BUILD_SHARED_LIBS "Build shared libraries." ON)
option(SECP256K1_DISABLE_SHARED "Disable shared library. Overrides BUILD_SHARED_LIBS." OFF)
if(SECP256K1_DISABLE_SHARED)
  set(BUILD_SHARED_LIBS OFF)
endif()

option(SECP256K1_INSTALL "Enable installation." ${PROJECT_IS_TOP_LEVEL})

option(SECP256K1_ENABLE_MODULE_ECDH "Enable ECDH module." ON)
if(SECP256K1_ENABLE_MODULE_ECDH)
  add_compile_definitions(ENABLE_MODULE_ECDH=1)
endif()

option(SECP256K1_ENABLE_MODULE_RECOVERY "Enable ECDSA pubkey recovery module." OFF)
if(SECP256K1_ENABLE_MODULE_RECOVERY)
  add_compile_definitions(ENABLE_MODULE_RECOVERY=1)
endif()

option(SECP256K1_ENABLE_MODULE_EXTRAKEYS "Enable extrakeys module." ON)
option(SECP256K1_ENABLE_MODULE_SCHNORRSIG "Enable schnorrsig module." ON)
if(SECP256K1_ENABLE_MODULE_SCHNORRSIG)
  set(SECP256K1_ENABLE_MODULE_EXTRAKEYS ON)
  add_compile_definitions(ENABLE_MODULE_SCHNORRSIG=1)
endif()
if(SECP256K1_ENABLE_MODULE_EXTRAKEYS)
  add_compile_definitions(ENABLE_MODULE_EXTRAKEYS=1)
endif()

option(SECP256K1_ENABLE_MODULE_ELLSWIFT "Enable ElligatorSwift module." ON)
if(SECP256K1_ENABLE_MODULE_ELLSWIFT)
  add_compile_definitions(ENABLE_MODULE_ELLSWIFT=1)
endif()

option(SECP256K1_USE_EXTERNAL_DEFAULT_CALLBACKS "Enable external default callback functions." OFF)
if(SECP256K1_USE_EXTERNAL_DEFAULT_CALLBACKS)
  add_compile_definitions(USE_EXTERNAL_DEFAULT_CALLBACKS=1)
endif()

set(SECP256K1_ECMULT_WINDOW_SIZE "AUTO" CACHE STRING "Window size for ecmult precomputation for verification, specified as integer in range [2..24]. \"AUTO\" is a reasonable setting for desktop machines (currently 15). [default=AUTO]")
set_property(CACHE SECP256K1_ECMULT_WINDOW_SIZE PROPERTY STRINGS "AUTO" 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24)
include(CheckStringOptionValue)
check_string_option_value(SECP256K1_ECMULT_WINDOW_SIZE)
if(SECP256K1_ECMULT_WINDOW_SIZE STREQUAL "AUTO")
  set(SECP256K1_ECMULT_WINDOW_SIZE 15)
endif()
add_compile_definitions(ECMULT_WINDOW_SIZE=${SECP256K1_ECMULT_WINDOW_SIZE})

set(SECP256K1_ECMULT_GEN_PREC_BITS "AUTO" CACHE STRING "Precision bits to tune the precomputed table size for signing, specified as integer 2, 4 or 8. \"AUTO\" is a reasonable setting for desktop machines (currently 4). [default=AUTO]")
set_property(CACHE SECP256K1_ECMULT_GEN_PREC_BITS PROPERTY STRINGS "AUTO" 2 4 8)
check_string_option_value(SECP256K1_ECMULT_GEN_PREC_BITS)
if(SECP256K1_ECMULT_GEN_PREC_BITS STREQUAL "AUTO")
  set(SECP256K1_ECMULT_GEN_PREC_BITS 4)
endif()
add_compile_definitions(ECMULT_GEN_PREC_BITS=${SECP256K1_ECMULT_GEN_PREC_BITS})

set(SECP256K1_TEST_OVERRIDE_WIDE_MULTIPLY "OFF" CACHE STRING "Test-only override of the (autodetected by the C code) \"widemul\" setting. Legal values are: \"OFF\", \"int128_struct\", \"int128\" or \"int64\". [default=OFF]")
set_property(CACHE SECP256K1_TEST_OVERRIDE_WIDE_MULTIPLY PROPERTY STRINGS "OFF" "int128_struct" "int128" "int64")
check_string_option_value(SECP256K1_TEST_OVERRIDE_WIDE_MULTIPLY)
if(SECP256K1_TEST_OVERRIDE_WIDE_MULTIPLY)
  string(TOUPPER "${SECP256K1_TEST_OVERRIDE_WIDE_MULTIPLY}" widemul_upper_value)
  add_compile_definitions(USE_FORCE_WIDEMUL_${widemul_upper_value}=1)
endif()
mark_as_advanced(FORCE SECP256K1_TEST_OVERRIDE_WIDE_MULTIPLY)

set(SECP256K1_ASM "AUTO" CACHE STRING "Assembly optimizations to use: \"AUTO\", \"OFF\", \"x86_64\" or \"arm32\" (experimental). [default=AUTO]")
set_property(CACHE SECP256K1_ASM PROPERTY STRINGS "AUTO" "OFF" "x86_64" "arm32")
check_string_option_value(SECP256K1_ASM)
if(SECP256K1_ASM STREQUAL "arm32")
  enable_language(ASM)
  include(CheckArm32Assembly)
  check_arm32_assembly()
  if(HAVE_ARM32_ASM)
    add_compile_definitions(USE_EXTERNAL_ASM=1)
  else()
    message(FATAL_ERROR "ARM32 assembly optimization requested but not available.")
  endif()
elseif(SECP256K1_ASM)
  include(CheckX86_64Assembly)
  check_x86_64_assembly()
  if(HAVE_X86_64_ASM)
    set(SECP256K1_ASM "x86_64")
    add_compile_definitions(USE_ASM_X86_64=1)
  elseif(SECP256K1_ASM STREQUAL "AUTO")
    set(SECP256K1_ASM "OFF")
  else()
    message(FATAL_ERROR "x86_64 assembly optimization requested but not available.")
  endif()
endif()

option(SECP256K1_EXPERIMENTAL "Allow experimental configuration options." OFF)
if(NOT SECP256K1_EXPERIMENTAL)
  if(SECP256K1_ASM STREQUAL "arm32")
    message(FATAL_ERROR "ARM32 assembly optimization is experimental. Use -DSECP256K1_EXPERIMENTAL=ON to allow.")
  endif()
endif()

set(SECP256K1_VALGRIND "AUTO" CACHE STRING "Build with extra checks for running inside Valgrind. [default=AUTO]")
set_property(CACHE SECP256K1_VALGRIND PROPERTY STRINGS "AUTO" "OFF" "ON")
check_string_option_value(SECP256K1_VALGRIND)
if(SECP256K1_VALGRIND)
  find_package(Valgrind MODULE)
  if(Valgrind_FOUND)
    set(SECP256K1_VALGRIND ON)
    include_directories(${Valgrind_INCLUDE_DIR})
    add_compile_definitions(VALGRIND)
  elseif(SECP256K1_VALGRIND STREQUAL "AUTO")
    set(SECP256K1_VALGRIND OFF)
  else()
    message(FATAL_ERROR "Valgrind support requested but valgrind/memcheck.h header not available.")
  endif()
endif()

option(SECP256K1_BUILD_BENCHMARK "Build benchmarks." ON)
option(SECP256K1_BUILD_TESTS "Build tests." ON)
option(SECP256K1_BUILD_EXHAUSTIVE_TESTS "Build exhaustive tests." ON)
option(SECP256K1_BUILD_CTIME_TESTS "Build constant-time tests." ${SECP256K1_VALGRIND})
option(SECP256K1_BUILD_EXAMPLES "Build examples." OFF)

# Redefine configuration flags.
# We leave assertions on, because they are only used in the examples, and we want them always on there.
if(MSVC)
  string(REGEX REPLACE "/DNDEBUG[ \t\r\n]*" "" CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO}")
  string(REGEX REPLACE "/DNDEBUG[ \t\r\n]*" "" CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")
  string(REGEX REPLACE "/DNDEBUG[ \t\r\n]*" "" CMAKE_C_FLAGS_MINSIZEREL "${CMAKE_C_FLAGS_MINSIZEREL}")
else()
  string(REGEX REPLACE "-DNDEBUG[ \t\r\n]*" "" CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO}")
  string(REGEX REPLACE "-DNDEBUG[ \t\r\n]*" "" CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")
  string(REGEX REPLACE "-DNDEBUG[ \t\r\n]*" "" CMAKE_C_FLAGS_MINSIZEREL "${CMAKE_C_FLAGS_MINSIZEREL}")
  # Prefer -O2 optimization level. (-O3 is CMake's default for Release for many compilers.)
  string(REGEX REPLACE "-O3[ \t\r\n]*" "-O2" CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")
endif()

# Define custom "Coverage" build type.
set(CMAKE_C_FLAGS_COVERAGE "${CMAKE_C_FLAGS_RELWITHDEBINFO} -O0 -DCOVERAGE=1 --coverage" CACHE STRING
  "Flags used by the C compiler during \"Coverage\" builds."
  FORCE
)
set(CMAKE_EXE_LINKER_FLAGS_COVERAGE "${CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO} --coverage" CACHE STRING
  "Flags used for linking binaries during \"Coverage\" builds."
  FORCE
)
set(CMAKE_SHARED_LINKER_FLAGS_COVERAGE "${CMAKE_SHARED_LINKER_FLAGS_RELWITHDEBINFO} --coverage" CACHE STRING
  "Flags used by the shared libraries linker during \"Coverage\" builds."
  FORCE
)
mark_as_advanced(
  CMAKE_C_FLAGS_COVERAGE
  CMAKE_EXE_LINKER_FLAGS_COVERAGE
  CMAKE_SHARED_LINKER_FLAGS_COVERAGE
)

get_property(is_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
set(default_build_type "RelWithDebInfo")
if(is_multi_config)
  set(CMAKE_CONFIGURATION_TYPES "${default_build_type}" "Release" "Debug" "MinSizeRel" "Coverage" CACHE STRING
    "Supported configuration types."
    FORCE
  )
else()
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY
    STRINGS "${default_build_type}" "Release" "Debug" "MinSizeRel" "Coverage"
  )
  if(NOT CMAKE_BUILD_TYPE)
    message(STATUS "Setting build type to \"${default_build_type}\" as none was specified")
    set(CMAKE_BUILD_TYPE "${default_build_type}" CACHE STRING
      "Choose the type of build."
      FORCE
    )
  endif()
endif()

include(TryAppendCFlags)
if(MSVC)
  # Keep the following commands ordered lexicographically.
  try_append_c_flags(/W3) # Production quality warning level.
  try_append_c_flags(/wd4146) # Disable warning C4146 "unary minus operator applied to unsigned type, result still unsigned".
  try_append_c_flags(/wd4244) # Disable warning C4244 "'conversion' conversion from 'type1' to 'type2', possible loss of data".
  try_append_c_flags(/wd4267) # Disable warning C4267 "'var' : conversion from 'size_t' to 'type', possible loss of data".
  # Eliminate deprecation warnings for the older, less secure functions.
  add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
else()
  # Keep the following commands ordered lexicographically.
  try_append_c_flags(-pedantic)
  try_append_c_flags(-Wall) # GCC >= 2.95 and probably many other compilers.
  try_append_c_flags(-Wcast-align) # GCC >= 2.95.
  try_append_c_flags(-Wcast-align=strict) # GCC >= 8.0.
  try_append_c_flags(-Wconditional-uninitialized) # Clang >= 3.0 only.
  try_append_c_flags(-Wextra) # GCC >= 3.4, this is the newer name of -W, which we don't use because older GCCs will warn about unused functions.
  try_append_c_flags(-Wnested-externs)
  try_append_c_flags(-Wno-long-long) # GCC >= 3.0, -Wlong-long is implied by -pedantic.
  try_append_c_flags(-Wno-overlength-strings) # GCC >= 4.2, -Woverlength-strings is implied by -pedantic.
  try_append_c_flags(-Wno-unused-function) # GCC >= 3.0, -Wunused-function is implied by -Wall.
  try_append_c_flags(-Wreserved-identifier) # Clang >= 13.0 only.
  try_append_c_flags(-Wshadow)
  try_append_c_flags(-Wstrict-prototypes)
  try_append_c_flags(-Wundef)
endif()

set(CMAKE_C_VISIBILITY_PRESET hidden)

# Ask CTest to create a "check" target (e.g., make check) as alias for the "test" target.
# CTEST_TEST_TARGET_ALIAS is not documented but supposed to be user-facing.
# See: https://gitlab.kitware.com/cmake/cmake/-/commit/816c9d1aa1f2b42d40c81a991b68c96eb12b6d2
set(CTEST_TEST_TARGET_ALIAS check)
include(CTest)
# We do not use CTest's BUILD_TESTING because a single toggle for all tests is too coarse for our needs.
mark_as_advanced(BUILD_TESTING)
if(SECP256K1_BUILD_BENCHMARK OR SECP256K1_BUILD_TESTS OR SECP256K1_BUILD_EXHAUSTIVE_TESTS OR SECP256K1_BUILD_CTIME_TESTS OR SECP256K1_BUILD_EXAMPLES)
  enable_testing()
endif()

add_subdirectory(src)
if(SECP256K1_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()

message("\n")
message("secp256k1 configure summary")
message("===========================")
message("Build artifacts:")
if(BUILD_SHARED_LIBS)
  set(library_type "Shared")
else()
  set(library_type "Static")
endif()

message("  library type ........................ ${library_type}")
message("Optional modules:")
message("  ECDH ................................ ${SECP256K1_ENABLE_MODULE_ECDH}")
message("  ECDSA pubkey recovery ............... ${SECP256K1_ENABLE_MODULE_RECOVERY}")
message("  extrakeys ........................... ${SECP256K1_ENABLE_MODULE_EXTRAKEYS}")
message("  schnorrsig .......................... ${SECP256K1_ENABLE_MODULE_SCHNORRSIG}")
message("  ElligatorSwift ...................... ${SECP256K1_ENABLE_MODULE_ELLSWIFT}")
message("Parameters:")
message("  ecmult window size .................. ${SECP256K1_ECMULT_WINDOW_SIZE}")
message("  ecmult gen precision bits ........... ${SECP256K1_ECMULT_GEN_PREC_BITS}")
message("Optional features:")
message("  assembly optimization ............... ${SECP256K1_ASM}")
message("  external callbacks .................. ${SECP256K1_USE_EXTERNAL_DEFAULT_CALLBACKS}")
if(SECP256K1_TEST_OVERRIDE_WIDE_MULTIPLY)
  message("  wide multiplication (test-only) ..... ${SECP256K1_TEST_OVERRIDE_WIDE_MULTIPLY}")
endif()
message("Optional binaries:")
message("  benchmark ........................... ${SECP256K1_BUILD_BENCHMARK}")
message("  noverify_tests ...................... ${SECP256K1_BUILD_TESTS}")
set(tests_status "${SECP256K1_BUILD_TESTS}")
if(CMAKE_BUILD_TYPE STREQUAL "Coverage")
  set(tests_status OFF)
endif()
message("  tests ............................... ${tests_status}")
message("  exhaustive tests .................... ${SECP256K1_BUILD_EXHAUSTIVE_TESTS}")
message("  ctime_tests ......................... ${SECP256K1_BUILD_CTIME_TESTS}")
message("  examples ............................ ${SECP256K1_BUILD_EXAMPLES}")
message("")
if(CMAKE_CROSSCOMPILING)
  set(cross_status "TRUE, for ${CMAKE_SYSTEM_NAME}, ${CMAKE_SYSTEM_PROCESSOR}")
else()
  set(cross_status "FALSE")
endif()
message("Cross compiling ....................... ${cross_status}")
message("Valgrind .............................. ${SECP256K1_VALGRIND}")
get_directory_property(definitions COMPILE_DEFINITIONS)
string(REPLACE ";" " " definitions "${definitions}")
message("Preprocessor defined macros ........... ${definitions}")
message("C compiler ............................ ${CMAKE_C_COMPILER}")
message("CFLAGS ................................ ${CMAKE_C_FLAGS}")
get_directory_property(compile_options COMPILE_OPTIONS)
string(REPLACE ";" " " compile_options "${compile_options}")
message("Compile options ....................... " ${compile_options})
if(NOT is_multi_config)
  message("Build type:")
  message(" - CMAKE_BUILD_TYPE ................... ${CMAKE_BUILD_TYPE}")
  string(TOUPPER "${CMAKE_BUILD_TYPE}" build_type)
  message(" - CFLAGS ............................. ${CMAKE_C_FLAGS_${build_type}}")
  message(" - LDFLAGS for executables ............ ${CMAKE_EXE_LINKER_FLAGS_${build_type}}")
  message(" - LDFLAGS for shared libraries ....... ${CMAKE_SHARED_LINKER_FLAGS_${build_type}}")
else()
  message("Supported configurations .............. ${CMAKE_CONFIGURATION_TYPES}")
  message("RelWithDebInfo configuration:")
  message(" - CFLAGS ............................. ${CMAKE_C_FLAGS_RELWITHDEBINFO}")
  message(" - LDFLAGS for executables ............ ${CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO}")
  message(" - LDFLAGS for shared libraries ....... ${CMAKE_SHARED_LINKER_FLAGS_RELWITHDEBINFO}")
  message("Debug configuration:")
  message(" - CFLAGS ............................. ${CMAKE_C_FLAGS_DEBUG}")
  message(" - LDFLAGS for executables ............ ${CMAKE_EXE_LINKER_FLAGS_DEBUG}")
  message(" - LDFLAGS for shared libraries ....... ${CMAKE_SHARED_LINKER_FLAGS_DEBUG}")
endif()
message("\n")
if(SECP256K1_EXPERIMENTAL)
  message(
    "  ******\n"
    "  WARNING: experimental build\n"
    "  Experimental features do not have stable APIs or properties, and may not be safe for production use.\n"
    "  ******\n"
  )
endif()
