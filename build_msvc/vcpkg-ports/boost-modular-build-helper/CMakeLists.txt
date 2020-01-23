cmake_minimum_required(VERSION 3.9)
project(boost CXX)

find_path(ZLIB_INCLUDE zlib.h)
find_path(BZIP2_INCLUDE bzlib.h)
find_path(ICU_PATH include/unicode/utf.h)

set(VCPKG_PLATFORM_TOOLSET external)

set(B2_OPTIONS)

if(DEFINED BOOST_CMAKE_FRAGMENT)
    include(${BOOST_CMAKE_FRAGMENT})
endif()

# Add build type specific options
if(BUILD_SHARED_LIBS)
    list(APPEND B2_OPTIONS runtime-link=shared)
    set(LIB_RUNTIME_LINK "shared")
else()
    list(APPEND B2_OPTIONS runtime-link=static)
    set(LIB_RUNTIME_LINK "static")
endif()

if(BUILD_SHARED_LIBS)
    list(APPEND B2_OPTIONS link=shared)
else()
    list(APPEND B2_OPTIONS link=static)
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    list(APPEND B2_OPTIONS address-model=64)
else()
    list(APPEND B2_OPTIONS address-model=32)
endif()

if(APPLE)
    list(APPEND B2_OPTIONS target-os=darwin toolset=clang)
else()
    list(APPEND B2_OPTIONS target-os=linux toolset=gcc)
endif()

# Properly handle compiler and linker flags passed by VCPKG
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CXXFLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_RELEASE}")
    set(CFLAGS "${CMAKE_C_FLAGS} ${CMAKE_C_FLAGS_RELEASE}")
    if(BUILD_SHARED_LIBS)
        set(LDFLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${CMAKE_SHARED_LINKER_FLAGS_RELEASE}")
    else()
        set(LDFLAGS "${CMAKE_STATIC_LINKER_FLAGS} ${CMAKE_STATIC_LINKER_FLAGS_RELEASE}")
    endif()
elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CXXFLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_DEBUG}")
    set(CFLAGS "${CMAKE_C_FLAGS} ${CMAKE_C_FLAGS_DEBUG}")
    if(BUILD_SHARED_LIBS)
        set(LDFLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${CMAKE_SHARED_LINKER_FLAGS_DEBUG}")
    else()
        set(LDFLAGS "${CMAKE_STATIC_LINKER_FLAGS} ${CMAKE_STATIC_LINKER_FLAGS_DEBUG}")
    endif()
endif()

string(STRIP "${CXXFLAGS}" CXXFLAGS)
string(STRIP "${CFLAGS}"   CFLAGS)
string(STRIP "${LDFLAGS}"  LDFLAGS)

if(NOT CXXFLAGS STREQUAL "")
    string(REPLACE " " " <cxxflags>" CXXFLAGS "<cxxflags>${CXXFLAGS}")
endif()
if(NOT CFLAGS STREQUAL "")
    string(REPLACE " " " <cflags>" CFLAGS "<cflags>${CFLAGS}")
endif()
if(NOT LDLAGS STREQUAL "")
    string(REPLACE " " " <linkflags>" LDFLAGS "<linkflags>${LDFLAGS}")
endif()

#set(CXXFLAGS "${CXXFLAGS} <compileflags>-Wno-error=unused-command-line-argument")
if(CMAKE_CXX_COMPILER_TARGET)
    set(CXXFLAGS "${CXXFLAGS} <compileflags>${CMAKE_CXX_COMPILE_OPTIONS_TARGET} <compileflags>${CMAKE_CXX_COMPILER_TARGET}")
endif()
if(CMAKE_CXX_COMPILER_EXTERNAL_TOOLCHAIN)
    set(CXXFLAGS "${CXXFLAGS} <compileflags>${CMAKE_CXX_COMPILE_OPTIONS_EXTERNAL_TOOLCHAIN} <compileflags>${CMAKE_CXX_COMPILER_EXTERNAL_TOOLCHAIN}")
endif()
if(CMAKE_SYSROOT AND CMAKE_CXX_COMPILE_OPTIONS_SYSROOT)
    set(CXXFLAGS "${CXXFLAGS} <compileflags>${CMAKE_CXX_COMPILE_OPTIONS_SYSROOT}${CMAKE_SYSROOT}")
endif()
foreach(INCDIR ${CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES})
    set(CXXFLAGS "${CXXFLAGS} <compileflags>${CMAKE_INCLUDE_FLAG_C}${CMAKE_INCLUDE_FLAG_C_SEP}${INCDIR}")
endforeach()

if(APPLE)
  set(CXXFLAGS "${CXXFLAGS} <compileflags>-D_DARWIN_C_SOURCE")
endif()

find_library(ZLIB_LIBPATH z)
list(APPEND B2_OPTIONS
    -sZLIB_BINARY=z
    -sZLIB_LIBPATH="${ZLIB_LIBPATH}"
)
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    find_library(BZIP2_LIBPATH bz2)
    list(APPEND B2_OPTIONS
        -sBZIP2_BINARY=bz2
        -sBZIP2_LIBPATH="${BZIP2_LIBPATH}"
        variant=release
    )
elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
    find_library(BZIP2_LIBPATH bz2d)
    list(APPEND B2_OPTIONS
        -sBZIP2_BINARY=bz2d
        -sBZIP2_LIBPATH="${BZIP2_LIBPATH}"
        variant=debug
    )
endif()

configure_file(${CMAKE_CURRENT_LIST_DIR}/user-config.jam ${CMAKE_CURRENT_BINARY_DIR}/user-config.jam @ONLY)

set(NUMBER_OF_PROCESSORS 1)
if(DEFINED ENV{NUMBER_OF_PROCESSORS})
    set(NUMBER_OF_PROCESSORS $ENV{NUMBER_OF_PROCESSORS})
elseif(CMAKE_HOST_LINUX)
    execute_process(
        COMMAND nproc
        OUTPUT_VARIABLE NUMBER_OF_PROCESSORS
    )
    string(REPLACE "\n" "" NUMBER_OF_PROCESSORS "${NUMBER_OF_PROCESSORS}")
    string(REPLACE " " "" NUMBER_OF_PROCESSORS "${NUMBER_OF_PROCESSORS}")
endif()
if(NOT NUMBER_OF_PROCESSORS)
    set(NUMBER_OF_PROCESSORS 1)
endif()

add_custom_target(boost ALL
    COMMAND "${B2_EXE}"
        --user-config=${CMAKE_CURRENT_BINARY_DIR}/user-config.jam
        --stagedir=${CMAKE_CURRENT_BINARY_DIR}/stage
        --build-dir=${CMAKE_CURRENT_BINARY_DIR}
        ${B2_OPTIONS}
        --layout=system
        --with-atomic
        --with-random
        --with-date_time
        --with-filesystem
        --with-system
        --with-thread
        --with-chrono
        -sZLIB_INCLUDE="${ZLIB_INCLUDE}"
        -sBZIP2_INCLUDE="${BZIP2_INCLUDE}"
        -sICU_PATH="${ICU_PATH}"
        -j${NUMBER_OF_PROCESSORS}
        -sBOOST_ROOT=${BOOST_BUILD_PATH}
        -sBOOST_BUILD_PATH=${BOOST_BUILD_PATH}
        --debug-configuration
        --ignore-site-config
        --hash
        -q

        architecture=x86
        threading=multi
        threadapi=pthread
        debug-symbols=on

        stage
    WORKING_DIRECTORY ${SOURCE_PATH}/build
)

set(SUBDIR ${CMAKE_CURRENT_BINARY_DIR}/stage/lib)
install(
    CODE "file(GLOB LIBS ${SUBDIR}/*.so.* ${SUBDIR}/*.so ${SUBDIR}/*.a ${SUBDIR}/*.dylib ${SUBDIR}/*.dylib.*)\nif(LIBS)\nfile(INSTALL \${LIBS} DESTINATION \"\${CMAKE_INSTALL_PREFIX}/lib\")\nendif()"
)
