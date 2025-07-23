cmake_minimum_required(VERSION 3.22)

#=============================
# Project / Package metadata
#=============================
project(libsecp256k1
  # The package (a.k.a. release) version is based on semantic versioning 2.0.0 of
  # the API. All changes in experimental modules are treated as
  # backwards-compatible and therefore at most increase the minor version.
  VERSION 0.7.1
  DESCRIPTION "Optimized C library for ECDSA signatures and secret/public key operations on curve secp256k1."
  HOMEPAGE_URL "https://github.com/bitcoin-core/secp256k1"
  LANGUAGES C
)
enable_testing()
include(CTestUseLaunchers) # Allow users to set CTEST_USE_LAUNCHERS in custom `ctest -S` scripts.
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

# The library version is based on libtool versioning of the ABI. The set of
# rules for updating the version can be found here:
# https://www.gnu.org/software/libtool/manual/html_node/Updating-version-info.html
# All changes in experimental modules are treated as if they don't affect the
# interface and therefore only increase the revision.
set(${PROJECT_NAME}_LIB_VERSION_CURRENT 6)
set(${PROJECT_NAME}_LIB_VERSION_REVISION 1)
set(${PROJECT_NAME}_LIB_VERSION_AGE 0)

#=============================
# Language setup
#=============================
set(CMAKE_C_STANDARD 90)
set(CMAKE_C_EXTENSIONS OFF)

#=============================
# Configurable options
#=============================
option(BUILD_SHARED_LIBS "Build shared libraries." ON)
option(SECP256K1_DISABLE_SHARED "Disable shared library. Overrides BUILD_SHARED_LIBS." OFF)
if(SECP256K1_DISABLE_SHARED)
  set(BUILD_SHARED_LIBS OFF)
endif()

option(SECP256K1_INSTALL "Enable installation." ${PROJECT_IS_TOP_LEVEL})

option(SECP256K1_ENABLE_API_VISIBILITY_ATTRIBUTES "Enable visibility attributes in the API." ON)

## Modules

# We declare all options before processing them, to make sure we can express
# dependencies while processing.
option(SECP256K1_ENABLE_MODULE_ECDH "Enable ECDH module." ON)
option(SECP256K1_ENABLE_MODULE_RECOVERY "Enable ECDSA pubkey recovery module." OFF)
option(SECP256K1_ENABLE_MODULE_EXTRAKEYS "Enable extrakeys module." ON)
option(SECP256K1_ENABLE_MODULE_SCHNORRSIG "Enable schnorrsig module." ON)
option(SECP256K1_ENABLE_MODULE_MUSIG "Enable musig module." ON)
option(SECP256K1_ENABLE_MODULE_ELLSWIFT "Enable ElligatorSwift module." ON)

option(SECP256K1_USE_EXTERNAL_DEFAULT_CALLBACKS "Enable external default callback functions." OFF)
if(SECP256K1_USE_EXTERNAL_DEFAULT_CALLBACKS)
  add_compile_definitions(USE_EXTERNAL_DEFAULT_CALLBACKS=1)
endif()

set(SECP256K1_ECMULT_WINDOW_SIZE 15 CACHE STRING "Window size for ecmult precomputation for verification, specified as integer in range [2..24]. The default value is a reasonable setting for desktop machines (currently 15). [default=15]")
set_property(CACHE SECP256K1_ECMULT_WINDOW_SIZE PROPERTY STRINGS 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24)
include(CheckStringOptionValue)
check_string_option_value(SECP256K1_ECMULT_WINDOW_SIZE)
add_compile_definitions(ECMULT_WINDOW_SIZE=${SECP256K1_ECMULT_WINDOW_SIZE})

set(SECP256K1_ECMULT_GEN_KB 86 CACHE STRING "The size of the precomputed table for signing in multiples of 1024 bytes (on typical platforms). Larger values result in possibly better signing or key generation performance at the cost of a larger table. Valid choices are 2, 22, 86. The default value is a reasonable setting for desktop machines (currently 86). [default=86]")
set_property(CACHE SECP256K1_ECMULT_GEN_KB PROPERTY STRINGS 2 22 86)
check_string_option_value(SECP256K1_ECMULT_GEN_KB)
if(SECP256K1_ECMULT_GEN_KB EQUAL 2)
  add_compile_definitions(COMB_BLOCKS=2)
  add_compile_definitions(COMB_TEETH=5)
elseif(SECP256K1_ECMULT_GEN_KB EQUAL 22)
  add_compile_definitions(COMB_BLOCKS=11)
  add_compile_definitions(COMB_TEETH=6)
elseif(SECP256K1_ECMULT_GEN_KB EQUAL 86)
  add_compile_definitions(COMB_BLOCKS=43)
  add_compile_definitions(COMB_TEETH=6)
endif()

set(SECP256K1_TEST_OVERRIDE_WIDE_MULTIPLY "OFF" CACHE STRING "Test-only override of the (autodetected by the C code) \"widemul\" setting. Legal values are: \"OFF\", \"int128_struct\", \"int128\" or \"int64\". [default=OFF]")
set_property(CACHE SECP256K1_TEST_OVERRIDE_WIDE_MULTIPLY PROPERTY STRINGS "OFF" "int128_struct" "int128" "int64")
check_string_option_value(SECP256K1_TEST_OVERRIDE_WIDE_MULTIPLY)
if(SECP256K1_TEST_OVERRIDE_WIDE_MULTIPLY)
  string(TOUPPER "${SECP256K1_TEST_OVERRIDE_WIDE_MULTIPLY}" widemul_upper_value)
  add_compile_definitions(USE_FORCE_WIDEMUL_${widemul_upper_value}=1)
endif()
mark_as_advanced(FORCE SECP256K1_TEST_OVERRIDE_WIDE_MULTIPLY)

set(SECP256K1_ASM "AUTO" CACHE STRING "Assembly to use: \"AUTO\", \"OFF\", \"x86_64\" or \"arm32\" (experimental). [default=AUTO]")
set_property(CACHE SECP256K1_ASM PROPERTY STRINGS "AUTO" "OFF" "x86_64" "arm32")
check_string_option_value(SECP256K1_ASM)
if(SECP256K1_ASM STREQUAL "arm32")
  enable_language(ASM)
  include(CheckArm32Assembly)
  check_arm32_assembly()
  if(HAVE_ARM32_ASM)
    add_compile_definitions(USE_EXTERNAL_ASM=1)
  else()
    message(FATAL_ERROR "ARM32 assembly requested but not available.")
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
    message(FATAL_ERROR "x86_64 assembly requested but not available.")
  endif()
endif()

option(SECP256K1_EXPERIMENTAL "Allow experimental configuration options." OFF)
if(NOT SECP256K1_EXPERIMENTAL)
  if(SECP256K1_ASM STREQUAL "arm32")
    message(FATAL_ERROR "ARM32 assembly is experimental. Use -DSECP256K1_EXPERIMENTAL=ON to allow.")
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
  string(REGEX REPLACE "-O3( |$)" "-O2\\1" CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")
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

if(PROJECT_IS_TOP_LEVEL)
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
endif()

include(TryAppendCFlags)
if(MSVC)
  # For both cl and clang-cl compilers.
  try_append_c_flags(/W3) # Production quality warning level.
  # Eliminate deprecation warnings for the older, less secure functions.
  add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
else()
  try_append_c_flags(-Wall) # GCC >= 2.95 and probably many other compilers.
endif()  
if(CMAKE_C_COMPILER_ID STREQUAL "MSVC")
  # Keep the following commands ordered lexicographically.
  try_append_c_flags(/wd4146) # Disable warning C4146 "unary minus operator applied to unsigned type, result still unsigned".
  try_append_c_flags(/wd4244) # Disable warning C4244 "'conversion' conversion from 'type1' to 'type2', possible loss of data".
  try_append_c_flags(/wd4267) # Disable warning C4267 "'var' : conversion from 'size_t' to 'type', possible loss of data".
else()
  # Keep the following commands ordered lexicographically.
  try_append_c_flags(-pedantic)
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

set(print_msan_notice)
if(SECP256K1_BUILD_CTIME_TESTS)
  include(CheckMemorySanitizer)
  check_memory_sanitizer(msan_enabled)
  if(msan_enabled)
    try_append_c_flags(-fno-sanitize-memory-param-retval)
    set(print_msan_notice YES)
  endif()
  unset(msan_enabled)
endif()

set(SECP256K1_APPEND_CFLAGS "" CACHE STRING "Compiler flags that are appended to the command line after all other flags added by the build system. This variable is intended for debugging and special builds.")
if(SECP256K1_APPEND_CFLAGS)
  # Appending to this low-level rule variable is the only way to
  # guarantee that the flags appear at the end of the command line.
  string(APPEND CMAKE_C_COMPILE_OBJECT " ${SECP256K1_APPEND_CFLAGS}")
endif()

set(SECP256K1_APPEND_LDFLAGS "" CACHE STRING "Linker flags that are appended to the command line after all other flags added by the build system. This variable is intended for debugging and special builds.")
if(SECP256K1_APPEND_LDFLAGS)
  # Appending to this low-level rule variable is the only way to
  # guarantee that the flags appear at the end of the command line.
  string(APPEND CMAKE_C_CREATE_SHARED_LIBRARY " ${SECP256K1_APPEND_LDFLAGS}")
  string(APPEND CMAKE_C_LINK_EXECUTABLE " ${SECP256K1_APPEND_LDFLAGS}")
endif()

if(NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
endif()
if(NOT CMAKE_LIBRARY_OUTPUT_DIRECTORY)
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
endif()
if(NOT CMAKE_ARCHIVE_OUTPUT_DIRECTORY)
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
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
message("  musig ............................... ${SECP256K1_ENABLE_MODULE_MUSIG}")
message("  ElligatorSwift ...................... ${SECP256K1_ENABLE_MODULE_ELLSWIFT}")
message("Parameters:")
message("  ecmult window size .................. ${SECP256K1_ECMULT_WINDOW_SIZE}")
message("  ecmult gen table size ............... ${SECP256K1_ECMULT_GEN_KB} KiB")
message("Optional features:")
message("  assembly ............................ ${SECP256K1_ASM}")
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
message("API visibility attributes ............. ${SECP256K1_ENABLE_API_VISIBILITY_ATTRIBUTES}")
message("Valgrind .............................. ${SECP256K1_VALGRIND}")
get_directory_property(definitions COMPILE_DEFINITIONS)
string(REPLACE ";" " " definitions "${definitions}")
message("Preprocessor defined macros ........... ${definitions}")
message("C compiler ............................ ${CMAKE_C_COMPILER_ID} ${CMAKE_C_COMPILER_VERSION}, ${CMAKE_C_COMPILER}")
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
if(SECP256K1_APPEND_CFLAGS)
  message("SECP256K1_APPEND_CFLAGS ............... ${SECP256K1_APPEND_CFLAGS}")
endif()
if(SECP256K1_APPEND_LDFLAGS)
  message("SECP256K1_APPEND_LDFLAGS .............. ${SECP256K1_APPEND_LDFLAGS}")
endif()
message("")
if(print_msan_notice)
  message(
    "Note:\n"
    "  MemorySanitizer detected, tried to add -fno-sanitize-memory-param-retval to compile options\n"
    "  to avoid false positives in ctime_tests. Pass -DSECP256K1_BUILD_CTIME_TESTS=OFF to avoid this.\n"
  )
endif()
if(SECP256K1_EXPERIMENTAL)
  message(
    "  ******\n"
    "  WARNING: experimental build\n"
    "  Experimental features do not have stable APIs or properties, and may not be safe for production use.\n"
    "  ******\n"
  )
endif()
