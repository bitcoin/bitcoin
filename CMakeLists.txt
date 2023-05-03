# Copyright (c) 2023 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# IMPORTANT: Changes which affect binary results may not be quietly gated
#            by CMake version.
#
# Debian 10 Buster, https://wiki.debian.org/LTS, EOL 2024:
#  - CMake 3.13.4, https://packages.debian.org/buster/cmake
#
# Ubuntu 22.04 Jammy, https://wiki.ubuntu.com/Releases, EOL 2032:
#  - CMake 3.22.1, https://packages.ubuntu.com/jammy/cmake
#
# Visual Studio 17 2022, https://visualstudio.microsoft.com:
#  - CMake 3.24
#
# All policies known to the running version of CMake and introduced
# in the 3.24 version or earlier will be set to use NEW behavior.
# All policies introduced in later versions will be unset.
# See: https://cmake.org/cmake/help/latest/manual/cmake-policies.7.html
cmake_minimum_required(VERSION 3.13...3.24)

project("Bitcoin Core"
  VERSION 24.99.0
  DESCRIPTION "Bitcoin client software"
  HOMEPAGE_URL "https://bitcoincore.org/"
  LANGUAGES CXX ASM
)

set(PACKAGE_NAME ${PROJECT_NAME})
set(CLIENT_VERSION_IS_RELEASE "false")
set(COPYRIGHT_YEAR "2023")
set(COPYRIGHT_HOLDERS "The %s developers")
set(COPYRIGHT_HOLDERS_FINAL "The ${PROJECT_NAME} developers")
set(PACKAGE_BUGREPORT "https://github.com/bitcoin/bitcoin/issues")

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/module)

# Configurable options.
# When adding a new option, end the <help_text> with a full stop for consistency.
include(CMakeDependentOption)
option(BUILD_DAEMON "Build bitcoind executable." ON)
option(BUILD_CLI "Build bitcoin-cli executable." ON)
option(BUILD_TX "Build bitcoin-tx executable." ON)
option(BUILD_UTIL "Build bitcoin-util executable." ON)
option(BUILD_UTIL_CHAINSTATE "Build experimental bitcoin-chainstate executable." OFF)
# We do not use CMake's BUILD_SHARED_LIBS option.
option(BUILD_SHARED "Build shared libraries." ON)
option(BUILD_STATIC "Build static libraries." ON)
if(NOT BUILD_SHARED AND NOT BUILD_STATIC)
  message(FATAL_ERROR "At least one of BUILD_SHARED and BUILD_STATIC must be enabled.")
endif()
option(BUILD_BITCOINCONSENSUS_LIB "Build bitcoinconsensus library." ON)
option(BUILD_BITCOINKERNEL_LIB "Build experimental bitcoinkernel library." ${BUILD_UTIL_CHAINSTATE})
option(ASM "Use assembly routines." ON)

option(ENABLE_WALLET "Enable wallet." ON)
# TODO: These tri-state options will be removed and most features
#       will become opt-in by default before merging into master.
include(TristateOption)
tristate_option(WITH_SQLITE "Enable SQLite wallet support." "if libsqlite3 is found." AUTO)
tristate_option(WITH_BDB "Enable Berkeley DB (BDB) wallet support." "if libdb_cxx is found." AUTO)
option(WARN_INCOMPATIBLE_BDB "Warn when using a Berkeley DB (BDB) version other than 4.8." ON)
cmake_dependent_option(BUILD_WALLET_TOOL "Build bitcoin-wallet tool." ON "ENABLE_WALLET" OFF)

cmake_dependent_option(CXX20 "Enable compilation in C++20 mode." OFF "NOT MSVC" ON)
option(THREADLOCAL "Enable features that depend on the C++ thread_local keyword (currently just thread names in debug logs)." ON)
option(HARDENING "Attempt to harden the resulting executables." ON)
option(REDUCE_EXPORTS "Attempt to reduce exported symbols in the resulting executables." OFF)
option(WERROR "Treat compiler warnings as errors." OFF)

tristate_option(CCACHE "Use ccache for compiling." "if ccache is found." AUTO)
tristate_option(WITH_NATPMP "Enable NAT-PMP." "if libnatpmp is found." AUTO)
tristate_option(WITH_MINIUPNPC "Enable UPnP." "if libminiupnpc is found." AUTO)
tristate_option(WITH_ZMQ "Enable ZMQ notifications." "if libzmq is found." AUTO)
tristate_option(WITH_USDT
  "Enable tracepoints for Userspace, Statically Defined Tracing."
  "if sys/sdt.h is found."
  AUTO
)
tristate_option(WITH_SECCOMP
  "Enable experimental syscall sandbox feature (-sandbox)."
  "if seccomp-bpf is found under Linux x86_64."
  AUTO
)
tristate_option(WITH_QRENCODE "Enable QR code support." "if libqrencode is found." AUTO)

option(BUILD_TESTS "Build test_bitcoin executable." ON)
option(BUILD_BENCH "Build bench_bitcoin executable." ON)
option(INSTALL_MAN "Install man pages." ON)

if(CXX20)
  set(CMAKE_CXX_STANDARD 20)
else()
  set(CMAKE_CXX_STANDARD 17)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(configure_warnings)

include(TryAppendCompilerFlag)
include(TryAppendLinkerFlag)

if(WIN32)
  #[=[
  This build system supports two ways to build binaries for Windows.

  1. Building on Windows using MSVC.
  Implementation notes:
  - /DWIN32 and /D_WINDOWS definitions are included into the CMAKE_CXX_FLAGS_INIT
    and CMAKE_CXX_FLAGS_INIT variables by default.
  - A run-time library is selected using the CMAKE_MSVC_RUNTIME_LIBRARY variable.
  - MSVC-specific options, for example, /Zc:__cplusplus, are additionally required.

  2. Cross-compiling using MinGW.
  Implementation notes:
  - WIN32 and _WINDOWS definitions must be provided explicitly.
  - A run-time library must be specified explicitly using _MT definition.
  ]=]

  add_compile_definitions(_WIN32_WINNT=0x0601 _WIN32_IE=0x0501 WIN32_LEAN_AND_MEAN NOMINMAX)

  if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    add_compile_options(/utf-8 /Zc:__cplusplus)
  endif()

  if(MINGW)
    add_compile_definitions(WIN32 _WINDOWS _MT)
    set(mingw_linker_flags "")
    # We require Windows 7 (NT 6.1) or later.
    try_append_linker_flag(mingw_linker_flags "-Wl,--major-subsystem-version,6")
    try_append_linker_flag(mingw_linker_flags "-Wl,--minor-subsystem-version,1")
    separate_arguments(mingw_linker_flags)
    add_link_options(${mingw_linker_flags})
  endif()
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  add_compile_definitions(MAC_OSX)
  # These flags are specific to ld64, and may cause issues with other linkers.
  # For example: GNU ld will interpret -dead_strip as -de and then try and use
  # "ad_strip" as the symbol for the entry point.
  set(macos_linker_flags "")
  try_append_linker_flag(macos_linker_flags "-Wl,-dead_strip")
  try_append_linker_flag(macos_linker_flags "-Wl,-dead_strip_dylibs")
  try_append_linker_flag(macos_linker_flags "-Wl,-headerpad_max_install_names")
  separate_arguments(macos_linker_flags)
  add_link_options(${macos_linker_flags})
endif()

if(CMAKE_CROSSCOMPILING AND DEPENDS_ALLOW_HOST_PACKAGES)
  list(APPEND CMAKE_FIND_ROOT_PATH "${CMAKE_SYSTEM_PREFIX_PATH}")
endif()

include(AddThreadsIfNeeded)
add_threads_if_needed()

if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.14)
  include(CheckPIESupported)
  check_pie_supported(OUTPUT_VARIABLE check_pie_output LANGUAGES CXX)
  if(CMAKE_CXX_LINK_PIE_SUPPORTED)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
  endif()
else()
  include(CheckSourceCompilesAndLinks)
  check_cxx_source_links_with_flags(-fPIE "int main(){}" COMPILER_SUPPORTS_PIE)
  if(COMPILER_SUPPORTS_PIE)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
  endif()
endif()

include(AddBoostIfNeeded)
add_boost_if_needed()

include(CheckSourceCompilesAndLinks)

include(AddLibeventIfNeeded)
add_libevent_if_needed()

include(cmake/introspection.cmake)

include(cmake/crc32c.cmake)
include(cmake/leveldb.cmake)
include(cmake/minisketch.cmake)
include(cmake/secp256k1.cmake)

# Redefine configuration flags.
add_compile_definitions($<$<CONFIG:Debug>:DEBUG>)
add_compile_definitions($<$<CONFIG:Debug>:DEBUG_LOCKORDER>)
add_compile_definitions($<$<CONFIG:Debug>:DEBUG_LOCKCONTENTION>)
add_compile_definitions($<$<CONFIG:Debug>:RPC_DOC_CHECK>)
add_compile_definitions($<$<CONFIG:Debug>:ABORT_ON_FAILED_ASSUME>)
if(MSVC)
  string(REGEX REPLACE "/DNDEBUG[ \t\r\n]*" "" CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO}")
  string(REGEX REPLACE "/DNDEBUG[ \t\r\n]*" "" CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
  string(REGEX REPLACE "/DNDEBUG[ \t\r\n]*" "" CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")
  string(REGEX REPLACE "/DNDEBUG[ \t\r\n]*" "" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
  string(REGEX REPLACE "/DNDEBUG[ \t\r\n]*" "" CMAKE_C_FLAGS_MINSIZEREL "${CMAKE_C_FLAGS_MINSIZEREL}")
  string(REGEX REPLACE "/DNDEBUG[ \t\r\n]*" "" CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL}")
else()
  string(REGEX REPLACE "-DNDEBUG[ \t\r\n]*" "" CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO}")
  string(REGEX REPLACE "-DNDEBUG[ \t\r\n]*" "" CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
  string(REGEX REPLACE "-DNDEBUG[ \t\r\n]*" "" CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")
  string(REGEX REPLACE "-DNDEBUG[ \t\r\n]*" "" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
  string(REGEX REPLACE "-DNDEBUG[ \t\r\n]*" "" CMAKE_C_FLAGS_MINSIZEREL "${CMAKE_C_FLAGS_MINSIZEREL}")
  string(REGEX REPLACE "-DNDEBUG[ \t\r\n]*" "" CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL}")

  # Prefer -O2 optimization level. (-O3 is CMake's default for Release for many compilers.)
  string(REGEX REPLACE "-O3[ \t\r\n]*" "-O2" CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")
  string(REGEX REPLACE "-O3[ \t\r\n]*" "-O2" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")

  set(debug_c_flags "")
  try_append_cflag(debug_c_flags "-O0")
  try_append_cflag(debug_c_flags "-g3")
  if(NOT C_SUPPORTS_G3)
    try_append_cflag(debug_c_flags "-g")
  endif()
  set(CMAKE_C_FLAGS_DEBUG "${debug_c_flags}")

  set(debug_cxx_flags "")
  try_append_cxxflag(debug_cxx_flags "-O0")
  try_append_cxxflag(debug_cxx_flags "-g3")
  if(NOT CXX_SUPPORTS_G3)
    try_append_cxxflag(debug_cxx_flags "-g")
  endif()
  try_append_cxxflag(debug_cxx_flags "-ftrapv")
  if(MINGW)
    try_append_cxxflag(debug_cxx_flags "-Wa,-mbig-obj")
  endif()
  set(CMAKE_CXX_FLAGS_DEBUG "${debug_cxx_flags}")
endif()

get_property(is_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
set(default_build_type "RelWithDebInfo")
if(is_multi_config)
  set(CMAKE_CONFIGURATION_TYPES "${default_build_type}" "Release" "Debug" "MinSizeRel" CACHE STRING
    "Supported configuration types."
    FORCE
  )
else()
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY
    STRINGS "${default_build_type}" "Release" "Debug" "MinSizeRel"
  )
  if(NOT CMAKE_BUILD_TYPE)
    message(STATUS "Setting build type to \"${default_build_type}\" as none was specified")
    set(CMAKE_BUILD_TYPE "${default_build_type}" CACHE STRING
      "Choose the type of build."
      FORCE
    )
  endif()
endif()

include(cmake/optional_qt.cmake)

include(cmake/optional.cmake)

set(hardened_cxx_flags "")
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  # All versions of gcc that we commonly use for building are subject to bug
  # https://gcc.gnu.org/bugzilla/show_bug.cgi?id=90348. To work around that, set
  # -fstack-reuse=none for all gcc builds.
  try_append_cxxflag(hardened_cxx_flags "-fstack-reuse=none")
endif()
if(HARDENING)
  if(MSVC)
    add_link_options(/DYNAMICBASE /HIGHENTROPYVA /NXCOMPAT)
  else()
    try_append_cxxflag(hardened_cxx_flags "-Wstack-protector")
    try_append_cxxflag(hardened_cxx_flags "-fstack-protector-all")
    try_append_cxxflag(hardened_cxx_flags "-fcf-protection=full")
    if(MINGW)
      link_libraries(ssp)
    else()
      # stack-clash-protection doesn't currently work, and likely should just be skipped for Windows.
      # See https://gcc.gnu.org/bugzilla/show_bug.cgi?id=90458 for more details.
      try_append_cxxflag(hardened_cxx_flags "-fstack-clash-protection")
    endif()

    set(hardened_linker_flags "")
    try_append_linker_flag(hardened_linker_flags "-Wl,--enable-reloc-section")
    try_append_linker_flag(hardened_linker_flags "-Wl,--dynamicbase")
    try_append_linker_flag(hardened_linker_flags "-Wl,--nxcompat")
    try_append_linker_flag(hardened_linker_flags "-Wl,--high-entropy-va")
    try_append_linker_flag(hardened_linker_flags "-Wl,-z,relro")
    try_append_linker_flag(hardened_linker_flags "-Wl,-z,now")
    try_append_linker_flag(hardened_linker_flags "-Wl,-z,separate-code")
    if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
      try_append_linker_flag(hardened_linker_flags "-Wl,-bind_at_load")
    endif()
    separate_arguments(hardened_linker_flags)
    add_link_options(${hardened_linker_flags})
  endif()
endif()
separate_arguments(hardened_cxx_flags)
add_compile_options(${hardened_cxx_flags})

if(REDUCE_EXPORTS)
  set(CMAKE_CXX_VISIBILITY_PRESET hidden)
  set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)
  if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    add_link_options(-Wl,--exclude-libs,ALL)
  endif()
endif()

if(WERROR)
  if(MSVC)
    add_compile_options(/WX)
  else()
    set(error_cxx_flags "")
    try_append_cxxflag(error_cxx_flags "-Werror")
    if(NOT CXX_SUPPORTS_WERROR)
      message(FATAL_ERROR "WERROR set but -Werror is not usable.")
    endif()

    # -Wreturn-type is broken in GCC for MinGW-w64.
    # https://sourceforge.net/p/mingw-w64/bugs/306/
    try_append_cxxflag(error_cxx_flags "-Werror=return-type"
      CHECK_PASSED_FLAG ""
      CHECK_FAILED_FLAG "-Wno-error=return-type"
      SOURCE "#include <cassert>\nint f(){ assert(false); }"
    )

    separate_arguments(error_cxx_flags)
    add_compile_options(${error_cxx_flags})
  endif()
endif()

if(BUILD_BITCOINCONSENSUS_LIB)
  set(HAVE_CONSENSUS_LIB ON)
endif()

find_package(Python3 3.8 COMPONENTS Interpreter)
set(PYTHON_COMMAND ${Python3_EXECUTABLE})

add_subdirectory(src)
add_subdirectory(test)

include(cmake/tests.cmake)

if(NOT CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  set(OBJCOPY ${CMAKE_OBJCOPY})
  set(STRIP ${CMAKE_STRIP})
  configure_file(contrib/devtools/split-debug.sh.in split-debug.sh @ONLY)
endif()

include(GetAllExecutables)
get_all_executables(executables)

add_custom_target(check-symbols
  COMMAND echo "Running symbol and dynamic library checks..."
  COMMAND ${PYTHON_COMMAND} ${CMAKE_SOURCE_DIR}/contrib/devtools/symbol-check.py ${executables}
  VERBATIM
)

if(HARDENING)
  add_custom_target(check-security
    COMMAND echo "Checking binary security..."
    COMMAND ${PYTHON_COMMAND} ${CMAKE_SOURCE_DIR}/contrib/devtools/security-check.py ${executables}
    VERBATIM
  )
else()
  add_custom_target(check-security)
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  set(exe_format MACHO)
elseif(WIN32)
  set(exe_format PE)
elseif(UNIX)
  set(exe_format ELF)
endif()
if(CMAKE_CROSSCOMPILING)
  string(REPLACE ";" " " c_compiler_command "${DEPENDS_C_COMPILER_WITH_LAUNCHER}")
else()
  set(c_compiler_command "${CMAKE_C_COMPILER}")
endif()
add_custom_target(test-security-check
  COMMAND env CC=${c_compiler_command} CFLAGS=${CMAKE_C_FLAGS} LDFLAGS=${CMAKE_EXE_LINKER_FLAGS} ${PYTHON_COMMAND} ${CMAKE_SOURCE_DIR}/contrib/devtools/test-security-check.py TestSecurityChecks.test_${exe_format}
  COMMAND env CC=${c_compiler_command} CFLAGS=${CMAKE_C_FLAGS} LDFLAGS=${CMAKE_EXE_LINKER_FLAGS} ${PYTHON_COMMAND} ${CMAKE_SOURCE_DIR}/contrib/devtools/test-symbol-check.py TestSymbolChecks.test_${exe_format}
  VERBATIM
)

if(MINGW AND TARGET bitcoin-qt AND TARGET bitcoind AND TARGET bitcoin-cli AND TARGET bitcoin-tx AND TARGET bitcoin-wallet AND TARGET bitcoin-util AND TARGET test_bitcoin)
  # TODO: After dropping Autotools-based build system, replace
  #       this code with the CPack NSIS Generator.
  # See https://cmake.org/cmake/help/latest/cpack_gen/nsis.html
  include(GenerateSetupNsi)
  generate_setup_nsi()
  add_custom_target(deploy
    COMMAND "${CMAKE_COMMAND}" --install "${CMAKE_BINARY_DIR}" --config "$<CONFIG>" --prefix "${CMAKE_BINARY_DIR}/release" --strip
    COMMAND "${CMAKE_COMMAND}" -E rename "${CMAKE_BINARY_DIR}/release/bin/$<TARGET_FILE_NAME:bitcoind>" "${CMAKE_BINARY_DIR}/release/$<TARGET_FILE_NAME:bitcoind>"
    COMMAND "${CMAKE_COMMAND}" -E rename "${CMAKE_BINARY_DIR}/release/bin/$<TARGET_FILE_NAME:bitcoin-qt>" "${CMAKE_BINARY_DIR}/release/$<TARGET_FILE_NAME:bitcoin-qt>"
    COMMAND "${CMAKE_COMMAND}" -E rename "${CMAKE_BINARY_DIR}/release/bin/$<TARGET_FILE_NAME:test_bitcoin>" "${CMAKE_BINARY_DIR}/release/$<TARGET_FILE_NAME:test_bitcoin>"
    COMMAND "${CMAKE_COMMAND}" -E rename "${CMAKE_BINARY_DIR}/release/bin/$<TARGET_FILE_NAME:bitcoin-cli>" "${CMAKE_BINARY_DIR}/release/$<TARGET_FILE_NAME:bitcoin-cli>"
    COMMAND "${CMAKE_COMMAND}" -E rename "${CMAKE_BINARY_DIR}/release/bin/$<TARGET_FILE_NAME:bitcoin-tx>" "${CMAKE_BINARY_DIR}/release/$<TARGET_FILE_NAME:bitcoin-tx>"
    COMMAND "${CMAKE_COMMAND}" -E rename "${CMAKE_BINARY_DIR}/release/bin/$<TARGET_FILE_NAME:bitcoin-wallet>" "${CMAKE_BINARY_DIR}/release/$<TARGET_FILE_NAME:bitcoin-wallet>"
    COMMAND "${CMAKE_COMMAND}" -E rename "${CMAKE_BINARY_DIR}/release/bin/$<TARGET_FILE_NAME:bitcoin-util>" "${CMAKE_BINARY_DIR}/release/$<TARGET_FILE_NAME:bitcoin-util>"
    COMMAND "${CMAKE_COMMAND}" -E remove_directory "${CMAKE_BINARY_DIR}/release/bin"
    COMMAND "${CMAKE_COMMAND}" -E remove_directory "${CMAKE_BINARY_DIR}/release/lib"
    COMMAND "${CMAKE_COMMAND}" -E remove_directory "${CMAKE_BINARY_DIR}/release/include"
    COMMAND "${CMAKE_COMMAND}" -E remove_directory "${CMAKE_BINARY_DIR}/release/share"
    COMMAND makensis -V2 "${CMAKE_BINARY_DIR}/share/setup.nsi"
    VERBATIM
  )
endif()

get_directory_property(definitions COMPILE_DEFINITIONS)
include(SeparateConfigs)
separate_configs(definitions)

message("\n")
message("Configure summary")
message("=================")
message("Executables:")
message("  bitcoind ............................ ${BUILD_DAEMON}")
message("  bitcoin-cli ......................... ${BUILD_CLI}")
message("  bitcoin-tx .......................... ${BUILD_TX}")
message("  bitcoin-util ........................ ${BUILD_UTIL}")
message("  bitcoin-chainstate (experimental) ... ${BUILD_UTIL_CHAINSTATE}")
message("  bitcoin-wallet ...................... ${BUILD_WALLET_TOOL}")
message("Libraries:")
message("  build shared ........................ ${BUILD_SHARED}")
message("  build static ........................ ${BUILD_STATIC}")
message("  libbitcoinconsensus ................. ${BUILD_BITCOINCONSENSUS_LIB}")
message("  libbitcoinkernel (experimental) ..... ${BUILD_BITCOINKERNEL_LIB}")
message("Wallet support:")
message("  SQLite, descriptor wallets .......... ${WITH_SQLITE}")
message("  Berkeley DB, legacy wallets ......... ${WITH_BDB}")
message("Optional packages:")
message("  GUI ................................. ${WITH_GUI}")
if(WITH_GUI)
  message("  QR code (GUI) ....................... ${WITH_QRENCODE}")
endif()
message("  NAT-PMP ............................. ${WITH_NATPMP}")
message("  UPnP ................................ ${WITH_MINIUPNPC}")
message("  ZeroMQ .............................. ${WITH_ZMQ}")
message("  USDT tracing ........................ ${WITH_USDT}")
message("  experimental syscall sandbox ........ ${WITH_SECCOMP}")
message("Tests:")
message("  test_bitcoin ........................ ${BUILD_TESTS}")
message("  bench_bitcoin ....................... ${BUILD_BENCH}")
message("")
if(CMAKE_CROSSCOMPILING)
  set(cross_status "TRUE, for ${CMAKE_SYSTEM_NAME}, ${CMAKE_SYSTEM_PROCESSOR}")
else()
  set(cross_status "FALSE")
endif()
message("Cross compiling ....................... ${cross_status}")
message("Preprocessor defined macros ........... ${definitions_ALL}")
message("C compiler ............................ ${CMAKE_C_COMPILER}")
message("CFLAGS ................................ ${CMAKE_C_FLAGS}")
message("C++ compiler .......................... ${CMAKE_CXX_COMPILER}")
message("CXXFLAGS .............................. ${CMAKE_CXX_FLAGS}")
get_directory_property(common_compile_options COMPILE_OPTIONS)
string(REPLACE ";" " " common_compile_options "${common_compile_options}")
message("Common compile options ................ ${common_compile_options}")
get_directory_property(common_link_options LINK_OPTIONS)
string(REPLACE ";" " " common_link_options "${common_link_options}")
message("Common link options ................... ${common_link_options}")
message("Linker flags for executables .......... ${CMAKE_EXE_LINKER_FLAGS}")
message("Linker flags for shared libraries ..... ${CMAKE_SHARED_LINKER_FLAGS}")
if(NOT is_multi_config)
  message("Build type:")
  message(" - CMAKE_BUILD_TYPE ................... ${CMAKE_BUILD_TYPE}")
  string(TOUPPER "${CMAKE_BUILD_TYPE}" build_type)
  message(" - Preprocessor defined macros ........ ${definitions_${build_type}}")
  message(" - CFLAGS ............................. ${CMAKE_C_FLAGS_${build_type}}")
  message(" - CXXFLAGS ........................... ${CMAKE_CXX_FLAGS_${build_type}}")
  message(" - LDFLAGS for executables ............ ${CMAKE_EXE_LINKER_FLAGS_${build_type}}")
  message(" - LDFLAGS for shared libraries ....... ${CMAKE_SHARED_LINKER_FLAGS_${build_type}}")
else()
  message("Supported configurations .............. ${CMAKE_CONFIGURATION_TYPES}")
  message("Debug configuration:")
  message(" - Preprocessor defined macros ........ ${definitions_DEBUG}")
  message(" - CFLAGS ............................. ${CMAKE_C_FLAGS_DEBUG}")
  message(" - CXXFLAGS ........................... ${CMAKE_CXX_FLAGS_DEBUG}")
  message(" - LDFLAGS for executables ............ ${CMAKE_EXE_LINKER_FLAGS_DEBUG}")
  message(" - LDFLAGS for shared libraries ....... ${CMAKE_SHARED_LINKER_FLAGS_DEBUG}")
  message("Release configuration:")
  message(" - Preprocessor defined macros ........ ${definitions_RELEASE}")
  message(" - CFLAGS ............................. ${CMAKE_C_FLAGS_RELEASE}")
  message(" - CXXFLAGS ........................... ${CMAKE_CXX_FLAGS_RELEASE}")
  message(" - LDFLAGS for executables ............ ${CMAKE_EXE_LINKER_FLAGS_RELEASE}")
  message(" - LDFLAGS for shared libraries ....... ${CMAKE_SHARED_LINKER_FLAGS_RELEASE}")
endif()
message("Use assembly routines ................. ${ASM}")
message("Treat compiler warnings as errors ..... ${WERROR}")
message("Use ccache for compiling .............. ${CCACHE}")
message("\n")
if(configure_warnings)
    message("  ******\n")
    foreach(warning IN LISTS configure_warnings)
      message(WARNING "${warning}")
    endforeach()
    message("  ******\n")
endif()
